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

#include <stdlib.h>
#include <string.h>

#include "xvdr/connection.h"
#include "xvdr/clientinterface.h"
#include "xvdr/msgpacket.h"
#include "xvdr/command.h"

#include "iso639.h"

using namespace XVDR;

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

Connection::Connection(ClientInterface* client)
 : m_statusinterface(false)
 , m_aborting(false)
 , m_timercount(0)
 , m_updatechannels(2)
 , m_client(client)
 , m_protocol(0)
 , m_compressionlevel(0)
 , m_audiotype(0)
{
}

Connection::~Connection()
{
  Abort();
  Cancel(1);
  Close();
}

bool Connection::Open(const std::string& hostname, const std::string& name)
{
  m_aborting = false;

  if(!Session::Open(hostname))
  {
    m_client->Log(XVDR_ERROR, "%s - Can't connect to XVDR Server", __FUNCTION__);
    return false;
  }

  m_name = name;

  if(!Login())
	 return false;

  Start();

  return true;
}

bool Connection::Login()
{
  std::string code = m_client->GetLanguageCode();
  const char* lang = ISO639_FindLanguage(code);

  MsgPacket vrp(XVDR_LOGIN);
  vrp.setProtocolVersion(XVDRPROTOCOLVERSION);
  vrp.put_U8(m_compressionlevel);
  vrp.put_String(m_name.empty() ? "XBMC Media Center" : m_name.c_str());
  vrp.put_String((lang != NULL) ? lang : "");
  vrp.put_U8(m_audiotype);

  // read welcome
  MsgPacket* vresp = Session::ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "failed to read greeting from server");
	return false;
  }

  m_protocol                = vresp->get_U32();
  uint32_t    vdrTime       = vresp->get_U32();
  int32_t     vdrTimeOffset = vresp->get_S32();
  m_server                  = vresp->get_String();
  m_version                 = vresp->get_String();

  if (m_name.empty())
	m_client->Log(XVDR_INFO, "Logged in at '%u+%i' to '%s' Version: '%s' with protocol version '%u'", vdrTime, vdrTimeOffset, m_server.c_str(), m_version.c_str(), m_protocol);

  m_client->Log(XVDR_INFO, "Preferred Audio Language: %s", lang);

  delete vresp;
  return true;
}

void Connection::Abort()
{
  MutexLock lock(&m_mutex);
  m_aborting = true;
  Session::Abort();
}

void Connection::SignalConnectionLost()
{
  MutexLock lock(&m_mutex);

  if(m_aborting)
    return;

  Session::SignalConnectionLost();
}

void Connection::OnDisconnect()
{
  m_client->Log(XVDR_ERROR, "%s - connection lost !!!", __FUNCTION__);
  m_client->Notification(XVDR_ERROR, m_client->GetLocalizedString(30044));
}

void Connection::OnReconnect()
{
  m_client->Notification(XVDR_INFO, m_client->GetLocalizedString(30045));

  m_client->TriggerTimerUpdate();
  m_client->TriggerRecordingUpdate();
}

MsgPacket* Connection::ReadResult(MsgPacket* vrp)
{
  if(m_connectionLost)
	return Session::ReadResult(vrp);

  m_mutex.Lock();

  SMessage &message(m_queue[vrp->getUID()]);
  message.event = new CondWait();
  message.pkt   = NULL;

  m_mutex.Unlock();

  if(!Session::TransmitMessage(vrp))
  {
	MutexLock lock(&m_mutex);
    m_queue.erase(vrp->getUID());
    return NULL;
  }

  message.event->Wait(m_timeout);

  m_mutex.Lock();

  MsgPacket* vresp = message.pkt;
  delete message.event;

  m_queue.erase(vrp->getUID());

  m_mutex.Unlock();

  return vresp;
}

bool Connection::GetDriveSpace(long long *total, long long *used)
{
  MsgPacket vrp(XVDR_RECORDINGS_DISKSIZE);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t totalspace    = vresp->get_U32();
  uint32_t freespace     = vresp->get_U32();

  *total = totalspace;
  *used  = (totalspace - freespace);

  /* Convert from kBytes to Bytes */
  *total *= 1024;
  *used  *= 1024;

  delete vresp;
  return true;
}

bool Connection::SupportChannelScan()
{
  MsgPacket vrp(XVDR_SCAN_SUPPORTED);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->get_U32();
  delete vresp;
  return ret == XVDR_RET_OK ? true : false;
}

bool Connection::EnableStatusInterface(bool onOff)
{
  MsgPacket vrp(XVDR_ENABLESTATUSINTERFACE);
  vrp.put_U8(onOff);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t ret = vresp->get_U32();
  delete vresp;
  if(ret == XVDR_RET_OK)
  {
    m_statusinterface = onOff;
    return true;
  }

  return false;
}

bool Connection::SetUpdateChannels(uint8_t method)
{
  MsgPacket vrp(XVDR_UPDATECHANNELS);
  vrp.put_U8(method);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_INFO, "Setting channel update method not supported by server. Consider updating the XVDR server.");
    return false;
  }

  m_client->Log(XVDR_INFO, "Channel update method set to %i", method);

  uint32_t ret = vresp->get_U32();
  delete vresp;
  if (ret == XVDR_RET_OK)
  {
    m_updatechannels = method;
    return true;
  }

  return false;
}

bool Connection::ChannelFilter(bool fta, bool nativelangonly, std::vector<int>& caids)
{
  std::size_t count = caids.size();

  MsgPacket vrp(XVDR_CHANNELFILTER);
  vrp.put_U32(fta);
  vrp.put_U32(nativelangonly);
  vrp.put_U32(count);

  for(int i = 0; i < count; i++)
    vrp.put_U32(caids[i]);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_INFO, "Channel filter method not supported by server. Consider updating the XVDR server.");
    return false;
  }

  m_client->Log(XVDR_INFO, "Channel filter set");

  uint32_t ret = vresp->get_U32();
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

int Connection::GetChannelsCount()
{
  MsgPacket vrp(XVDR_CHANNELS_GETCOUNT);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->get_U32();

  delete vresp;
  return count;
}

bool Connection::GetChannelsList(bool radio)
{
  MsgPacket vrp(XVDR_CHANNELS_GETCHANNELS);
  vrp.put_U32(radio);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  while (!vresp->eop())
  {
	Channel tag;

    tag[channel_number] = vresp->get_U32();
    tag[channel_name] = vresp->get_String();
    tag[channel_uid] = vresp->get_U32();
                            vresp->get_U32(); // still here for compatibility
    tag[channel_encryptionsystem] = vresp->get_U32();
                            vresp->get_U32(); // uint32_t vtype - currently unused
    tag[channel_isradio] = radio;
    tag[channel_inputformat] = "";
    tag[channel_streamurl] = "";
    tag[channel_iconpath] = "";
    tag[channel_ishidden] = false;

    m_client->TransferChannelEntry(tag);
  }

  delete vresp;
  return true;
}

bool Connection::GetEPGForChannel(uint32_t channeluid, time_t start, time_t end)
{
  MsgPacket vrp(XVDR_EPG_GETFORCHANNEL);
  vrp.put_U32(channeluid);
  vrp.put_U32(start);
  vrp.put_U32(end - start);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  while (!vresp->eop())
  {
    Epg tag;

    tag[epg_uid] = channeluid;
    tag[epg_broadcastid] = vresp->get_U32();
    uint32_t starttime = vresp->get_U32();
    tag[epg_starttime] = starttime;
    tag[epg_endtime] = starttime + vresp->get_U32();
    uint32_t content        = vresp->get_U32();
    tag[epg_genretype] = content & 0xF0;
    tag[epg_genresubtype] = content & 0x0F;
    tag[epg_parentalrating] = vresp->get_U32();
    tag[epg_title] = vresp->get_String();
    tag[epg_plotoutline] = vresp->get_String();
    tag[epg_plot] = vresp->get_String();

    m_client->TransferEpgEntry(tag);
  }

  delete vresp;
  return true;
}


/** OPCODE's 60 - 69: XVDR network functions for timer access */

int Connection::GetTimersCount()
{
  // return caches values on connection loss
  if(ConnectionLost())
    return m_timercount;

  MsgPacket vrp(XVDR_TIMER_GETCOUNT);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return m_timercount;
  }

  m_timercount = vresp->get_U32();

  delete vresp;
  return m_timercount;
}

void Connection::ReadTimerPacket(MsgPacket* resp, Timer& tag) {
  tag[timer_index] = resp->get_U32();

  int iActive           = resp->get_U32();
  int iRecording        = resp->get_U32();
  int iPending          = resp->get_U32();

  tag[timer_state] = iRecording;
  tag[timer_priority] = resp->get_U32();
  tag[timer_lifetime] = resp->get_U32();
  tag[timer_channeluid] =  resp->get_U32();
  tag[timer_starttime] = resp->get_U32();
  tag[timer_endtime] = resp->get_U32();
  tag[timer_firstday] = resp->get_U32();
  int weekdays = resp->get_U32();
  tag[timer_weekdays] = weekdays;
  tag[timer_isrepeating] = ((weekdays == 0) ? false : true);

  const char* title = resp->get_String();
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

bool Connection::GetTimerInfo(unsigned int timernumber, Timer& tag)
{
  MsgPacket vrp(XVDR_TIMER_GET);
  vrp.put_U32(timernumber);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->get_U32();
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

bool Connection::GetTimersList()
{
  MsgPacket vrp(XVDR_TIMER_GETLIST);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    delete vresp;
    m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  uint32_t numTimers = vresp->get_U32();
  if (numTimers > 0)
  {
    while (!vresp->eop())
    {
      Timer timer;
      ReadTimerPacket(vresp, timer);
      m_client->TransferTimerEntry(timer);
    }
  }
  delete vresp;
  return true;
}

bool Connection::AddTimer(const Timer& timer)
{
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
	m_client->Log(XVDR_ERROR, "%s - Empty filename !", __FUNCTION__);
    return false;
  }

  // use timer margin to calculate start/end times
  uint32_t starttime = (uint32_t)timer[timer_starttime] - (uint32_t)timer[timer_marginstart] * 60;
  uint32_t endtime = (uint32_t)timer[timer_endtime] + (uint32_t)timer[timer_marginend] * 60;

  MsgPacket vrp(XVDR_TIMER_ADD);
  vrp.put_U32(1);
  vrp.put_U32(timer[timer_priority]);
  vrp.put_U32(timer[timer_lifetime]);
  vrp.put_U32(timer[timer_channeluid]);
  vrp.put_U32(starttime);
  vrp.put_U32(endtime);
  vrp.put_U32(timer[timer_isrepeating] ? timer[timer_firstday] : 0);
  vrp.put_U32(timer[timer_weekdays]);
  vrp.put_String(path.c_str());
  vrp.put_String("");

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }
  uint32_t returnCode = vresp->get_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
}

int Connection::DeleteTimer(uint32_t timerindex, bool force)
{
  MsgPacket vrp(XVDR_TIMER_DELETE);
  vrp.put_U32(timerindex);
  vrp.put_U32(force);

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->get_U32();
  delete vresp;

  return returnCode;
}

bool Connection::UpdateTimer(const Timer& timer)
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

  MsgPacket vrp(XVDR_TIMER_UPDATE);
  vrp.put_U32(timer[timer_index]);
  vrp.put_U32(2);
  vrp.put_U32(timer[timer_priority]);
  vrp.put_U32(timer[timer_lifetime]);
  vrp.put_U32(timer[timer_channeluid]);
  vrp.put_U32(starttime);
  vrp.put_U32(endtime);
  vrp.put_U32(timer[timer_isrepeating] ? timer[timer_firstday] : 0);
  vrp.put_U32(timer[timer_weekdays]);
  vrp.put_String(title.c_str());
  vrp.put_String("");

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->get_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
}

int Connection::GetRecordingsCount()
{
  MsgPacket vrp(XVDR_RECORDINGS_GETCOUNT);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return -1;
  }

  uint32_t count = vresp->get_U32();

  delete vresp;
  return count;
}

bool Connection::GetRecordingsList()
{
  MsgPacket vrp(XVDR_RECORDINGS_GETLIST);

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
	m_client->Log(XVDR_ERROR, "%s - Can't get response packet", __FUNCTION__);
    return false;
  }

  while (!vresp->eop())
  {
	RecordingEntry rec;
	rec[recording_time] = vresp->get_U32();
	rec[recording_duration] = vresp->get_U32();
	rec[recording_priority] = vresp->get_U32();
	rec[recording_lifetime] = vresp->get_U32();
	rec[recording_channelname] = vresp->get_String();
	rec[recording_title] = vresp->get_String();
	rec[recording_plotoutline] = vresp->get_String();
	rec[recording_plot] = vresp->get_String();
	rec[recording_directory] = vresp->get_String();
	rec[recording_id] = vresp->get_String();
	rec[recording_streamurl] = "";
	rec[recording_genretype] = 0;
	rec[recording_genresubtype] = 0;
	rec[recording_playcount] = vresp->get_U32();

	m_client->TransferRecordingEntry(rec);
  }

  delete vresp;

  return true;
}

bool Connection::RenameRecording(const std::string& recid, const std::string& newname)
{
  m_client->Log(XVDR_DEBUG, "%s - uid: %s", __FUNCTION__, recid.c_str());

  MsgPacket vrp(XVDR_RECORDINGS_RENAME);
  vrp.put_String(recid.c_str());
  vrp.put_String(newname.c_str());

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->get_U32();
  delete vresp;

  return (returnCode == XVDR_RET_OK);
}

int Connection::DeleteRecording(const std::string& recid)
{
  MsgPacket vrp(XVDR_RECORDINGS_DELETE);
  vrp.put_String(recid.c_str());

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return false;
  }

  uint32_t returnCode = vresp->get_U32();
  delete vresp;

  return returnCode;
}

void Connection::OnResponsePacket(MsgPacket* pkt)
{
}

void Connection::Action()
{
  uint32_t lastPing = 0;
  MsgPacket* vresp;

  while (Running())
  {
    // try to reconnect
    if(ConnectionLost() && !TryReconnect())
    {
      CondWait::SleepMs(500);
      continue;
   }

    // read message
    vresp = Session::ReadMessage();

    // check if the connection is still up
    if ((vresp == NULL) && (time(NULL) - lastPing) > 5)
    {
      MutexLock lock(&m_mutex);
      if(m_queue.empty())
      {
        lastPing = time(NULL);

    	m_client->Log(XVDR_DEBUG, "Ping ...");

        if(!SendPing())
          SignalConnectionLost();
      }
    }

    // there wasn't any response
    if (vresp == NULL)
      continue;

    lastPing = time(NULL);

    // CHANNEL_REQUEST_RESPONSE

    if (vresp->getType() == XVDR_CHANNEL_REQUEST_RESPONSE)
    {
      MutexLock lock(&m_mutex);
      SMessages::iterator it = m_queue.find(vresp->getUID());
      if (it != m_queue.end())
      {
        it->second.pkt = vresp;
        it->second.event->Signal();
        vresp = NULL;
      }
    }

    // CHANNEL_STATUS

    else if (vresp->getType() == XVDR_CHANNEL_STATUS)
    {
      if (vresp->getMsgID() == XVDR_STATUS_MESSAGE)
      {
        uint32_t type = vresp->get_U32();
        const char* msgstr = vresp->get_String();

        if (type == 2)
          m_client->Notification(XVDR_ERROR, msgstr);
        if (type == 1)
          m_client->Notification(XVDR_WARNING, msgstr);
        else
          m_client->Notification(XVDR_INFO, msgstr);

      }
      else if (vresp->getMsgID() == XVDR_STATUS_RECORDING)
      {
                           vresp->get_U32(); // device currently unused
        uint32_t on      = vresp->get_U32();
        const char* str1 = vresp->get_String();
        const char* str2 = vresp->get_String();

        m_client->Recording(str1, str2, on);
        m_client->TriggerTimerUpdate();
      }
      else if (vresp->getMsgID() == XVDR_STATUS_TIMERCHANGE)
      {
    	m_client->Log(XVDR_DEBUG, "Server requested timer update");
        m_client->TriggerTimerUpdate();
      }
      else if (vresp->getMsgID() == XVDR_STATUS_CHANNELCHANGE)
      {
    	m_client->Log(XVDR_DEBUG, "Server requested channel update");
        m_client->TriggerChannelUpdate();
      }
      else if (vresp->getMsgID() == XVDR_STATUS_RECORDINGSCHANGE)
      {
    	m_client->Log(XVDR_DEBUG, "Server requested recordings update");
    	m_client->TriggerRecordingUpdate();
      }
    }

    // OTHER CHANNELID

    else
    {
      OnResponsePacket(vresp);
    }

    delete vresp;
  }
}

int Connection::GetChannelGroupCount(bool automatic)
{
  MsgPacket vrp(XVDR_CHANNELGROUP_GETCOUNT);
  vrp.put_U32(automatic);

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return 0;
  }

  uint32_t count = vresp->get_U32();

  delete vresp;
  return count;
}

bool Connection::GetChannelGroupList(bool bRadio)
{
  MsgPacket vrp(XVDR_CHANNELGROUP_LIST);
  vrp.put_U8(bRadio);

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return false;
  }

  while (!vresp->eop())
  {
    ChannelGroup group;

    group[channelgroup_name] = vresp->get_String();
    group[channelgroup_isradio] = vresp->get_U8();
    m_client->TransferChannelGroup(group);
  }

  delete vresp;
  return true;
}

bool Connection::GetChannelGroupMembers(const std::string& groupname, bool radio)
{
  MsgPacket vrp(XVDR_CHANNELGROUP_MEMBERS);
  vrp.put_String(groupname.c_str());
  vrp.put_U8(radio);

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return false;
  }

  while (!vresp->eop())
  {
    ChannelGroupMember member;
    member[channelgroupmember_name] = groupname;
    member[channelgroupmember_uid] = vresp->get_U32();
    member[channelgroupmember_number] = vresp->get_U32();

    m_client->TransferChannelGroupMember(member);
  }

  delete vresp;
  return true;
}

bool Connection::OpenRecording(const std::string& recid)
{
  MsgPacket vrp(XVDR_RECSTREAM_OPEN);
  vrp.put_String(recid.c_str());

  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
    return false;

  uint32_t returnCode = vresp->get_U32();
  if (returnCode == XVDR_RET_OK)
  {
    m_currentPlayingRecordFrames    = vresp->get_U32();
    m_currentPlayingRecordBytes     = vresp->get_U64();
    m_currentPlayingRecordPosition  = 0;
    m_recid = recid;
  }
  else {
	m_client->Log(XVDR_ERROR, "%s - Can't open recording", __FUNCTION__);
    m_recid.clear();

  }
  delete vresp;
  return (returnCode == XVDR_RET_OK);
}

bool Connection::CloseRecording()
{
  if(m_recid.empty())
    return false;

  m_recid.clear();

  MsgPacket vrp(XVDR_RECSTREAM_CLOSE);
  MsgPacket* vresp = ReadResult(&vrp);
  if (!vresp)
    return false;

  uint32_t returnCode = vresp->get_U32();
  return (returnCode == XVDR_RET_OK);
}

int Connection::ReadRecording(unsigned char* buf, uint32_t buf_size)
{
  if (ConnectionLost())
    return 0;

  if (m_currentPlayingRecordPosition >= m_currentPlayingRecordBytes)
    return 0;

  MsgPacket* vresp = NULL;

  MsgPacket vrp1(XVDR_RECSTREAM_UPDATE);
  if ((vresp = ReadResult(&vrp1)) != NULL)
  {
    uint32_t frames = vresp->get_U32();
    uint64_t bytes  = vresp->get_U64();

    if(frames != m_currentPlayingRecordFrames || bytes != m_currentPlayingRecordBytes)
    {
      m_currentPlayingRecordFrames = frames;
      m_currentPlayingRecordBytes  = bytes;
      m_client->Log(XVDR_DEBUG, "Size of recording changed: %lu bytes (%u frames)", bytes, frames);
    }
    delete vresp;
  }

  MsgPacket vrp2(XVDR_RECSTREAM_GETBLOCK);
  vrp2.put_U64(m_currentPlayingRecordPosition);
  vrp2.put_U32(buf_size);

  vresp = ReadResult(&vrp2);
  if (!vresp)
    return -1;

  uint32_t length = vresp->getPayloadLength();

  if(length == 0) {
	  return -1;
  }

  uint8_t *data = vresp->getPayload();
  if (length > buf_size)
  {
	m_client->Log(XVDR_ERROR, "%s: PANIC - Received more bytes as requested", __FUNCTION__);
    delete vresp;
    return 0;
  }

  memcpy(buf, data, length);
  m_currentPlayingRecordPosition += length;

  delete vresp;
  return length;
}

long long Connection::SeekRecording(long long pos, uint32_t whence)
{
  uint64_t nextPos = m_currentPlayingRecordPosition;

  switch (whence)
  {
    case SEEK_SET:
      nextPos = pos;
      break;

    case SEEK_CUR:
      nextPos += pos;
      break;

    case SEEK_END:
      nextPos = m_currentPlayingRecordBytes + pos;
      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos > m_currentPlayingRecordBytes)
    return -1;

  m_currentPlayingRecordPosition = nextPos;

  return m_currentPlayingRecordPosition;
}

long long Connection::RecordingPosition(void)
{
  return m_currentPlayingRecordPosition;
}

long long Connection::RecordingLength(void)
{
  return m_currentPlayingRecordBytes;
}

bool Connection::TryReconnect() {
  if(!Open(m_hostname))
    return false;

  EnableStatusInterface(m_statusinterface);
  ChannelFilter(m_ftachannels, m_nativelang, m_caids);
  SetUpdateChannels(m_updatechannels);

  m_client->Log(XVDR_DEBUG, "%s - reconnected", __FUNCTION__);
  m_connectionLost = false;

  OnReconnect();

  return true;
}

void Connection::SetTimeout(int ms)
{
  m_timeout = ms;
}

void Connection::SetCompressionLevel(int level)
{
#ifndef HAVE_ZLIB
  level = 0;
#else

  if (level < 0 || level > 9)
    return;

#endif

  m_compressionlevel = level;
}

void Connection::SetAudioType(int type)
{
  m_audiotype = type;
}

bool Connection::SetRecordingPlayCount(const std::string& recid, int count)
{
  MsgPacket vrp(XVDR_RECORDINGS_SETPLAYCOUNT);
  vrp.put_String(recid.c_str());
  vrp.put_U32(count);

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL)
  {
    delete vresp;
    return false;
  }

  delete vresp;
  return true;
}

bool Connection::SetRecordingLastPosition(const std::string& recid, int64_t pos)
{
  MsgPacket vrp(XVDR_RECORDINGS_SETPOSITION);
  vrp.put_String(recid.c_str());
  vrp.put_S64(pos);

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL)
  {
    delete vresp;
    return false;
  }

  delete vresp;
  return true;
}

int64_t Connection::GetRecordingLastPosition(const std::string& recid)
{
  MsgPacket vrp(XVDR_RECORDINGS_GETPOSITION);
  vrp.put_String(recid.c_str());

  MsgPacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->eop())
  {
    delete vresp;
    return -1;
  }

  int64_t pos = vresp->get_S64();
  delete vresp;

  return pos;
}
