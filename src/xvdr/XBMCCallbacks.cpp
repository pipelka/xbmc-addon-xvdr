#include "XBMCCallbacks.h"
#include "XBMCAddon.h"

#include <stdio.h>
#include <stdarg.h>
#include "DVDDemuxPacket.h"

using namespace ADDON;

cXBMCCallbacks::cXBMCCallbacks() : m_handle(NULL)
{
}

void cXBMCCallbacks::SetHandle(ADDON_HANDLE handle)
{
  m_handle = handle;
}

void cXBMCCallbacks::Log(LEVEL level, const std::string& text, ...)
{
  addon_log_t lt;
  switch(level)
  {
    case DEBUG:
      lt = LOG_DEBUG;
      break;
    default:
    case INFO:
      lt = LOG_INFO;
      break;
    case NOTICE:
      lt = LOG_NOTICE;
      break;
    case ERROR:
      lt = LOG_ERROR;
      break;
  }

  va_list ap;
  va_start(ap, &text);

  char msg[512];
  vsnprintf(msg, sizeof(msg), text.c_str(), ap);
  va_end(ap);

  XBMC->Log(lt, msg);
}

void cXBMCCallbacks::Notification(LEVEL level, const std::string& text, ...)
{
  queue_msg qm;
  switch(level)
  {
    default:
    case INFO:
      qm = QUEUE_INFO;
      break;
    case ERROR:
      qm = QUEUE_ERROR;
      break;
    case WARNING:
      qm = QUEUE_WARNING;
      break;
  }

  va_list ap;
  va_start(ap, &text);

  char msg[512];
  vsnprintf(msg, sizeof(msg), text.c_str(), ap);
  va_end(ap);

  XBMC->QueueNotification(qm, msg);
}

void cXBMCCallbacks::Recording(const std::string& line1, const std::string& line2, bool on)
{
  PVR->Recording(line1.c_str(), line2.c_str(), on);
}

void cXBMCCallbacks::ConvertToUTF8(std::string& text)
{
  XBMC->UnknownToUTF8(text);
}

std::string cXBMCCallbacks::GetLanguageCode()
{
  const char* code = XBMC->GetDVDMenuLanguage();
  return (code == NULL) ? "" : code;
}

const char* cXBMCCallbacks::GetLocalizedString(int id)
{
  return XBMC->GetLocalizedString(id);
}

void cXBMCCallbacks::TriggerChannelUpdate()
{
  PVR->TriggerChannelUpdate();
}

void cXBMCCallbacks::TriggerRecordingUpdate()
{
  PVR->TriggerRecordingUpdate();
}

void cXBMCCallbacks::TriggerTimerUpdate()
{
  PVR->TriggerTimerUpdate();
}

void cXBMCCallbacks::TransferChannelEntry(PVR_CHANNEL* channel)
{
  PVR->TransferChannelEntry(m_handle, channel);
}

void cXBMCCallbacks::TransferEpgEntry(EPG_TAG* tag)
{
  PVR->TransferEpgEntry(m_handle, tag);
}

void cXBMCCallbacks::TransferTimerEntry(PVR_TIMER* timer)
{
  PVR->TransferTimerEntry(m_handle, timer);
}

void cXBMCCallbacks::TransferRecordingEntry(PVR_RECORDING* rec)
{
  PVR->TransferRecordingEntry(m_handle, rec);
}

void cXBMCCallbacks::TransferChannelGroup(PVR_CHANNEL_GROUP* group)
{
  PVR->TransferChannelGroup(m_handle, group);
}

void cXBMCCallbacks::TransferChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* member)
{
  PVR->TransferChannelGroupMember(m_handle, member);
}

XVDRPacket* cXBMCCallbacks::AllocatePacket(int s)
{
  DemuxPacket* d = PVR->AllocateDemuxPacket(s);
  if(d == NULL)
    return NULL;

  d->iSize = s;
  return (XVDRPacket*)d;
}

uint8_t* cXBMCCallbacks::GetPacketPayload(XVDRPacket* packet) {
  if (packet == NULL)
    return NULL;

  return static_cast<DemuxPacket*>(packet)->pData;
}

void cXBMCCallbacks::SetPacketData(XVDRPacket* packet, uint8_t* data, int streamid, uint64_t dts, uint64_t pts)
{
  if (packet == NULL)
    return;

  DemuxPacket* d = static_cast<DemuxPacket*>(packet);

  d->iStreamId = streamid;
  d->duration  = 0; //(double)resp->getDuration() * DVD_TIME_BASE / 1000000;
  d->dts       = (double)dts * DVD_TIME_BASE / 1000000;
  d->pts       = (double)pts * DVD_TIME_BASE / 1000000;

  if (data != NULL)
    memcpy(d->pData, data, d->iSize);
}

void cXBMCCallbacks::FreePacket(XVDRPacket* packet)
{
  PVR->FreeDemuxPacket((DemuxPacket*)packet);
}
