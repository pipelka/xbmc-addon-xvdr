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
	cXVDRChannel tag;

    tag[channel_number] = vresp->extract_U32();
    tag[channel_name] = vresp->extract_String();
    tag[channel_uid] = vresp->extract_U32();
                            vresp->extract_U32(); // still here for compatibility
    tag[channel_encryptionsystem] = vresp->extract_U32();
                            vresp->extract_U32(); // uint32_t vtype - currently unused
    tag[channel_isradio] = radio;
    tag[channel_inputformat] = "";
    tag[channel_streamurl] = "";
    tag[channel_iconpath] = "";
    tag[channel_ishidden] = false;

    XVDRTransferChannelEntry(tag);
  }

  delete vresp;
  return true;
}

bool cXVDRData::GetEPGForChannel(uint32_t channeluid, time_t start, time_t end)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_EPG_GETFORCHANNEL))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  if (!vrp.add_U32(channeluid) || !vrp.add_U32(start) || !vrp.add_U32(end - start))
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
      cXVDREpg tag;

      tag[epg_uid] = channeluid;
      tag[epg_broadcastid] = vresp->extract_U32();
      uint32_t starttime = vresp->extract_U32();
      tag[epg_starttime] = starttime;
      tag[epg_endtime] = starttime + vresp->extract_U32();
      uint32_t content        = vresp->extract_U32();
      tag[epg_genretype] = content & 0xF0;
      tag[epg_genresubtype] = content & 0x0F;
      //tag[epg_genredescription] = "";
      tag[epg_parentalrating] = vresp->extract_U32();
      tag[epg_title] = vresp->extract_String();
      tag[epg_plotoutline] = vresp->extract_String();
      tag[epg_plot] = vresp->extract_String();

      XVDRTransferEpgEntry(tag);
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

void cXVDRData::ReadTimerPacket(cXVDRResponsePacket* resp, cXVDRTimer& tag) {
  tag[timer_index] = resp->extract_U32();

  int iActive           = resp->extract_U32();
  int iRecording        = resp->extract_U32();
  int iPending          = resp->extract_U32();

  tag[timer_state] = iRecording;
  /*else if (iPending || iActive)
    tag.state = PVR_TIMER_STATE_SCHEDULED;
  else
    tag.state = PVR_TIMER_STATE_CANCELLED;*/
  tag[timer_priority] = resp->extract_U32();
  tag[timer_lifetime] = resp->extract_U32();
                          resp->extract_U32(); // channel number - unused
  tag[timer_channeluid] =  resp->extract_U32();
  tag[timer_starttime] = resp->extract_U32();
  tag[timer_endtime] = resp->extract_U32();
  tag[timer_firstday] = resp->extract_U32();
  int weekdays = resp->extract_U32();
  tag[timer_weekdays] = weekdays;
  tag[timer_isrepeating] = ((weekdays == 0) ? false : true);

  const char* title = resp->extract_String();
  tag[timer_marginstart] = 0;
  tag[timer_marginend] = 0;

  char* p = (char*)strrchr(title, '~');
  if(p == NULL || *p == 0) {
	  tag[timer_title] = title;
	  tag[timer_directory] =  "";
  }
  else {
	  const char* name = p + 1;

	  p[0] = 0;
	  const char* dir = title;

	  // replace dir separators
	  for(p = (char*)dir; *p != 0; p++)
		  if(*p == '~') *p = '/';

	  tag[timer_title] = name;
	  tag[timer_directory] =  dir;
  }
}

bool cXVDRData::GetTimerInfo(unsigned int timernumber, cXVDRTimer& tag)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_GET))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  if (!vrp.add_U32(timernumber))
    return false;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->extract_U32();
  if (returnCode != XVDR_RET_OK)
  {
    delete vresp;
    if (returnCode == XVDR_RET_DATAUNKNOWN)
      return false;
    else if (returnCode == XVDR_RET_ERROR)
      return false;
  }

  ReadTimerPacket(vresp, tag);

  delete vresp;
  return true;
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
      cXVDRTimer timer;
      ReadTimerPacket(vresp, timer);
      XVDRTransferTimerEntry(timer);
    }
  }
  delete vresp;
  return true;
}

bool cXVDRData::AddTimer(const cXVDRTimer& timer)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_ADD))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  // add directory in front of the title
  std::string path;
  std::string directory = timer[timer_directory];
  std::string title = timer[timer_title];

  if(!directory.empty()) {
    path += directory;
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

  if(!title.empty()) {
    path += title;
  }

  // replace directory separators
  for(std::size_t i=0; i<path.size(); i++) {
    if(path[i] == '/' || path[i] == '\\') {
      path[i] = '~';
    }
  }

  if(path.empty()) {
    XVDRLog(XVDR_ERROR, "%s - Empty filename !", __FUNCTION__);
    return false;
  }

  // use timer margin to calculate start/end times
  uint32_t starttime = (uint32_t)timer[timer_starttime] - (uint32_t)timer[timer_marginstart] * 60;
  uint32_t endtime = (uint32_t)timer[timer_endtime] + (uint32_t)timer[timer_marginend] * 60;

  vrp.add_U32(1);
  vrp.add_U32(timer[timer_priority]);
  vrp.add_U32(timer[timer_lifetime]);
  vrp.add_U32(timer[timer_channeluid]);
  vrp.add_U32(starttime);
  vrp.add_U32(endtime);
  vrp.add_U32(timer[timer_isrepeating] ? timer[timer_firstday] : 0);
  vrp.add_U32(timer[timer_weekdays]);
  vrp.add_String(path.c_str());
  vrp.add_String("");

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    XVDRLog(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }
  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
}

bool cXVDRData::DeleteTimer(uint32_t timerindex, bool force)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_TIMER_DELETE))
    return false;

  if (!vrp.add_U32(timerindex))
    return false;

  if (!vrp.add_U32(force))
    return false;

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
}

bool cXVDRData::UpdateTimer(const cXVDRTimer& timer)
{
  // use timer margin to calculate start/end times
  uint32_t starttime = timer[timer_starttime] - timer[timer_marginstart] * 60;
  uint32_t endtime = timer[timer_endtime] + timer[timer_marginend] * 60;

  std::string dir = timer[timer_directory];
  while(dir[dir.size()-1] == '/' && dir.size() > 1)
    dir = dir.substr(0, dir.size()-1);

  std::string name = timer[timer_title];
  std::string title;

  if(!dir.empty() && dir != "/")
	  title = dir + "/";

  title += name;

  // replace dir separators
  for(std::string::iterator i = title.begin(); i != title.end(); i++)
	  if(*i == '/') *i = '~';

  cRequestPacket vrp;
  vrp.init(XVDR_TIMER_UPDATE);
  vrp.add_U32(timer[timer_index]);
  vrp.add_U32(2);
  vrp.add_U32(timer[timer_priority]);
  vrp.add_U32(timer[timer_lifetime]);
  vrp.add_U32(timer[timer_channeluid]);
  vrp.add_U32(starttime);
  vrp.add_U32(endtime);
  vrp.add_U32(timer[timer_isrepeating] ? timer[timer_firstday] : 0);
  vrp.add_U32(timer[timer_weekdays]);
  vrp.add_String(title.c_str());
  vrp.add_String("");

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
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

bool cXVDRData::GetRecordingsList()
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_GETLIST))
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

  while (!vresp->end())
  {
	cXVDRRecordingEntry rec;
	rec[recording_time] = vresp->extract_U32();
	rec[recording_duration] = vresp->extract_U32();
	rec[recording_priority] = vresp->extract_U32();
	rec[recording_lifetime] = vresp->extract_U32();
	rec[recording_channelname] = vresp->extract_String();
	rec[recording_title] = vresp->extract_String();
	rec[recording_plotoutline] = vresp->extract_String();
	rec[recording_plot] = vresp->extract_String();
	rec[recording_directory] = vresp->extract_String();
	rec[recording_id] = vresp->extract_String();
	rec[recording_streamurl] = "";
	rec[recording_genretype] = 0;
	rec[recording_genresubtype] = 0;
	rec[recording_playcount] = 0;

    XVDRTransferRecordingEntry(rec);
  }

  delete vresp;

  return true;
}

bool cXVDRData::RenameRecording(const std::string& recid, const std::string& newname)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_RENAME))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  // add uid
  XVDRLog(XVDR_DEBUG, "%s - uid: %s", __FUNCTION__, recid.c_str());

  vrp.add_String(recid.c_str());
  vrp.add_String(newname.c_str());

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
}

bool cXVDRData::DeleteRecording(const std::string& recid)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_RECORDINGS_DELETE))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  vrp.add_String(recid.c_str());

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->extract_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
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
    cXVDRChannelGroup group;

    group[channelgroup_name] = vresp->extract_String();
    group[channelgroup_isradio] = vresp->extract_U8();
    XVDRTransferChannelGroup(group);
  }

  delete vresp;
  return true;
}

bool cXVDRData::GetChannelGroupMembers(const std::string& groupname, bool radio)
{
  cRequestPacket vrp;
  if (!vrp.init(XVDR_CHANNELGROUP_MEMBERS))
  {
    XVDRLog(XVDR_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }

  vrp.add_String(groupname.c_str());
  vrp.add_U8(radio);

  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }

  while (!vresp->end())
  {
    cXVDRChannelGroupMember member;
    member[channelgroupmember_name] = groupname;
    member[channelgroupmember_uid] = vresp->extract_U32();
    member[channelgroupmember_number] = vresp->extract_U32();

    XVDRTransferChannelGroupMember(member);
  }

  delete vresp;
  return true;
}
