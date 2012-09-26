/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "XBMCAddon.h"
#include "XBMCCallbacks.h"
#include "XBMCChannelScan.h"
#include "XBMCSettings.h"

#include "xvdr/demux.h"
#include "xvdr/connection.h"
#include "xvdr/thread.h"

#include "xbmc_pvr_dll.h"
#include "xbmc_addon_types.h"

#include <string.h>
#include <sstream>
#include <string>
#include <iostream>
#include <stdlib.h>

using namespace ADDON;
using namespace XVDR;

CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_gui   *GUI    = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;

Demux* XVDRDemuxer = NULL;
Connection* XVDRData = NULL;
cXBMCCallbacks *mCallbacks = NULL;

Mutex XVDRMutex;
Mutex XVDRMutexDemux;
Mutex XVDRMutexRec;
int CurrentChannel = 0;

static int priotable[] = { 0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,99,100 };

extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    delete PVR;
    delete XBMC;
    PVR = NULL;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  XBMC->Log(LOG_DEBUG, "Creating VDR XVDR PVR-Client");

  mCallbacks = new cXBMCCallbacks;
  Callbacks::Register(mCallbacks);

  cXBMCSettings& s = cXBMCSettings::GetInstance();
  s.load();

  XVDRData = new Connection;
  XVDRData->SetTimeout(s.ConnectTimeout() * 1000);
  XVDRData->SetCompressionLevel(s.Compression() * 3);
  XVDRData->SetAudioType(s.AudioType());

  TimeMs RetryTimeout;
  bool bConnected = false;

  while (!(bConnected = XVDRData->Open(s.Hostname())) && RetryTimeout.Elapsed() < (uint)s.ConnectTimeout() * 1000)
    Session::SleepMs(100);

  if (!bConnected){
    delete XVDRData;
    delete PVR;
    delete XBMC;
    delete mCallbacks;
    XVDRData = NULL;
    PVR = NULL;
    XBMC = NULL;
    return ADDON_STATUS_LOST_CONNECTION;
  }


  if (!XVDRData->Login())
  {
    return ADDON_STATUS_LOST_CONNECTION;
  }

  if (!XVDRData->EnableStatusInterface(s.HandleMessages()))
  {
    return ADDON_STATUS_LOST_CONNECTION;
  }

  XVDRData->ChannelFilter(s.FTAChannels(), s.NativeLangOnly(), s.vcaids);
  XVDRData->SetUpdateChannels(s.UpdateChannels());

  return ADDON_STATUS_OK;
}

ADDON_STATUS ADDON_GetStatus()
{
  if(XVDRData != NULL) {
	  if(XVDRData->ConnectionLost()) {
		  return ADDON_STATUS_LOST_CONNECTION;
	  }
	  else {
		  return ADDON_STATUS_OK;
	  }
  }

  return ADDON_STATUS_UNKNOWN;
}

void ADDON_Destroy()
{
  delete XVDRData;
  delete PVR;
  delete XBMC;
  delete mCallbacks;

  XVDRData = NULL;
  PVR = NULL;
  XBMC = NULL;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  bool bChanged = false;
  cXBMCSettings& s = cXBMCSettings::GetInstance();
  bChanged = s.set(settingName, settingValue);

  if(!bChanged)
    return ADDON_STATUS_OK;

  if(strcmp(settingName, "host") == 0)
    return ADDON_STATUS_NEED_RESTART;

  s.checkValues();

  XVDRData->SetTimeout(s.ConnectTimeout() * 1000);
  XVDRData->SetCompressionLevel(s.Compression() * 3);
  XVDRData->SetAudioType(s.AudioType());

  if(!bChanged)
    return ADDON_STATUS_OK;

  XVDRData->EnableStatusInterface(s.HandleMessages());
  XVDRData->SetUpdateChannels(s.UpdateChannels());
  XVDRData->ChannelFilter(s.FTAChannels(), s.NativeLangOnly(), s.vcaids);

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  MutexLock lock(&XVDRMutex);

  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bSupportsChannelScan        = (XVDRData && XVDRData->SupportChannelScan());
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = true;

  pCapabilities->bSupportsRecordingFolders   = true;
  pCapabilities->bSupportsRecordingPlayCount = false;
  pCapabilities->bSupportsLastPlayedPosition = false;

  return PVR_ERROR_NO_ERROR;
}

const char * GetBackendName(void)
{
  MutexLock lock(&XVDRMutex);

  static std::string BackendName = XVDRData ? XVDRData->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion(void)
{
  MutexLock lock(&XVDRMutex);

  static std::string BackendVersion;
  if (XVDRData) {
    std::stringstream format;
    format << XVDRData->GetVersion() << "(Protocol: " << XVDRData->GetProtocol() << ")";
    BackendVersion = format.str();
  }
  return BackendVersion.c_str();
}

const char * GetConnectionString(void)
{
  MutexLock lock(&XVDRMutex);

  static std::string ConnectionString;
  std::stringstream format;

  if (XVDRData) {
    format << cXBMCSettings::GetInstance().Hostname();
  }
  else {
    format << cXBMCSettings::GetInstance().Hostname() << " (addon error!)";
  }
  ConnectionString = format.str();
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetDriveSpace(iTotal, iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR DialogChannelScan(void)
{
  MutexLock lock(&XVDRMutex);

  cXBMCChannelScan scanner;
  scanner.Open(cXBMCSettings::GetInstance().Hostname());
  return PVR_ERROR_NO_ERROR;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->SetHandle(handle);
  return (XVDRData->GetEPGForChannel(channel.iUniqueId, iStart, iEnd) ? PVR_ERROR_NO_ERROR: PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount(void)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetChannelsCount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->SetHandle(handle);
  return (XVDRData->GetChannelsList(bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channelgroups Functions           **/

int GetChannelGroupsAmount()
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->GetChannelGroupCount(cXBMCSettings::GetInstance().AutoChannelGroups());
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->SetHandle(handle);
  if(XVDRData->GetChannelGroupCount(cXBMCSettings::GetInstance().AutoChannelGroups()) > 0)
    return XVDRData->GetChannelGroupList(bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->SetHandle(handle);
  XVDRData->GetChannelGroupMembers(group.strGroupName, group.bIsRadio);
  return PVR_ERROR_NO_ERROR;
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetTimersCount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->SetHandle(handle);
  return (XVDRData->GetTimersList() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  MutexLock lock(&XVDRMutex);

  Timer xvdrtimer;
  xvdrtimer << timer;

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->AddTimer(xvdrtimer) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteTimer(timer.iClientIndex, bForce) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Timer xvdrtimer;
  xvdrtimer << timer;

  return XVDRData->UpdateTimer(xvdrtimer) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetRecordingsCount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->SetHandle(handle);
  return XVDRData->GetRecordingsList() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->RenameRecording(recording.strRecordingId, recording.strTitle) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  MutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteRecording(recording.strRecordingId) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

/*******************************************/
/** PVR Live Stream Functions             **/

void CloseLiveStream(void);

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  MutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
  }

  XVDRDemuxer = new Demux;
  XVDRDemuxer->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);
  XVDRDemuxer->SetAudioType(cXBMCSettings::GetInstance().AudioType());
  XVDRDemuxer->SetPriority(priotable[cXBMCSettings::GetInstance().Priority()]);

  if (XVDRDemuxer->OpenChannel(cXBMCSettings::GetInstance().Hostname(), channel.iUniqueId)) {
	  CurrentChannel = channel.iChannelNumber;
	  return true;
  }

  return false;
}

void CloseLiveStream(void)
{
  MutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
    XVDRDemuxer = NULL;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  MutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  const StreamProperties& streamprops = XVDRDemuxer->GetStreamProperties();

  *pProperties << streamprops;
  return PVR_ERROR_NO_ERROR;
}

void DemuxAbort(void)
{
  MutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxAbort");
}

void DemuxReset(void)
{
  MutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxReset");
}

void DemuxFlush(void)
{
  MutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxFlush");
}

DemuxPacket* DemuxRead(void)
{
  MutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return NULL;

  return (DemuxPacket*)XVDRDemuxer->Read();
}

int GetCurrentClientChannel(void)
{
  MutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
    return CurrentChannel;

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  MutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer == NULL)
    return false;

  XVDRDemuxer->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);
  XVDRDemuxer->SetAudioType(cXBMCSettings::GetInstance().AudioType());
  XVDRDemuxer->SetPriority(priotable[cXBMCSettings::GetInstance().Priority()]);

  if(XVDRDemuxer->SwitchChannel(channel.iUniqueId)) {
	  CurrentChannel = channel.iChannelNumber;
	  return true;
  }

  return false;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  MutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  const XVDR::SignalStatus& status = XVDRDemuxer->GetSignalStatus();

  signalStatus << status;

  return PVR_ERROR_NO_ERROR;
}


/*******************************************/
/** PVR Recording Stream Functions        **/

void CloseRecordedStream();

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  MutexLock lock(&XVDRMutexRec);

  if(!XVDRData)
    return false;

  CloseRecordedStream();

  XVDRData->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);

  return XVDRData->OpenRecording(recording.strRecordingId);
}

void CloseRecordedStream(void)
{
  MutexLock lock(&XVDRMutexRec);

  if (!XVDRData)
    return;

  XVDRData->CloseRecording();
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  MutexLock lock(&XVDRMutexRec);

  if (!XVDRData)
    return -1;

  return XVDRData->ReadRecording(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  MutexLock lock(&XVDRMutexRec);

  if (XVDRData)
    return XVDRData->SeekRecording(iPosition, iWhence);

  return -1;
}

long long PositionRecordedStream(void)
{
  MutexLock lock(&XVDRMutexRec);

  if (XVDRData)
    return XVDRData->RecordingPosition();

  return 0;
}

long long LengthRecordedStream(void)
{
  MutexLock lock(&XVDRMutexRec);

  if (XVDRData)
    return XVDRData->RecordingLength();

  return 0;
}

const char* GetPVRAPIVersion(void)
{
  static const char *strApiVersion = XBMC_PVR_API_VERSION;
  return strApiVersion;
}

const char* GetMininumPVRAPIVersion(void)
{
  static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
  return strMinApiVersion;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
unsigned int GetChannelSwitchDelay(void) { return 0; }
bool CanPauseStream() { return false; }
bool CanSeekStream() { return false; }
void PauseStream(bool bPaused) {}
}
