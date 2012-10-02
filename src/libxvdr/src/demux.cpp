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

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "avcodec.h"
#include "xvdr/demux.h"
#include "xvdr/callbacks.h"
#include "xvdr/responsepacket.h"
#include "xvdr/requestpacket.h"
#include "xvdr/command.h"

using namespace XVDR;

Demux::Demux(Callbacks* client) : Connection(client), m_priority(50), m_queuelocked(false)
{
}

Demux::~Demux()
{
}

bool Demux::OpenChannel(const std::string& hostname, uint32_t channeluid)
{
  if(!Open(hostname))
    return false;

  return SwitchChannel(channeluid);
}

void Demux::CloseChannel() {
}

StreamProperties Demux::GetStreamProperties()
{
  MutexLock lock(&m_lock);
  return m_streams;
}

void Demux::CleanupPacketQueue()
{
  MutexLock lock(&m_lock);
  Packet* p = NULL;

  while(!m_queue.empty())
  {
    p = m_queue.front();
    m_queue.pop();
    m_client->FreePacket(p);
  }
}

void Demux::Abort()
{
  m_streams.clear();
  Connection::Abort();
  CleanupPacketQueue();
  m_cond.Signal();
}

Packet* Demux::Read()
{
  if(ConnectionLost())
  {
    return NULL;
  }

  Packet* p = NULL;
  m_lock.Lock();

  if(m_queuelocked) {
      m_lock.Unlock();
      return m_client->AllocatePacket(0);
  }

  bool bEmpty = m_queue.empty();

  // empty queue -> wait for packet
  if (bEmpty) {
         m_lock.Unlock();
         m_cond.Wait(100);
         m_lock.Lock();
         bEmpty = m_queue.empty();
  }

  if (!bEmpty)
  {
    p = m_queue.front();
    m_queue.pop();
  }

  m_lock.Unlock();

  if(p == NULL) {
    p = m_client->AllocatePacket(0);
  }

  return p;
}

void Demux::OnResponsePacket(ResponsePacket *resp) {
  {
    MutexLock lock(&m_lock);
    if(m_queuelocked)
    	return;
  }

  if (resp->getChannelID() != XVDR_CHANNEL_STREAM)
	return;

  Packet* pkt = NULL;
  int iStreamId = -1;

  switch (resp->getOpCodeID())
  {
    case XVDR_STREAM_CHANGE:
      StreamChange(resp);
      pkt = m_client->StreamChange(m_streams);
      break;

    case XVDR_STREAM_STATUS:
      StreamStatus(resp);
      break;

    case XVDR_STREAM_SIGNALINFO:
      StreamSignalInfo(resp);
      break;

    case XVDR_STREAM_CONTENTINFO:
      // send stream updates only if there are changes
      if(!StreamContentInfo(resp))
        break;

      pkt = m_client->ContentInfo(m_streams);
      break;

    case XVDR_STREAM_MUXPKT:
      // figure out the stream for this packet
      int id = resp->getStreamID();
      Stream& stream = m_streams[id];

      if(stream[stream_physicalid] != id) {
          m_client->Log(XVDR_DEBUG, "stream id %i not found", id);
    	  break;
      }

      int index = stream[stream_index];
      pkt = m_client->AllocatePacket(resp->getUserDataLength());
   	  m_client->SetPacketData(pkt, resp->getUserData(), index, resp->getDTS(), resp->getPTS());
   	  free(resp->getUserData());
      break;
  }

  if(pkt != NULL) {
	  {
	    MutexLock lock(&m_lock);

	    // limit queue size
	    if(m_queue.size() > 50)
	    {
	      m_client->FreePacket(pkt);
	      return;
	    }

        m_queue.push(pkt);
	  }
	  m_cond.Signal();
  }

  return;
}

bool Demux::SwitchChannel(uint32_t channeluid)
{
  m_client->Log(XVDR_DEBUG, "changing to channel %d (priority %i)", channeluid, m_priority);

  {
    MutexLock lock(&m_lock);
    m_queuelocked = true;
  }

  CleanupPacketQueue();
  m_cond.Signal();

  RequestPacket vrp(XVDR_CHANNELSTREAM_OPEN);
  vrp.add_U32(channeluid);
  vrp.add_S32(m_priority);

  ResponsePacket* vresp = ReadResult(&vrp);
  uint32_t rc = XVDR_RET_ERROR;

  if(vresp != NULL)
	  rc = vresp->extract_U32();

  delete vresp;

  if(rc == XVDR_RET_OK)
  {
    m_channeluid = channeluid;

    {
      MutexLock lock(&m_lock);
      m_queuelocked = false;
      m_streams.clear();
    }

    return true;
  }

  switch (rc)
  {
    // active recording
    case XVDR_RET_RECRUNNING:
      m_client->Notification(XVDR_INFO, m_client->GetLocalizedString(30062));
      break;
    // all receivers busy
    case XVDR_RET_DATALOCKED:
      m_client->Notification(XVDR_INFO, m_client->GetLocalizedString(30063));
      break;
    // encrypted channel
    case XVDR_RET_ENCRYPTED:
      m_client->Notification(XVDR_INFO, m_client->GetLocalizedString(30066));
      break;
    // error on switching channel
    default:
    case XVDR_RET_ERROR:
      m_client->Notification(XVDR_INFO, m_client->GetLocalizedString(30064));
      break;
    // invalid channel
    case XVDR_RET_DATAINVALID:
      m_client->Notification(XVDR_ERROR, m_client->GetLocalizedString(30065), "");
      break;
  }

  m_cond.Signal();

  m_client->Log(XVDR_ERROR, "%s - failed to set channel", __FUNCTION__);
  return true;
}

SignalStatus Demux::GetSignalStatus()
{
  MutexLock lock(&m_lock);
  return m_signal;
}

void Demux::StreamChange(ResponsePacket *resp)
{
  MutexLock lock(&m_lock);
  m_streams.clear();

  int index = 0;

  while (!resp->end())
  {
    uint32_t    id   = resp->extract_U32();
    const char* type = resp->extract_String();

    Stream stream;

    stream[stream_index] = index++;
    stream[stream_physicalid] = id;
    stream[stream_identifier] = -1;

    if(!strcmp(type, "AC3"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_AUDIO;
      stream[stream_codecid] = CODEC_ID_AC3;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_AUDIO;
      stream[stream_codecid] = CODEC_ID_MP2;
    }
    else if(!strcmp(type, "AAC"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_AUDIO;
      stream[stream_codecid] = CODEC_ID_AAC;
    }
    else if(!strcmp(type, "LATM"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_AUDIO;
      stream[stream_codecid] = CODEC_ID_AAC_LATM;
    }
    else if(!strcmp(type, "DTS"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_AUDIO;
      stream[stream_codecid] = CODEC_ID_DTS;
    }
    else if(!strcmp(type, "EAC3"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_AUDIO;
      stream[stream_codecid] = CODEC_ID_EAC3;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      stream[stream_codectype] = AVMEDIA_TYPE_VIDEO;
      stream[stream_codecid] = CODEC_ID_MPEG2VIDEO;
      stream[stream_fpsscale] = resp->extract_U32();
      stream[stream_fpsrate] = resp->extract_U32();
      stream[stream_height] = resp->extract_U32();
      stream[stream_width] = resp->extract_U32();
      stream[stream_aspect] = resp->extract_Double();
    }
    else if(!strcmp(type, "H264"))
    {
      stream[stream_codectype] = AVMEDIA_TYPE_VIDEO;
      stream[stream_codecid] = CODEC_ID_H264;
      stream[stream_fpsscale] = resp->extract_U32();
      stream[stream_fpsrate] = resp->extract_U32();
      stream[stream_height] = resp->extract_U32();
      stream[stream_width] = resp->extract_U32();
      stream[stream_aspect] = resp->extract_Double();
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      stream[stream_language] = resp->extract_String();
      stream[stream_codectype] = AVMEDIA_TYPE_SUBTITLE;
      stream[stream_codecid] = CODEC_ID_DVB_SUBTITLE;

      uint32_t composition_id = resp->extract_U32();
      uint32_t ancillary_id   = resp->extract_U32();

      stream[stream_identifier] = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
    }
    else if(!strcmp(type, "TELETEXT"))
    {
      stream[stream_codectype] = AVMEDIA_TYPE_SUBTITLE;
      stream[stream_codecid] = CODEC_ID_DVB_TELETEXT;
    }

    if (index > 16)
    {
      m_client->Log(XVDR_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }

    m_streams[id] = stream;
  }
}

void Demux::StreamStatus(ResponsePacket *resp)
{
  uint32_t status = resp->extract_U32();

  switch(status) {
    case XVDR_STREAM_STATUS_SIGNALLOST:
      m_client->Notification(XVDR_ERROR, m_client->GetLocalizedString(30047));
      break;
    case XVDR_STREAM_STATUS_SIGNALRESTORED:
      m_client->Notification(XVDR_INFO, m_client->GetLocalizedString(30048));
      SwitchChannel(m_channeluid);
      break;
    default:
      break;
  }
}

void Demux::StreamSignalInfo(ResponsePacket *resp)
{
  const char* name = resp->extract_String();
  const char* status = resp->extract_String();

  MutexLock lock(&m_lock);
  m_signal[signal_adaptername] = name;
  m_signal[signal_adapterstatus] = status;
  m_signal[signal_snr] = resp->extract_U32();
  m_signal[signal_strength] = resp->extract_U32();
  m_signal[signal_ber] = resp->extract_U32();
  m_signal[signal_unc] = resp->extract_U32();
}

bool Demux::StreamContentInfo(ResponsePacket *resp)
{
  MutexLock lock(&m_lock);
  StreamProperties old = m_streams;

  for (unsigned int i = 0; i < m_streams.size() && !resp->end(); i++)
  {
    uint32_t id = resp->extract_U32();

    if(m_streams.find(id) == m_streams.end())
      continue;

    Stream& stream = m_streams[id];

    const char* language    = NULL;
    uint32_t composition_id = 0;
    uint32_t ancillary_id   = 0;

    switch (stream[stream_codectype])
    {
      case AVMEDIA_TYPE_AUDIO:
    	stream[stream_language] = resp->extract_String();
        stream[stream_channels] = resp->extract_U32();
        stream[stream_samplerate] = resp->extract_U32();
        stream[stream_blockalign] = resp->extract_U32();
        stream[stream_bitrate] = resp->extract_U32();
        stream[stream_bitspersample] = resp->extract_U32();
        break;

      case AVMEDIA_TYPE_VIDEO:
        stream[stream_fpsscale] = resp->extract_U32();
        stream[stream_fpsrate] = resp->extract_U32();
        stream[stream_height] = resp->extract_U32();
        stream[stream_width] = resp->extract_U32();
        stream[stream_aspect] = resp->extract_Double();
        break;

      case AVMEDIA_TYPE_SUBTITLE:
      	stream[stream_language] = resp->extract_String();

        composition_id = resp->extract_U32();
        ancillary_id   = resp->extract_U32();

        stream[stream_identifier] = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
        break;

      default:
        break;
    }
  }

  return (old != m_streams);
}

void Demux::OnReconnect()
{
}

void Demux::SetPriority(int priority)
{
  if(priority < -1 || priority > 99)
    priority = 50;

  m_priority = priority;
}
