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
#include "xvdr/command.h"
#include "xvdr/connection.h"

#include "xbmc_pvr_dll.h"
#include "xbmc_addon_types.h"

#include <string.h>
#include <sstream>
#include <string>
#include <iostream>
#include <stdlib.h>

using namespace ADDON;
using namespace XVDR;

CHelper_libXBMC_addon* XBMC = NULL;
CHelper_libXBMC_gui* GUI = NULL;
CHelper_libXBMC_pvr* PVR = NULL;

Demux* mDemuxer = NULL;
Connection* mConnection = NULL;
cXBMCCallbacks *mCallbacks = NULL;
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

  cXBMCSettings& s = cXBMCSettings::GetInstance();
  s.load();

  mConnection = new Connection(mCallbacks);
  mConnection->SetTimeout(s.ConnectTimeout() * 1000);
  mConnection->SetCompressionLevel(s.Compression() * 3);
  mConnection->SetAudioType(s.AudioType());

  TimeMs RetryTimeout;
  bool bConnected = false;

  while (!(bConnected = mConnection->Open(s.Hostname())) && RetryTimeout.Elapsed() < (uint32_t)s.ConnectTimeout() * 1000)
	XVDR::CondWait::SleepMs(100);

  if (!bConnected){
    delete mConnection;
    delete PVR;
    delete XBMC;
    delete mCallbacks;
    mConnection = NULL;
    PVR = NULL;
    XBMC = NULL;
    return ADDON_STATUS_LOST_CONNECTION;
  }

  if (!mConnection->EnableStatusInterface(s.HandleMessages()))
  {
    return ADDON_STATUS_LOST_CONNECTION;
  }

  mConnection->ChannelFilter(s.FTAChannels(), s.NativeLangOnly(), s.vcaids);
  mConnection->SetUpdateChannels(s.UpdateChannels());

  return ADDON_STATUS_OK;
}

ADDON_STATUS ADDON_GetStatus()
{
  if(mConnection != NULL) {
    if(mConnection->ConnectionLost()) {
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
  delete mConnection;
  delete PVR;
  delete XBMC;
  delete mCallbacks;

  mConnection = NULL;
  PVR = NULL;
  XBMC = NULL;
  mCallbacks = NULL;
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

  mConnection->SetTimeout(s.ConnectTimeout() * 1000);
  mConnection->SetCompressionLevel(s.Compression() * 3);
  mConnection->SetAudioType(s.AudioType());

  if(!bChanged)
    return ADDON_STATUS_OK;

  mConnection->EnableStatusInterface(s.HandleMessages());
  mConnection->SetUpdateChannels(s.UpdateChannels());
  mConnection->ChannelFilter(s.FTAChannels(), s.NativeLangOnly(), s.vcaids);

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
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bSupportsChannelScan        = (mConnection && mConnection->SupportChannelScan());
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = true;

  pCapabilities->bSupportsRecordingFolders   = true;
  pCapabilities->bSupportsRecordingPlayCount = false;
  pCapabilities->bSupportsLastPlayedPosition = false;

  return PVR_ERROR_NO_ERROR;
}

const char * GetBackendName(void)
{
  static std::string BackendName = mConnection ? mConnection->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion(void)
{
  static std::string BackendVersion;
  if (mConnection) {
    std::stringstream format;
    format << mConnection->GetVersion() << "(Protocol: " << mConnection->GetProtocol() << ")";
    BackendVersion = format.str();
  }
  return BackendVersion.c_str();
}

const char * GetConnectionString(void)
{
  static std::string ConnectionString;
  std::stringstream format;

  if (mConnection) {
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
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  return (mConnection->GetDriveSpace(iTotal, iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR DialogChannelScan(void)
{
  mCallbacks->Lock();

  cXBMCChannelScan scanner(mCallbacks);
  scanner.Open(cXBMCSettings::GetInstance().Hostname());

  mCallbacks->Unlock();
  return PVR_ERROR_NO_ERROR;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->Lock();

  mCallbacks->SetHandle(handle);
  PVR_ERROR rc = (mConnection->GetEPGForChannel(channel.iUniqueId, iStart, iEnd) ? PVR_ERROR_NO_ERROR: PVR_ERROR_SERVER_ERROR);

  mCallbacks->Unlock();
  return rc;
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount(void)
{
  if (!mConnection)
    return 0;

  return mConnection->GetChannelsCount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->Lock();

  mCallbacks->SetHandle(handle);
  PVR_ERROR rc = (mConnection->GetChannelsList(bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);


  mCallbacks->Unlock();
  return rc;
}


/*******************************************/
/** PVR Channelgroups Functions           **/

int GetChannelGroupsAmount()
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  return mConnection->GetChannelGroupCount(cXBMCSettings::GetInstance().AutoChannelGroups());
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->Lock();

  PVR_ERROR rc = PVR_ERROR_NO_ERROR;
  mCallbacks->SetHandle(handle);

  if(mConnection->GetChannelGroupCount(cXBMCSettings::GetInstance().AutoChannelGroups()) > 0)
    rc = mConnection->GetChannelGroupList(bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  mCallbacks->Unlock();
  return rc;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->Lock();

  mCallbacks->SetHandle(handle);
  PVR_ERROR rc = (mConnection->GetChannelGroupMembers(group.strGroupName, group.bIsRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);

  mCallbacks->Unlock();
  return rc;
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  if (!mConnection)
    return 0;

  return mConnection->GetTimersCount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->Lock();

  mCallbacks->SetHandle(handle);
  PVR_ERROR rc = (mConnection->GetTimersList() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);

  mCallbacks->Unlock();
  return rc;
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  Timer xvdrtimer;
  xvdrtimer << timer;

  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  return mConnection->AddTimer(xvdrtimer) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  int rc = mConnection->DeleteTimer(timer.iClientIndex, bForce);

  if(rc == XVDR_RET_OK)
	return PVR_ERROR_NO_ERROR;
  else if(rc == XVDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  Timer xvdrtimer;
  xvdrtimer << timer;

  return mConnection->UpdateTimer(xvdrtimer) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  if (!mConnection)
    return 0;

  return mConnection->GetRecordingsCount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  mCallbacks->Lock();

  mCallbacks->SetHandle(handle);
  PVR_ERROR rc = mConnection->GetRecordingsList() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  mCallbacks->Unlock();
  return rc;
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  return mConnection->RenameRecording(recording.strRecordingId, recording.strTitle) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!mConnection)
    return PVR_ERROR_SERVER_ERROR;

  int rc = mConnection->DeleteRecording(recording.strRecordingId);

  if(rc == XVDR_RET_OK)
    return PVR_ERROR_NO_ERROR;
  else if(rc == XVDR_RET_DATALOCKED)
    return PVR_ERROR_RECORDING_RUNNING;
  else
    return PVR_ERROR_SERVER_ERROR;
}

/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  mCallbacks->Lock();

  if (mDemuxer)
  {
    mDemuxer->Close();
    delete mDemuxer;
  }

  bool rc = false;
  mDemuxer = new Demux(mCallbacks);
  mDemuxer->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);
  mDemuxer->SetAudioType(cXBMCSettings::GetInstance().AudioType());
  mDemuxer->SetPriority(priotable[cXBMCSettings::GetInstance().Priority()]);

  if (mDemuxer->OpenChannel(cXBMCSettings::GetInstance().Hostname(), channel.iUniqueId)) {
    CurrentChannel = channel.iChannelNumber;
    rc = true;
  }

  mCallbacks->Unlock();
  return rc;
}

void CloseLiveStream(void)
{
  mCallbacks->Lock();

  if (mDemuxer)
  {
    mDemuxer->Close();
    delete mDemuxer;
    mDemuxer = NULL;
  }

  mCallbacks->Unlock();
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  if (!mDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  *pProperties << mDemuxer->GetStreamProperties();

  return PVR_ERROR_NO_ERROR;
}

void DemuxAbort(void)
{
}

void DemuxReset(void)
{
}

void DemuxFlush(void)
{
}

DemuxPacket* DemuxRead(void)
{
  if (!mDemuxer)
    return NULL;

  return (DemuxPacket*)mDemuxer->Read();
}

int GetCurrentClientChannel(void)
{
  if (mDemuxer)
    return CurrentChannel;

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  mCallbacks->Lock();

  if (mDemuxer == NULL)
  {
    mCallbacks->Unlock();
    return false;
  }

  bool rc = false;
  mDemuxer->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);
  mDemuxer->SetAudioType(cXBMCSettings::GetInstance().AudioType());
  mDemuxer->SetPriority(priotable[cXBMCSettings::GetInstance().Priority()]);

  if(mDemuxer->SwitchChannel(channel.iUniqueId)) {
    CurrentChannel = channel.iChannelNumber;
    rc = true;
  }

  mCallbacks->Unlock();
  return rc;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  mCallbacks->Lock();

  if (mDemuxer == NULL)
  {
    mCallbacks->Unlock();
    return PVR_ERROR_SERVER_ERROR;
  }

  signalStatus << mDemuxer->GetSignalStatus();

  mCallbacks->Unlock();
  return PVR_ERROR_NO_ERROR;
}


/*******************************************/
/** PVR Recording Stream Functions        **/

void CloseRecordedStream();

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  if(!mConnection)
    return false;

  mConnection->CloseRecording();
  mConnection->SetTimeout(cXBMCSettings::GetInstance().ConnectTimeout() * 1000);

  return mConnection->OpenRecording(recording.strRecordingId);
}

void CloseRecordedStream(void)
{
  if (!mConnection)
    return;

  mConnection->CloseRecording();
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!mConnection)
    return -1;

  return mConnection->ReadRecording(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (mConnection)
    return mConnection->SeekRecording(iPosition, iWhence);

  return -1;
}

long long PositionRecordedStream(void)
{
  if (mConnection)
    return mConnection->RecordingPosition();

  return 0;
}

long long LengthRecordedStream(void)
{
  if (mConnection)
    return mConnection->RecordingLength();

  return 0;
}

bool CanPauseStream()
{
  return true;
}

bool CanSeekStream()
{
  mCallbacks->Lock();
  bool rc = (mDemuxer == NULL);
  mCallbacks->Unlock();

  return rc;
}

void PauseStream(bool bPaused)
{
  if(mDemuxer == NULL)
    return;

  mDemuxer->Pause(bPaused);
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
