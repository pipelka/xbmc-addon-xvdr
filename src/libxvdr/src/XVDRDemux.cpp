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
#include <limits.h>
#include <string.h>
#include "codecids.h" // For codec id's
#include "XVDRDemux.h"
#include "XVDRCallbacks.h"
#include "XVDRResponsePacket.h"
#include "requestpacket.h"
#include "xvdrcommand.h"

cXVDRDemux::cXVDRDemux() : m_priority(50)
{
}

cXVDRDemux::~cXVDRDemux()
{
}

bool cXVDRDemux::OpenChannel(const std::string& hostname, uint32_t channeluid)
{
  if(!cXVDRSession::Open(hostname))
    return false;

  if(!cXVDRSession::Login())
    return false;

  return SwitchChannel(channeluid);
}

const cXVDRStreamProperties& cXVDRDemux::GetStreamProperties()
{
  return m_streams;
}

void cXVDRDemux::Abort()
{
  m_streams.clear();
  cXVDRSession::Abort();
}

XVDRPacket* cXVDRDemux::Read()
{
  if(ConnectionLost())
  {
    SleepMs(100);
    return XVDRAllocatePacket(0);
  }

  cXVDRResponsePacket *resp = ReadMessage();

  if(resp == NULL)
    return XVDRAllocatePacket(0);

  if (resp->getChannelID() != XVDR_CHANNEL_STREAM)
  {
    delete resp;
    return XVDRAllocatePacket(0);
  }

  XVDRPacket* pkt = NULL;
  int iStreamId = -1;

  switch (resp->getOpCodeID())
  {
    case XVDR_STREAM_CHANGE:
      StreamChange(resp);
      pkt = XVDRStreamChange(m_streams);
      delete resp;
      return pkt;

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

      pkt = XVDRContentInfo(m_streams);
      delete resp;
      return pkt;
      break;

    case XVDR_STREAM_MUXPKT:
      // figure out the stream for this packet
      int id = resp->getStreamID();
      cXVDRStream& stream = m_streams[id];

      if(stream[stream_physicalid] != id) {
          delete resp;
          XVDRLog(XVDR_DEBUG, "stream id %i not found", id);
    	  break;
      }

      int index = stream[stream_index];
      pkt = (XVDRPacket*)resp->getUserData();
      if (pkt != NULL)
        XVDRSetPacketData(pkt, NULL, index, resp->getDTS(), resp->getPTS());

      delete resp;
      return pkt;
  }

  delete resp;
  return XVDRAllocatePacket(0);
}

bool cXVDRDemux::SwitchChannel(uint32_t channeluid)
{
  XVDRLog(XVDR_DEBUG, "changing to channel %d (priority %i)", channeluid, m_priority);

  cRequestPacket vrp;
  uint32_t rc = 0;

  if (vrp.init(XVDR_CHANNELSTREAM_OPEN) && vrp.add_U32(channeluid) && vrp.add_S32(m_priority) && ReadSuccess(&vrp, rc))
  {
    m_channeluid = channeluid;
    m_streams.clear();

    return true;
  }

  switch (rc)
  {
    // active recording
    case XVDR_RET_RECRUNNING:
     XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30062));
      break;
    // all receivers busy
    case XVDR_RET_DATALOCKED:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30063));
      break;
    // encrypted channel
    case XVDR_RET_ENCRYPTED:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30066));
      break;
    // error on switching channel
    default:
    case XVDR_RET_ERROR:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30064));
      break;
    // invalid channel
    case XVDR_RET_DATAINVALID:
      XVDRNotification(XVDR_ERROR, XVDRGetLocalizedString(30065), "");
      break;
  }

  XVDRLog(XVDR_ERROR, "%s - failed to set channel", __FUNCTION__);
  return true;
}

const cXVDRSignalStatus& cXVDRDemux::GetSignalStatus()
{
  return m_signal;
}

void cXVDRDemux::StreamChange(cXVDRResponsePacket *resp)
{
  // TODO - locking
  m_streams.clear();
  int index = 0;

  while (!resp->end())
  {
    uint32_t    id   = resp->extract_U32();
    const char* type = resp->extract_String();

    cXVDRStream stream;

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
      XVDRLog(XVDR_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }

    m_streams[id] = stream;
  }
}

void cXVDRDemux::StreamStatus(cXVDRResponsePacket *resp)
{
  uint32_t status = resp->extract_U32();

  switch(status) {
    case XVDR_STREAM_STATUS_SIGNALLOST:
      XVDRNotification(XVDR_ERROR, XVDRGetLocalizedString(30047));
      break;
    case XVDR_STREAM_STATUS_SIGNALRESTORED:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30048));
      SwitchChannel(m_channeluid);
      break;
    default:
      break;
  }
}

void cXVDRDemux::StreamSignalInfo(cXVDRResponsePacket *resp)
{
  const char* name = resp->extract_String();
  const char* status = resp->extract_String();

  m_signal[signal_adaptername] = name;
  m_signal[signal_adapterstatus] = status;
  m_signal[signal_snr] = resp->extract_U32();
  m_signal[signal_strength] = resp->extract_U32();
  m_signal[signal_ber] = resp->extract_U32();
  m_signal[signal_unc] = resp->extract_U32();
}

bool cXVDRDemux::StreamContentInfo(cXVDRResponsePacket *resp)
{
  cXVDRStreamProperties old = m_streams;

  for (unsigned int i = 0; i < m_streams.size() && !resp->end(); i++)
  {
    uint32_t id = resp->extract_U32();

    if(m_streams.find(id) == m_streams.end())
      continue;

    cXVDRStream& stream = m_streams[id];

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

void cXVDRDemux::OnReconnect()
{
}

void cXVDRDemux::SetPriority(int priority)
{
  if(priority < -1 || priority > 99)
    priority = 50;

  m_priority = priority;
}
