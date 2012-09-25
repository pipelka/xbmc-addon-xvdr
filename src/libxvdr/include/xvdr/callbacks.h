#pragma once

#include <stdint.h>
#include <time.h>
#include <string>
#include "xvdr/dataset.h"

#define XVDRLog(x...)                     XVDR::Callbacks::Get()->Log(x)
#define XVDRNotification(x...)            XVDR::Callbacks::Get()->Notification(x)
#define XVDRConvertToUTF8(t)              XVDR::Callbacks::Get()->ConvertToUTF8(t)
#define XVDRGetLocalizedString(id)        XVDR::Callbacks::Get()->GetLocalizedString(id)
#define XVDRAllocatePacket(s)             XVDR::Callbacks::Get()->AllocatePacket(s)
#define XVDRSetPacketData(p, d, id, dts, pts) XVDR::Callbacks::Get()->SetPacketData(p, d, id, dts, pts)
#define XVDRGetPacketPayload(p)           XVDR::Callbacks::Get()->GetPacketPayload(p)
#define XVDRFreePacket(p)                 XVDR::Callbacks::Get()->FreePacket(p)
#define XVDRGetLanguageCode()             XVDR::Callbacks::Get()->GetLanguageCode()
#define XVDRGetSetting(n, v)              XVDR::Callbacks::Get()->GetSetting(n, v)
#define XVDRTriggerChannelUpdate()        XVDR::Callbacks::Get()->TriggerChannelUpdate()
#define XVDRTriggerRecordingUpdate()      XVDR::Callbacks::Get()->TriggerRecordingUpdate()
#define XVDRTriggerTimerUpdate()          XVDR::Callbacks::Get()->TriggerTimerUpdate()
#define XVDRTransferChannelEntry(c)       XVDR::Callbacks::Get()->TransferChannelEntry(c)
#define XVDRTransferEpgEntry(t)           XVDR::Callbacks::Get()->TransferEpgEntry(t)
#define XVDRTransferTimerEntry(t)         XVDR::Callbacks::Get()->TransferTimerEntry(t)
#define XVDRTransferRecordingEntry(r)     XVDR::Callbacks::Get()->TransferRecordingEntry(r)
#define XVDRTransferChannelGroup(g)       XVDR::Callbacks::Get()->TransferChannelGroup(g)
#define XVDRTransferChannelGroupMember(m) XVDR::Callbacks::Get()->TransferChannelGroupMember(m)
#define XVDRRecording(l1, l2, on)         XVDR::Callbacks::Get()->Recording(l1, l2, on)
#define XVDRStreamChange(p)               XVDR::Callbacks::Get()->StreamChange(p)
#define XVDRContentInfo(p)                XVDR::Callbacks::Get()->ContentInfo(p)

#define XVDR_INFO    XVDR::Callbacks::INFO
#define XVDR_NOTICE  XVDR::Callbacks::NOTICE
#define XVDR_WARNING XVDR::Callbacks::WARNING
#define XVDR_ERROR   XVDR::Callbacks::ERROR
#define XVDR_DEBUG   XVDR::Callbacks::DEBUG


namespace XVDR {

typedef void Packet;

class Callbacks
{
public:

  typedef enum
  {
    INFO,
    NOTICE,
    WARNING,
    ERROR,
    DEBUG
  } LEVEL;

  Callbacks();

  virtual ~Callbacks();

  // log and notification

  virtual void Log(LEVEL level, const std::string& text, ...) = 0;

  virtual void Notification(LEVEL level, const std::string& text, ...) = 0;

  virtual void Recording(const std::string& line1, const std::string& line2, bool on) = 0;

  // localisation

  virtual void ConvertToUTF8(std::string& text) = 0;

  virtual std::string GetLanguageCode() = 0;

  virtual const char* GetLocalizedString(int id) = 0;

  // triggers

  virtual void TriggerChannelUpdate() = 0;

  virtual void TriggerRecordingUpdate() = 0;

  virtual void TriggerTimerUpdate() = 0;

  // data transfer functions

  virtual void TransferChannelEntry(const Channel& channel) = 0;

  virtual void TransferEpgEntry(const Epg& tag) = 0;

  virtual void TransferTimerEntry(const Timer& timer) = 0;

  virtual void TransferRecordingEntry(const RecordingEntry& rec) = 0;

  virtual void TransferChannelGroup(const ChannelGroup& group) = 0;

  virtual void TransferChannelGroupMember(const ChannelGroupMember& member) = 0;

  // packet allocation

  virtual Packet* AllocatePacket(int length) = 0;

  virtual uint8_t* GetPacketPayload(Packet* packet) = 0;

  virtual void SetPacketData(Packet* packet, uint8_t* data = NULL, int streamid = 0, uint64_t dts = 0, uint64_t pts = 0) = 0;

  virtual void FreePacket(Packet* p) = 0;

  virtual Packet* StreamChange(const StreamProperties& p) = 0;

  virtual Packet* ContentInfo(const StreamProperties& p) = 0;

  // static registration and access

  static void Register(Callbacks* callbacks);

  static Callbacks* Get();

private:

  static Callbacks* m_callbacks;

};

} // namespace XVDR
