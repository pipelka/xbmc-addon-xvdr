#include "XBMCCallbacks.h"
#include "XBMCAddon.h"

#include <stdio.h>
#include <stdarg.h>

using namespace ADDON;

cXBMCCallbacks::cXBMCCallbacks() : m_handle(NULL)
{
}

void cXBMCCallbacks::SetHandle(PVR_HANDLE handle)
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

bool cXBMCCallbacks::GetSetting(const std::string& setting, void* value)
{
  return XBMC->GetSetting(setting.c_str(), value);
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

DemuxPacket* cXBMCCallbacks::AllocatePacket(int s)
{
  return PVR->AllocateDemuxPacket(s);
}

void cXBMCCallbacks::FreePacket(DemuxPacket* p)
{
  PVR->FreeDemuxPacket(p);
}
