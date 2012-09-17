#pragma once

#include "XBMCAddon.h"
#include "XVDRCallbacks.h"
#include "XVDRThread.h"

#include "xbmc_pvr_types.h"

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

  void SetHandle(ADDON_HANDLE handle);

  void TransferChannelEntry(const cXVDRChannel& channel);

  void TransferEpgEntry(const cXVDREpg& tag);

  void TransferTimerEntry(const cXVDRTimer& timer);

  void TransferRecordingEntry(const cXVDRRecordingEntry& rec);

  void TransferChannelGroup(const cXVDRChannelGroup& group);

  void TransferChannelGroupMember(const cXVDRChannelGroupMember& member);

  XVDRPacket* AllocatePacket(int length);

  uint8_t* GetPacketPayload(XVDRPacket* packet);

  void SetPacketData(XVDRPacket* packet, uint8_t* data = NULL, int streamid = 0, uint64_t dts = 0, uint64_t pts = 0);

  void FreePacket(XVDRPacket* packet);

  XVDRPacket* StreamChange(const cXVDRStreamProperties& p);

  XVDRPacket* ContentInfo(const cXVDRStreamProperties& p);

private:

  ADDON_HANDLE m_handle;
};

PVR_CHANNEL& operator<< (PVR_CHANNEL& lhs, const cXVDRChannel& rhs);

EPG_TAG& operator<< (EPG_TAG& lhs, const cXVDREpg& rhs);

cXVDRTimer& operator<< (cXVDRTimer& lhs, const PVR_TIMER& rhs);

PVR_TIMER& operator<< (PVR_TIMER& lhs, const cXVDRTimer& rhs);

cXVDRRecordingEntry& operator<< (cXVDRRecordingEntry& lhs, const PVR_RECORDING& rhs);

PVR_RECORDING& operator<< (PVR_RECORDING& lhs, const cXVDRRecordingEntry& rhs);

PVR_CHANNEL_GROUP& operator<< (PVR_CHANNEL_GROUP& lhs, const cXVDRChannelGroup& rhs);

PVR_CHANNEL_GROUP_MEMBER& operator<< (PVR_CHANNEL_GROUP_MEMBER& lhs, const cXVDRChannelGroupMember& rhs);

PVR_STREAM_PROPERTIES& operator<< (PVR_STREAM_PROPERTIES& lhs, const cXVDRStreamProperties& rhs);

PVR_STREAM_PROPERTIES::PVR_STREAM& operator<< (PVR_STREAM_PROPERTIES::PVR_STREAM& lhs, const cXVDRStream& rhs);

PVR_SIGNAL_STATUS& operator<< (PVR_SIGNAL_STATUS& lhs, const cXVDRSignalStatus& rhs);
