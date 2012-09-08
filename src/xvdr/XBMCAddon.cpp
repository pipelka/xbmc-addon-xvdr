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

#include "XVDRDemux.h"
#include "XVDRRecording.h"
#include "XVDRData.h"
#include "XVDRThread.h"

#include "xbmc_pvr_dll.h"
#include "xbmc_addon_types.h"
#include "DVDDemuxPacket.h"

#include <string.h>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;
using namespace ADDON;

CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_gui   *GUI    = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;

cXVDRDemux      *XVDRDemuxer       = NULL;
cXVDRData       *XVDRData          = NULL;
cXVDRRecording  *XVDRRecording     = NULL;
cXBMCCallbacks  *Callbacks         = NULL;
cMutex          XVDRMutex;
cMutex          XVDRMutexDemux;
cMutex          XVDRMutexRec;

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

  Callbacks = new cXBMCCallbacks;
  cXVDRCallbacks::Register(Callbacks);

  cXBMCSettings& s = cXBMCSettings::GetInstance();
  s.load();

  XVDRData = new cXVDRData;
  XVDRData->SetTimeout(s.ConnectTimeout() * 1000);
  XVDRData->SetCompressionLevel(s.Compression() * 3);
  XVDRData->SetAudioType(s.AudioType());

  cTimeMs RetryTimeout;
  bool bConnected = false;

  while (!(bConnected = XVDRData->Open(s.Hostname())) && RetryTimeout.Elapsed() < (uint)s.ConnectTimeout() * 1000)
    cXVDRSession::SleepMs(100);

  if (!bConnected){
    delete XVDRData;
    delete PVR;
    delete XBMC;
    delete Callbacks;
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
  delete Callbacks;

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
  cMutexLock lock(&XVDRMutex);

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
  cMutexLock lock(&XVDRMutex);

  static std::string BackendName = XVDRData ? XVDRData->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion(void)
{
  cMutexLock lock(&XVDRMutex);

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
  cMutexLock lock(&XVDRMutex);

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
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRData->GetDriveSpace(iTotal, iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR DialogChannelScan(void)
{
  cMutexLock lock(&XVDRMutex);

  cXBMCChannelScan scanner;
  scanner.Open(cXBMCSettings::GetInstance().Hostname());
  return PVR_ERROR_NO_ERROR;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Callbacks->SetHandle(handle);
  return (XVDRData->GetEPGForChannel(channel, iStart, iEnd) ? PVR_ERROR_NO_ERROR: PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount(void)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetChannelsCount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Callbacks->SetHandle(handle);
  return (XVDRData->GetChannelsList(bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channelgroups Functions           **/

int GetChannelGroupsAmount()
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->GetChannelGroupCount(cXBMCSettings::GetInstance().AutoChannelGroups());
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Callbacks->SetHandle(handle);
  if(XVDRData->GetChannelGroupCount(cXBMCSettings::GetInstance().AutoChannelGroups()) > 0)
    return XVDRData->GetChannelGroupList(bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Callbacks->SetHandle(handle);
  XVDRData->GetChannelGroupMembers(group);
  return PVR_ERROR_NO_ERROR;
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetTimersCount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Callbacks->SetHandle(handle);
  return (XVDRData->GetTimersList() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteTimer(timer, bForce);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->UpdateTimer(timer);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return 0;

  return XVDRData->GetRecordingsCount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  Callbacks->SetHandle(handle);
  return XVDRData->GetRecordingsList();
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->RenameRecording(recording, recording.strTitle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  cMutexLock lock(&XVDRMutex);

  if (!XVDRData)
    return PVR_ERROR_SERVER_ERROR;

  return XVDRData->DeleteRecording(recording);
}

/*******************************************/
/** PVR Live Stream Functions             **/

void CloseLiveStream(void);

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
  }

  XVDRDemuxer = new cXVDRDemux;
  XVDRDemuxer->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);
  XVDRDemuxer->SetAudioType(cXBMCSettings::GetInstance().AudioType());
  XVDRDemuxer->SetPriority(priotable[cXBMCSettings::GetInstance().Priority()]);

  return XVDRDemuxer->OpenChannel(cXBMCSettings::GetInstance().Hostname(), channel);
}

void CloseLiveStream(void)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
  {
    XVDRDemuxer->Close();
    delete XVDRDemuxer;
    XVDRDemuxer = NULL;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRDemuxer->GetStreamProperties(pProperties) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

void DemuxAbort(void)
{
  cMutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxAbort");
}

void DemuxReset(void)
{
  cMutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxReset");
}

void DemuxFlush(void)
{
  cMutexLock lock(&XVDRMutexDemux);
  XBMC->Log(LOG_DEBUG, "DemuxFlush");
}

DemuxPacket* DemuxRead(void)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return NULL;

  return (DemuxPacket*)XVDRDemuxer->Read();
}

int GetCurrentClientChannel(void)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer)
    return XVDRDemuxer->CurrentChannel();

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (XVDRDemuxer == NULL)
    return false;

  XVDRDemuxer->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);
  XVDRDemuxer->SetAudioType(cXBMCSettings::GetInstance().AudioType());
  XVDRDemuxer->SetPriority(priotable[cXBMCSettings::GetInstance().Priority()]);

  return XVDRDemuxer->SwitchChannel(channel);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  cMutexLock lock(&XVDRMutexDemux);

  if (!XVDRDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (XVDRDemuxer->GetSignalStatus(signalStatus) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Recording Stream Functions        **/

void CloseRecordedStream();

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  cMutexLock lock(&XVDRMutexRec);

  if(!XVDRData)
    return false;

  CloseRecordedStream();

  XVDRRecording = new cXVDRRecording;
  XVDRRecording->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);

  return XVDRRecording->OpenRecording(cXBMCSettings::GetInstance().Hostname(), recording);
}

void CloseRecordedStream(void)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
  {
    XVDRRecording->Close();
    delete XVDRRecording;
    XVDRRecording = NULL;
  }
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  cMutexLock lock(&XVDRMutexRec);

  if (!XVDRRecording)
    return -1;

  return XVDRRecording->Read(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
    return XVDRRecording->Seek(iPosition, iWhence);

  return -1;
}

long long PositionRecordedStream(void)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
    return XVDRRecording->Position();

  return 0;
}

long long LengthRecordedStream(void)
{
  cMutexLock lock(&XVDRMutexRec);

  if (XVDRRecording)
    return XVDRRecording->Length();

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
}
