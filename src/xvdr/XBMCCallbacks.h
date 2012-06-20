#pragma once

#include "XBMCAddon.h"
#include "XVDRCallbacks.h"
#include "XVDRThread.h"

class cXBMCCallbacks : public cXVDRCallbacks
{
public:

  cXBMCCallbacks();

  void Log(LEVEL level, const std::string& text, ...);

  void Notification(LEVEL level, const std::string& text, ...);

  void Recording(const std::string& line1, const std::string& line2, bool on);

  void ConvertToUTF8(std::string& text);

  std::string GetLanguageCode();

  const char* GetLocalizedString(int id);

  void TriggerChannelUpdate();

  void TriggerRecordingUpdate();

  void TriggerTimerUpdate();

  void SetHandle(PVR_HANDLE handle);

  void TransferChannelEntry(PVR_CHANNEL* channel);

  void TransferEpgEntry(EPG_TAG* tag);

  void TransferTimerEntry(PVR_TIMER* timer);

  void TransferRecordingEntry(PVR_RECORDING* rec);

  void TransferChannelGroup(PVR_CHANNEL_GROUP* group);

  void TransferChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* member);

  DemuxPacket* AllocatePacket(int s);

  void FreePacket(DemuxPacket* p);

private:

  PVR_HANDLE m_handle;
};
