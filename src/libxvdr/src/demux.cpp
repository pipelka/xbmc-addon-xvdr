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
#include "xvdr/msgpacket.h"
#include "xvdr/command.h"

using namespace XVDR;

Demux::Demux(ClientInterface* client) : Connection(client), m_priority(50), m_queuelocked(false), m_paused(false), m_timeshiftmode(false)
{
}

Demux::~Demux()
{
  // wait for pending requests
  MutexLock lock(&m_lock);
}

bool Demux::OpenChannel(const std::string& hostname, uint32_t channeluid)
{
  if(!Open(hostname))
    return false;

  m_paused = false;
  m_timeshiftmode = false;

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

         // request packets in timeshift mode
         if(m_timeshiftmode)
         {
           MsgPacket req(XVDR_CHANNELSTREAM_REQUEST, XVDR_CHANNEL_STREAM);
           if(!Session::TransmitMessage(&req))
             return NULL;
         }

         m_cond.Wait(1000);

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

void Demux::OnResponsePacket(MsgPacket *resp) {
  {
    MutexLock lock(&m_lock);
    if(m_queuelocked)
    	return;
  }

  if (resp->getType() != XVDR_CHANNEL_STREAM)
	return;

  Packet* pkt = NULL;
  int iStreamId = -1;

  switch (resp->getMsgID())
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
      uint16_t id = resp->get_U16();
      int64_t pts = resp->get_S64();
      int64_t dts = resp->get_S64();
      uint32_t length = resp->get_U32();
      uint8_t* payload = resp->consume(length);

      Stream& stream = m_streams[id];

      if(stream.PhysicalId != id) {
          m_client->Log(XVDR_DEBUG, "stream id %i not found", id);
          break;
      }

      pkt = m_client->AllocatePacket(length);
      m_client->SetPacketData(pkt, payload, stream.Index, dts, pts);
      break;
  }

  if(pkt != NULL) {
	  {
	    MutexLock lock(&m_lock);

	    // limit queue size
	    if(m_queue.size() > 200)
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

  MsgPacket vrp(XVDR_CHANNELSTREAM_OPEN);
  vrp.put_U32(channeluid);
  vrp.put_S32(m_priority);

  MsgPacket* vresp = ReadResult(&vrp);

  {
    MutexLock lock(&m_lock);
    m_queuelocked = false;
    m_paused = false;
    m_timeshiftmode = false;

    m_streams.clear();
  }

  uint32_t rc = XVDR_RET_ERROR;

  if(vresp != NULL)
	  rc = vresp->get_U32();

  delete vresp;

  if(rc == XVDR_RET_OK)
  {
    m_channeluid = channeluid;
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

void Demux::StreamChange(MsgPacket *resp)
{
  MutexLock lock(&m_lock);
  m_streams.clear();

  int index = 0;

  while (!resp->eop())
  {
    Stream stream;

    stream.Index = index++;
    stream.PhysicalId = resp->get_U32();
    stream.Type = resp->get_String();

    stream.Identifier = -1;

    if(stream.Type == "AC3")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_AUDIO;
      stream.CodecId = CODEC_ID_AC3;
    }
    else if(stream.Type == "MPEG2AUDIO")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_AUDIO;
      stream.CodecId = CODEC_ID_MP2;
    }
    else if(stream.Type == "AAC")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_AUDIO;
      stream.CodecId = CODEC_ID_AAC;
    }
    else if(stream.Type == "LATM")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_AUDIO;
      stream.CodecId = CODEC_ID_AAC_LATM;
    }
    else if(stream.Type == "DTS")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_AUDIO;
      stream.CodecId = CODEC_ID_DTS;
    }
    else if(stream.Type == "EAC3")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_AUDIO;
      stream.CodecId = CODEC_ID_EAC3;
    }
    else if(stream.Type == "MPEG2VIDEO")
    {
      stream.CodecType = AVMEDIA_TYPE_VIDEO;
      stream.CodecId = CODEC_ID_MPEG2VIDEO;
      stream.FpsScale = resp->get_U32();
      stream.FpsRate = resp->get_U32();
      stream.Height = resp->get_U32();
      stream.Width = resp->get_U32();
      stream.Aspect = (double)resp->get_S64() / 10000.0;
    }
    else if(stream.Type == "H264")
    {
      stream.CodecType = AVMEDIA_TYPE_VIDEO;
      stream.CodecId = CODEC_ID_H264;
      stream.FpsScale = resp->get_U32();
      stream.FpsRate = resp->get_U32();
      stream.Height = resp->get_U32();
      stream.Width = resp->get_U32();
      stream.Aspect = (double)resp->get_S64() / 10000.0;
    }
    else if(stream.Type == "DVBSUB")
    {
      stream.Language = resp->get_String();
      stream.CodecType = AVMEDIA_TYPE_SUBTITLE;
      stream.CodecId = CODEC_ID_DVB_SUBTITLE;

      uint32_t composition_id = resp->get_U32();
      uint32_t ancillary_id   = resp->get_U32();

      stream.Identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
    }
    else if(stream.Type == "TELETEXT")
    {
      stream.CodecType = AVMEDIA_TYPE_SUBTITLE;
      stream.CodecId = CODEC_ID_DVB_TELETEXT;
    }

    if (index > 16)
    {
      m_client->Log(XVDR_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }

    m_streams[stream.PhysicalId] = stream;
  }
}

void Demux::StreamStatus(MsgPacket *resp)
{
  uint32_t status = resp->get_U32();

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

void Demux::StreamSignalInfo(MsgPacket *resp)
{
  MutexLock lock(&m_lock);
  m_signal << resp;
}

bool Demux::StreamContentInfo(MsgPacket *resp)
{
  MutexLock lock(&m_lock);
  StreamProperties old = m_streams;

  for (unsigned int i = 0; i < m_streams.size() && !resp->eop(); i++)
  {
    uint32_t id = resp->get_U32();

    if(m_streams.find(id) == m_streams.end())
      continue;

    Stream& stream = m_streams[id];

    const char* language    = NULL;
    uint32_t composition_id = 0;
    uint32_t ancillary_id   = 0;

    switch (stream.CodecType)
    {
      case AVMEDIA_TYPE_AUDIO:
    	  stream.Language = resp->get_String();
        stream.Channels = resp->get_U32();
        stream.SampleRate = resp->get_U32();
        stream.BlockAlign = resp->get_U32();
        stream.BitRate = resp->get_U32();
        stream.BitsPerSample = resp->get_U32();
        break;

      case AVMEDIA_TYPE_VIDEO:
        stream.FpsScale = resp->get_U32();
        stream.FpsRate = resp->get_U32();
        stream.Height = resp->get_U32();
        stream.Width = resp->get_U32();
        stream.Aspect = (double)resp->get_S64() / 10000.0;
        break;

      case AVMEDIA_TYPE_SUBTITLE:
      	stream.Language = resp->get_String();

        composition_id = resp->get_U32();
        ancillary_id   = resp->get_U32();

        stream.Identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
        break;

      default:
        break;
    }
  }

  return (old != m_streams);
}

void Demux::OnDisconnect()
{
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

void Demux::Pause(bool on)
{
  MsgPacket req(XVDR_CHANNELSTREAM_PAUSE);
  req.put_U32(on);

  MsgPacket* vresp = ReadResult(&req);
  delete vresp;

  {
    MutexLock lock(&m_lock);

    if(!on && m_paused)
      m_timeshiftmode = true;

    m_paused = on;
  }

  m_cond.Signal();
}
