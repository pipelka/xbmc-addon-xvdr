/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
 *      http://www.xbmc.org
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

#include <string.h>

#include "XVDRData.h"
#include "XVDRCallbacks.h"
#include "XVDRResponsePacket.h"
#include "requestpacket.h"
#include "xvdrcommand.h"

extern "C" {
#include "libTcpSocket/os-dependent_socket.h"
}

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

cXVDRData::cXVDRData()
 : m_statusinterface(false)
 , m_aborting(false)
 , m_timercount(0)
 , m_updatechannels(2)
{
}

cXVDRData::~cXVDRData()
{
  Abort();
  Cancel(1);
  Close();
}

bool cXVDRData::Open(const std::string& hostname, const char* name)
{
  m_aborting = false;

  if(!cXVDRSession::Open(hostname, name))
    return false;

  if(name != NULL) {
    SetDescription(name);
  }
  return true;
}

bool cXVDRData::Login()
{
  if(!cXVDRSession::Login())
    return false;

  Start();
  return true;
}

void cXVDRData::Abort()
{
  CMD_LOCK;
  m_aborting = true;
  cXVDRSession::Abort();
}

void cXVDRData::SignalConnectionLost()
{
  CMD_LOCK;

  if(m_aborting)
    return;

  cXVDRSession::SignalConnectionLost();
}

void cXVDRData::OnDisconnect()
{
  XVDRNotification(XVDR_ERROR, XVDRGetLocalizedString(30044));
}

void cXVDRData::OnReconnect()
{
  XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30045));

  EnableStatusInterface(m_statusinterface, true);
  ChannelFilter(m_ftachannels, m_nativelang, m_caids, true);
  SetUpdateChannels(m_updatechannels, true);

  XVDRTriggerTimerUpdate();
  XVDRTriggerRecordingUpdate();
}

cXVDRResponsePacket* cXVDRData::ReadResult(cRequestPacket* vrp)
{
  m_Mutex.Lock();

  SMessage &message(m_queue[vrp->getSerial()]);
  message.event = new cCondWait();
  message.pkt   = NULL;

  m_Mutex.Unlock();

  if(!cXVDRSession::SendMessage(vrp))
  {
    CMD_LOCK;
    m_queue.erase(vrp->getSerial());
    return NULL;
  }

  message.event->Wait(m_timeout);

  m_Mutex.Lock();

  cXVDRResponsePacket* vresp = message.pkt;
  delete message.event;

  m_queue.erase(vrp->getSerial());

  m_Mutex.Unlock();

  return vresp;
}

bool cXVDRData::GetDriveSpace(long long *total, long long *used)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_DISKSIZE))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t totalspace    = vresp->extract_U32();
  uint32_t freespace     = vresp->extract_U32();
  /* vresp->extract_U32(); percent not used */

  *total = totalspace;
  *used  = (totalspace - freespace);

  /* Convert from kBytes to Bytes */
  *total *= 1024;
  *used  *= 1024;

  delete vresp;
  return true;
}

bool cXVDRData::SupportChannelScan()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_SCAN_SUPPORTED))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  return ret == XVDR_RET_OK ? true : false;
}

bool cXVDRData::EnableStatusInterface(bool onOff, bool direct)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_ENABLESTATUSINTERFACE)) return false;
  if (!vrp.add_U8(onOff)) return false;

  cXVDRResponsePacket* vresp = direct ? cXVDRSession::ReadResult(&vrp) : ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  if(ret == XVDR_RET_OK)
  {
    m_statusinterface = onOff;
    return true;
  }

  return false;
}

bool cXVDRData::SetUpdateChannels(uint8_t method, bool direct)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_UPDATECHANNELS)) return false;
  if (!vrp.add_U8(method)) return false;

  cXVDRResponsePacket* vresp = direct ? cXVDRSession::ReadResult(&vrp) : ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_INFO, "Setting channel update method not supported by server. Consider updating the XVDR server.");
    return false;
  }

  XVDRLog(XVDR_INFO, "Channel update method set to %i", method);

  uint32_t ret = vresp->extract_U32();
  delete vresp;
  if (ret == XVDR_RET_OK)
  {
    m_updatechannels = method;
    return true;
  }

  return false;
}

bool cXVDRData::ChannelFilter(bool fta, bool nativelangonly, std::vector<int>& caids, bool direct)
{
  std::size_t count = caids.size();
  cRequestPacket vrp;

  if (!vrp.init(XVDR_CHANNELFILTER)) return false;
  if (!vrp.add_U32(fta)) return false;
  if (!vrp.add_U32(nativelangonly)) return false;
  if (!vrp.add_U32(count)) return false;

  for(int i = 0; i < count; i++)
    if (!vrp.add_U32(caids[i])) return false;

  cXVDRResponsePacket* vresp = direct ? cXVDRSession::ReadResult(&vrp) : ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_INFO, "Channel filter method not supported by server. Consider updating the XVDR server.");
    return false;
  }

  XVDRLog(XVDR_INFO, "Channel filter set");

  uint32_t ret = vresp->extract_U32();
  delete vresp;

  if (ret == XVDR_RET_OK)
  {
    m_ftachannels = fta;
    m_nativelang = nativelangonly;
    m_caids = caids;
    return true;
  }

  return false;
}

int cXVDRData::GetChannelsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELS_GETCOUNT))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cXVDRData::GetChannelsList(bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELS_GETCHANNELS))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }
  if (!vrp.add_U32(radio))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't add parameter to cRequestPacket", __FUNCTION__);
    return false;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(tag));

    tag.iChannelNumber    = vresp->extract_U32();
    strncpy(tag.strChannelName, vresp->extract_String(), sizeof(tag.strChannelName));
    tag.iUniqueId         = vresp->extract_U32();
                            vresp->extract_U32(); // still here for compatibility
    tag.iEncryptionSystem = vresp->extract_U32();
                            vresp->extract_U32(); // uint32_t vtype - currently unused
    tag.bIsRadio          = radio;
    tag.strInputFormat[0] = 0;
    tag.strStreamURL[0]   = 0;
    tag.strIconPath[0]    = 0;
    tag.bIsHidden         = false;

    XVDRTransferChannelEntry(&tag);
  }

  delete vresp;
  return true;
}

bool cXVDRData::GetEPGForChannel(const PVR_CHANNEL &channel, time_t start, time_t end)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_EPG_GETFORCHANNEL))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }
  if (!vrp.add_U32(channel.iUniqueId) || !vrp.add_U32(start) || !vrp.add_U32(end - start))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't add parameter to cRequestPacket", __FUNCTION__);
    return false;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  if (!vresp->serverError())
  {
    while (!vresp->end())
    {
      EPG_TAG tag;
      memset(&tag, 0 , sizeof(tag));

      tag.iChannelNumber      = channel.iChannelNumber;
      tag.iUniqueBroadcastId  = vresp->extract_U32();
      tag.startTime           = vresp->extract_U32();
      tag.endTime             = tag.startTime + vresp->extract_U32();
      uint32_t content        = vresp->extract_U32();
      tag.iGenreType          = content & 0xF0;
      tag.iGenreSubType       = content & 0x0F;
      tag.strGenreDescription = "";
      tag.iParentalRating     = vresp->extract_U32();
      tag.strTitle            = vresp->extract_String();
      tag.strPlotOutline      = vresp->extract_String();
      tag.strPlot             = vresp->extract_String();

      XVDRTransferEpgEntry(&tag);
    }
  }

  delete vresp;
  return true;
}


/** OPCODE's 60 - 69: XVDR network functions for timer access */

int cXVDRData::GetTimersCount()
{
  // return caches values on connection loss
  if(ConnectionLost())
    return m_timercount;

  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GETCOUNT))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return m_timercount;
  }

  m_timercount = vresp->extract_U32();

  delete vresp;
  return m_timercount;
}

void cXVDRData::ReadTimerPacket(cXVDRResponsePacket* resp, PVR_TIMER &tag) {
  tag.iClientIndex      = resp->extract_U32();
  int iActive           = resp->extract_U32();
  int iRecording        = resp->extract_U32();
  int iPending          = resp->extract_U32();
  if (iRecording)
    tag.state = PVR_TIMER_STATE_RECORDING;
  else if (iPending || iActive)
    tag.state = PVR_TIMER_STATE_SCHEDULED;
  else
    tag.state = PVR_TIMER_STATE_CANCELLED;
  tag.iPriority         = resp->extract_U32();
  tag.iLifetime         = resp->extract_U32();
                          resp->extract_U32(); // channel number - unused
  tag.iClientChannelUid = resp->extract_U32();
  tag.startTime         = resp->extract_U32();
  tag.endTime           = resp->extract_U32();
  tag.firstDay          = resp->extract_U32();
  tag.iWeekdays         = resp->extract_U32();
  tag.bIsRepeating      = tag.iWeekdays == 0 ? false : true;
  const char* title     = resp->extract_String();
  tag.iMarginStart      = 0;
  tag.iMarginEnd        = 0;

  char* p = (char*)strrchr(title, '~');
  if(p == NULL || *p == 0) {
	  strncpy(tag.strTitle, title, sizeof(tag.strTitle));
	  tag.strDirectory[0] = 0;
  }
  else {
	  const char* name = p + 1;

	  p[0] = 0;
	  const char* dir = title;

	  // replace dir separators
	  for(p = (char*)dir; *p != 0; p++)
		  if(*p == '~') *p = '/';

	  strncpy(tag.strTitle, name, sizeof(tag.strTitle));
	  strncpy(tag.strDirectory, dir, sizeof(tag.strDirectory));
  }
}

PVR_ERROR cXVDRData::GetTimerInfo(unsigned int timernumber, PVR_TIMER &tag)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GET))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  if (!vrp.add_U32(timernumber))
    return PVR_ERROR_UNKNOWN;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode != XVDR_RET_OK)
  {
    delete vresp;
    if (returnCode == XVDR_RET_DATAUNKNOWN)
      return PVR_ERROR_INVALID_PARAMETERS;
    else if (returnCode == XVDR_RET_ERROR)
      return PVR_ERROR_SERVER_ERROR;
  }

  ReadTimerPacket(vresp, tag);

  delete vresp;
  return PVR_ERROR_NO_ERROR;
}

bool cXVDRData::GetTimersList()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GETLIST))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t numTimers = vresp->extract_U32();
  if (numTimers > 0)
  {
    while (!vresp->end())
    {
      PVR_TIMER tag;
      ReadTimerPacket(vresp, tag);
      XVDRTransferTimerEntry(&tag);
    }
  }
  delete vresp;
  return true;
}

PVR_ERROR cXVDRData::AddTimer(const PVR_TIMER &timerinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_ADD))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // add directory in front of the title
  std::string path;
  if(timerinfo.strDirectory != NULL && strlen(timerinfo.strDirectory) > 0) {
    path += timerinfo.strDirectory;
    if(path == "/") {
      path.clear();
    }
    else if(path.size() > 1) {
      if(path[0] == '/') {
        path = path.substr(1);
      }
    }

    if(path.size() > 0 && path[path.size()-1] != '/') {
      path += "/";
    }
  }

  if(timerinfo.strTitle != NULL) {
    path += timerinfo.strTitle;
  }

  // replace directory separators
  for(std::size_t i=0; i<path.size(); i++) {
    if(path[i] == '/' || path[i] == '\\') {
      path[i] = '~';
    }
  }

  if(path.empty()) {
    XVDRLog(XVDR_ERROR, "%s - Empty filename !", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // use timer margin to calculate start/end times
  uint32_t starttime = timerinfo.startTime - timerinfo.iMarginStart*60;
  uint32_t endtime = timerinfo.endTime + timerinfo.iMarginEnd*60;

  if (!vrp.add_U32(timerinfo.state == PVR_TIMER_STATE_SCHEDULED))     return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iPriority))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iLifetime))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iClientChannelUid)) return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(starttime))  return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(endtime))    return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.bIsRepeating ? timerinfo.firstDay : 0))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iWeekdays))return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(path.c_str()))      return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKNOWN;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == XVDR_RET_DATALOCKED)
    return PVR_ERROR_ALREADY_PRESENT;
  else if (returnCode == XVDR_RET_DATAINVALID)
    return PVR_ERROR_FAILED;
  else if (returnCode == XVDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::DeleteTimer(const PVR_TIMER &timerinfo, bool force)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_DELETE))
    return PVR_ERROR_UNKNOWN;

  if (!vrp.add_U32(timerinfo.iClientIndex))
    return PVR_ERROR_UNKNOWN;

  if (!vrp.add_U32(force))
    return PVR_ERROR_UNKNOWN;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if (returnCode == XVDR_RET_DATALOCKED)
    return PVR_ERROR_FAILED;
  if (returnCode == XVDR_RET_RECRUNNING)
    return PVR_ERROR_RECORDING_RUNNING;
  else if (returnCode == XVDR_RET_DATAINVALID)
    return PVR_ERROR_FAILED;
  else if (returnCode == XVDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::UpdateTimer(const PVR_TIMER &timerinfo)
{
  // use timer margin to calculate start/end times
  uint32_t starttime = timerinfo.startTime - timerinfo.iMarginStart*60;
  uint32_t endtime = timerinfo.endTime + timerinfo.iMarginEnd*60;

  std::string dir = (timerinfo.strDirectory == NULL ? "" : timerinfo.strDirectory);
  while(dir[dir.size()-1] == '/' && dir.size() > 1)
    dir = dir.substr(0, dir.size()-1);

  std::string name = (timerinfo.strTitle == NULL ? "" : timerinfo.strTitle);
  std::string title;

  if(!dir.empty() && dir != "/")
	  title = dir + "/";

  title += name;

  // replace dir separators
  for(std::string::iterator i = title.begin(); i != title.end(); i++)
	  if(*i == '/') *i = '~';

  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_UPDATE))        return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iClientIndex))      return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.state == PVR_TIMER_STATE_SCHEDULED))     return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iPriority))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iLifetime))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iClientChannelUid)) return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(starttime))  return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(endtime))    return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.bIsRepeating ? timerinfo.firstDay : 0))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_U32(timerinfo.iWeekdays))return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(title.c_str()))   return PVR_ERROR_UNKNOWN;
  if (!vrp.add_String(""))                return PVR_ERROR_UNKNOWN;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;
  if (returnCode == XVDR_RET_DATAUNKNOWN)
    return PVR_ERROR_FAILED;
  else if (returnCode == XVDR_RET_DATAINVALID)
    return PVR_ERROR_FAILED;
  else if (returnCode == XVDR_RET_ERROR)
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

int cXVDRData::GetRecordingsCount()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_GETCOUNT))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return -1;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

PVR_ERROR cXVDRData::GetRecordingsList()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_GETLIST))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  while (!vresp->end())
  {
    PVR_RECORDING tag;
    tag.recordingTime   = vresp->extract_U32();
    tag.iDuration       = vresp->extract_U32();
    tag.iPriority       = vresp->extract_U32();
    tag.iLifetime       = vresp->extract_U32();
    strncpy(tag.strChannelName, vresp->extract_String(), sizeof(tag.strChannelName));
    strncpy(tag.strTitle, vresp->extract_String(), sizeof(tag.strTitle));
    strncpy(tag.strPlotOutline, vresp->extract_String(), sizeof(tag.strPlotOutline));
    strncpy(tag.strPlot, vresp->extract_String(), sizeof(tag.strPlot));
    strncpy(tag.strDirectory, vresp->extract_String(), sizeof(tag.strDirectory));
    strncpy(tag.strRecordingId, vresp->extract_String(), sizeof(tag.strRecordingId));
    tag.strStreamURL[0] = 0;
    tag.iGenreType      = 0;
    tag.iGenreSubType   = 0;
    tag.iPlayCount      = 0;

    XVDRTransferRecordingEntry(&tag);
  }

  delete vresp;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::RenameRecording(const PVR_RECORDING& recinfo, const char* newname)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_RENAME))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  // add uid
  XVDRLog(XVDR_DEBUG, "%s - uid: %s", __FUNCTION__, recinfo.strRecordingId);
  if (!vrp.add_String(recinfo.strRecordingId))
    return PVR_ERROR_UNKNOWN;

  // add new title
  if (!vrp.add_String(newname))
    return PVR_ERROR_UNKNOWN;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_SERVER_ERROR;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  if(returnCode != 0)
   return PVR_ERROR_FAILED;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cXVDRData::DeleteRecording(const PVR_RECORDING& recinfo)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_DELETE))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return PVR_ERROR_UNKNOWN;
  }

  if (!vrp.add_String(recinfo.strRecordingId))
    return PVR_ERROR_UNKNOWN;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return PVR_ERROR_UNKNOWN;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  switch(returnCode)
  {
    case XVDR_RET_DATALOCKED:
      return PVR_ERROR_FAILED;

    case XVDR_RET_RECRUNNING:
      return PVR_ERROR_RECORDING_RUNNING;

    case XVDR_RET_DATAINVALID:
      return PVR_ERROR_FAILED;

    case XVDR_RET_ERROR:
      return PVR_ERROR_SERVER_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

bool cXVDRData::OnResponsePacket(cXVDRResponsePacket* pkt)
{
  return false;
}

bool cXVDRData::SendPing()
{
  XVDRLog(XVDR_DEBUG, "%s", __FUNCTION__);

  cRequestPacket vrp;
  if (!vrp.init(XVDR_PING))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  cXVDRResponsePacket* vresp = cXVDRSession::ReadResult(&vrp);
  delete vresp;

  return (vresp != NULL);
}

void cXVDRData::Action()
{
  uint32_t lastPing = 0;
  cXVDRResponsePacket* vresp;

  SetPriority(19);

  while (Running())
  {
    // try to reconnect
    if(ConnectionLost() && !TryReconnect())
    {
      SleepMs(1000);
      continue;
   }

    // read message
    vresp = cXVDRSession::ReadMessage();

    // check if the connection is still up
    if ((vresp == NULL) && (time(NULL) - lastPing) > 5)
    {
      CMD_LOCK;
      if(m_queue.empty())
      {
        lastPing = time(NULL);

        if(!SendPing())
          SignalConnectionLost();
      }
    }

    // there wasn't any response
    if (vresp == NULL)
      continue;

    // CHANNEL_REQUEST_RESPONSE

    if (vresp->getChannelID() == XVDR_CHANNEL_REQUEST_RESPONSE)
    {
      CMD_LOCK;
      SMessages::iterator it = m_queue.find(vresp->getRequestID());
      if (it != m_queue.end())
      {
        it->second.pkt = vresp;
        it->second.event->Signal();
      }
      else
      {
        delete vresp;
      }
    }

    // CHANNEL_STATUS

    else if (vresp->getChannelID() == XVDR_CHANNEL_STATUS)
    {
      if (vresp->getRequestID() == XVDR_STATUS_MESSAGE)
      {
        uint32_t type = vresp->extract_U32();
        const char* msgstr = vresp->extract_String();

        if (type == 2)
          XVDRNotification(XVDR_ERROR, msgstr);
        if (type == 1)
          XVDRNotification(XVDR_WARNING, msgstr);
        else
          XVDRNotification(XVDR_INFO, msgstr);

      }
      else if (vresp->getRequestID() == XVDR_STATUS_RECORDING)
      {
                           vresp->extract_U32(); // device currently unused
        uint32_t on      = vresp->extract_U32();
        const char* str1 = vresp->extract_String();
        const char* str2 = vresp->extract_String();

        XVDRRecording(str1, str2, on);
        XVDRTriggerTimerUpdate();
      }
      else if (vresp->getRequestID() == XVDR_STATUS_TIMERCHANGE)
      {
        XVDRLog(XVDR_DEBUG, "Server requested timer update");
        XVDRTriggerTimerUpdate();
      }
      else if (vresp->getRequestID() == XVDR_STATUS_CHANNELCHANGE)
      {
        XVDRLog(XVDR_DEBUG, "Server requested channel update");
        XVDRTriggerChannelUpdate();
      }
      else if (vresp->getRequestID() == XVDR_STATUS_RECORDINGSCHANGE)
      {
        XVDRLog(XVDR_DEBUG, "Server requested recordings update");
        XVDRTriggerRecordingUpdate();
      }

      delete vresp;
    }

    // UNKOWN CHANNELID

    else if (!OnResponsePacket(vresp))
    {
      XVDRLog(XVDR_ERROR, "%s - Rxd a response packet on channel %lu !!", __FUNCTION__, vresp->getChannelID());
      delete vresp;
    }
  }
}

int cXVDRData::GetChannelGroupCount(bool automatic)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_GETCOUNT))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return 0;
  }

  if (!vrp.add_U32(automatic))
  {
    return 0;
  }

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return 0;
  }

  uint32_t count = vresp->extract_U32();

  delete vresp;
  return count;
}

bool cXVDRData::GetChannelGroupList(bool bRadio)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_LIST))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  vrp.add_U8(bRadio);

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL_GROUP tag;

    strncpy(tag.strGroupName, vresp->extract_String(), sizeof(tag.strGroupName));
    tag.bIsRadio = vresp->extract_U8();
    XVDRTransferChannelGroup(&tag);
  }

  delete vresp;
  return true;
}

bool cXVDRData::GetChannelGroupMembers(const PVR_CHANNEL_GROUP &group)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_MEMBERS))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  vrp.add_String(group.strGroupName);
  vrp.add_U8(group.bIsRadio);

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  while (!vresp->end())
  {
    PVR_CHANNEL_GROUP_MEMBER tag;
    strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
    tag.iChannelUniqueId = vresp->extract_U32();
    tag.iChannelNumber = vresp->extract_U32();

    XVDRTransferChannelGroupMember(&tag);
  }

  delete vresp;
  return true;
}
