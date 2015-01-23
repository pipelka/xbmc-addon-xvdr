/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2012 Alexander Pipelka
 *
 *      https://github.com/pipelka/xbmc-addon-xvdr
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
 *  along with this program; if not, write to the Free Software
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "xvdr/demux.h"
#include "xvdr/msgpacket.h"
#include "xvdr/command.h"

using namespace XVDR;

Demux::Demux(ClientInterface* client, PacketBuffer* buffer) : Connection(client), m_priority(50),
    m_paused(false), m_timeshiftmode(false), m_channeluid(0), m_buffer(buffer),
    m_iframestart(false)
{
}

Demux::~Demux()
{
  // wait for pending requests
  MutexLock lock(&m_lock);

  if (m_buffer != NULL) {
    delete m_buffer;
  }
}

Demux::SwitchStatus Demux::OpenChannel(const std::string& hostname, uint32_t channeluid, const std::string& clientname)
{
  if(!Open(hostname, clientname))
    return SC_ERROR;

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

  if (m_buffer != NULL) {
    m_buffer->clear();
  }

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
  if(ConnectionLost() || Aborting()) {
    return NULL;
  }

  Packet* p = NULL;
  MsgPacket* pkt = NULL;
  m_lock.Lock();

  if (m_buffer != NULL) {
    pkt = m_buffer->get();

    if (pkt == NULL) {
      m_lock.Unlock();
      m_cond.Wait(100);
      m_lock.Lock();
      pkt = m_buffer->get();
    }

    if (pkt != NULL) {
      pkt->rewind();

      switch (pkt->getMsgID()) {
      case XVDR_STREAM_MUXPKT: {
        uint16_t id = pkt->get_U16();
        int64_t pts = pkt->get_S64();
        int64_t dts = pkt->get_S64();
        uint32_t duration = pkt->get_U32();
        uint32_t length = pkt->get_U32();
        uint8_t* payload = pkt->consume(length);
        Stream& stream = m_streams[id];

        if (stream.PhysicalId != id) {
          m_client->Log(DEBUG, "stream id %i not found", id);
        } else {
          p = m_client->AllocatePacket(length);
          m_client->SetPacketData(p, payload, stream.Index, dts, pts, duration);
        }
        break;
      }
      case XVDR_STREAM_CHANGE: {
        StreamChange(pkt);
        p = m_client->StreamChange(m_streams);
        break;
      }
      }
    }

    if (p == NULL) {
      p = m_client->AllocatePacket(0);
    }

    m_buffer->release(pkt);

    m_lock.Unlock();
    return p;
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

bool Demux::OnResponsePacket(MsgPacket *resp) {
  if (resp->getType() != XVDR_CHANNEL_STREAM)
    return false;

  Packet* pkt = NULL;
  int iStreamId = -1;

  switch (resp->getMsgID())
  {
    case XVDR_STREAM_DETACH:
      m_client->OnDetach();
      Abort();
      break;

    case XVDR_STREAM_CHANGE:
      if (m_buffer != NULL) {
        m_lock.Lock();
        m_buffer->put(resp);
        m_lock.Unlock();
        m_cond.Signal();
        return true;
      }

      StreamChange(resp);
      pkt = m_client->StreamChange(m_streams);
      break;

    case XVDR_STREAM_STATUS:
      StreamStatus(resp);
      break;

    case XVDR_STREAM_SIGNALINFO:
      StreamSignalInfo(resp);
      break;

    case XVDR_STREAM_MUXPKT:
      {
        if (m_buffer != NULL) {
          m_lock.Lock();
          m_buffer->put(resp);
          m_lock.Unlock();
          m_cond.Signal();
          return true;
        }

        // figure out the stream for this packet
        uint16_t id = resp->get_U16();
        Stream& stream = m_streams[id];

        if(stream.PhysicalId != id) {
            m_client->Log(DEBUG, "stream id %i not found", id);
            break;
        }

        int64_t pts = resp->get_S64();
        int64_t dts = resp->get_S64();
        uint32_t duration = resp->get_U32();
        uint32_t length = resp->get_U32();
        uint8_t* payload = resp->consume(length);
        pkt = m_client->AllocatePacket(length);
        m_client->SetPacketData(pkt, payload, stream.Index, dts, pts, duration);
      }
      break;

    // discard unknown packet types
    default:
      pkt = NULL;
      return false;
  }

  if(pkt != NULL) {
	  {
	    MutexLock lock(&m_lock);
	    m_queue.push(pkt);

	    // limit queue size
	    while(m_queue.size() > 50)
	    {
	      pkt = m_queue.front();
	      m_queue.pop();
	      m_client->FreePacket(pkt);
	    }
	  }
	  m_cond.Signal();
  }

  return false;
}

Demux::SwitchStatus Demux::SwitchChannel(uint32_t channeluid)
{
  m_client->Log(DEBUG, "changing to channel %d (priority %i)", channeluid, m_priority);

  CleanupPacketQueue();

  {
    MutexLock lock(&m_lock);
    m_streams.clear();
  }

  m_cond.Signal();

  MsgPacket vrp(XVDR_CHANNELSTREAM_OPEN);
  vrp.put_U32(channeluid);
  vrp.put_S32(m_priority);
  vrp.put_U8(m_iframestart);

  MsgPacket* vresp = ReadResult(&vrp);

  m_paused = false;
  m_timeshiftmode = false;

  SwitchStatus status = SC_OK;

  if(vresp != NULL)
    status = (SwitchStatus)vresp->get_U32();

  delete vresp;

  if(status == SC_OK)
  {
    m_channeluid = channeluid;
    m_client->Log(INFO, "sucessfully switched channel");
  }
  else
    m_client->Log(FAILURE, "%s - failed to set channel (status: %i)", __FUNCTION__, status);

  m_cond.Signal();

  return status;
}

SignalStatus Demux::GetSignalStatus()
{
  MutexLock lock(&m_lock);
  return m_signal;
}

void Demux::GetContentFromType(const std::string& type, std::string& content) {
  if(type == "AC3") {
    content = "AUDIO";
  }
  else if(type == "MPEG2AUDIO") {
    content = "AUDIO";
  }
  else if(type == "AAC") {
    content = "AUDIO";
  }
  else if(type == "EAC3") {
    content = "AUDIO";
  }
  else if(type == "MPEG2VIDEO") {
    content = "VIDEO";
  }
  else if(type == "H264") {
    content = "VIDEO";
  }
  else if(type == "DVBSUB") {
    content = "SUBTITLE";
  }
  else if(type == "TELETEXT") {
    content = "TELETEXT";
  }
  else {
    content = "UNKNOWN";
  }
}

void Demux::StreamChange(MsgPacket *resp)
{
  MutexLock lock(&m_lock);
  m_streams.clear();

  int index = 0;
  uint32_t composition_id;
  uint32_t ancillary_id;

  while (!resp->eop())
  {
    Stream stream;

    stream.Index = index++;
    stream.PhysicalId = resp->get_U32();
    stream.Type = resp->get_String();

    GetContentFromType(stream.Type, stream.Content);

    stream.Identifier = -1;

    if(stream.Content == "AUDIO") {
      stream.Language = resp->get_String();
      stream.Channels = resp->get_U32();
      stream.SampleRate = resp->get_U32();
      stream.BlockAlign = resp->get_U32();
      stream.BitRate = resp->get_U32();
      stream.BitsPerSample = resp->get_U32();
    }
    else if(stream.Content == "VIDEO") {
      stream.FpsScale = resp->get_U32();
      stream.FpsRate = resp->get_U32();
      stream.Height = resp->get_U32();
      stream.Width = resp->get_U32();
      stream.Aspect = (double)resp->get_S64() / 10000.0;
    }
    else if(stream.Content == "SUBTITLE") {
      stream.Language = resp->get_String();
      composition_id = resp->get_U32();
      ancillary_id   = resp->get_U32();
      stream.Identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
    }

    if (index > 16)
    {
      m_client->Log(FAILURE, "%s - max amount of streams reached", __FUNCTION__);
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
      m_client->OnSignalLost();
      break;
    case XVDR_STREAM_STATUS_SIGNALRESTORED:
      m_client->OnSignalRestored();
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
  if (m_buffer != NULL) {
    return;
  }

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

void Demux::RequestSignalInfo()
{
  if(!m_lastsignal.TimedOut())
    return;

  MsgPacket req(XVDR_CHANNELSTREAM_SIGNAL);
  Session::TransmitMessage(&req);

  // signal status timeout
  m_lastsignal.Set(5000);
}

bool Demux::CanSeekStream() {
  return m_buffer != NULL;
}

bool Demux::SeekTime(int time, bool backwards, double *startpts) {
  MutexLock lock(&m_lock);

  if (m_buffer == NULL) {
    return false;
  }

  return m_buffer->seek(time, backwards, startpts);
}

void Demux::SetStartWithIFrame(bool on) {
  m_iframestart = on;
}
