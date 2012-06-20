#pragma once

#include <stdint.h>
#include <time.h>
#include <string>
#include "xbmc_pvr_types.h"

#define XVDRLog(x...)                     cXVDRCallbacks::Get()->Log(x)
#define XVDRNotification(x...)            cXVDRCallbacks::Get()->Notification(x)
#define XVDRConvertToUTF8(t)              cXVDRCallbacks::Get()->ConvertToUTF8(t)
#define XVDRGetLocalizedString(id)        cXVDRCallbacks::Get()->GetLocalizedString(id)
#define XVDRAllocatePacket(s)             cXVDRCallbacks::Get()->AllocatePacket(s)
#define XVDRSetPacketData(p, d, id, dts, pts) cXVDRCallbacks::Get()->SetPacketData(p, d, id, dts, pts)
#define XVDRGetPacketPayload(p)           cXVDRCallbacks::Get()->GetPacketPayload(p)
#define XVDRFreePacket(p)                 cXVDRCallbacks::Get()->FreePacket(p)
#define XVDRGetLanguageCode()             cXVDRCallbacks::Get()->GetLanguageCode()
#define XVDRGetSetting(n, v)              cXVDRCallbacks::Get()->GetSetting(n, v)
#define XVDRTriggerChannelUpdate()        cXVDRCallbacks::Get()->TriggerChannelUpdate()
#define XVDRTriggerRecordingUpdate()      cXVDRCallbacks::Get()->TriggerRecordingUpdate()
#define XVDRTriggerTimerUpdate()          cXVDRCallbacks::Get()->TriggerTimerUpdate()
#define XVDRTransferChannelEntry(c)       cXVDRCallbacks::Get()->TransferChannelEntry(c)
#define XVDRTransferEpgEntry(t)           cXVDRCallbacks::Get()->TransferEpgEntry(t)
#define XVDRTransferTimerEntry(t)         cXVDRCallbacks::Get()->TransferTimerEntry(t)
#define XVDRTransferRecordingEntry(r)     cXVDRCallbacks::Get()->TransferRecordingEntry(r)
#define XVDRTransferChannelGroup(g)       cXVDRCallbacks::Get()->TransferChannelGroup(g)
#define XVDRTransferChannelGroupMember(m) cXVDRCallbacks::Get()->TransferChannelGroupMember(m)
#define XVDRRecording(l1, l2, on)         cXVDRCallbacks::Get()->Recording(l1, l2, on)

#define XVDR_INFO    cXVDRCallbacks::INFO
#define XVDR_NOTICE  cXVDRCallbacks::NOTICE
#define XVDR_WARNING cXVDRCallbacks::WARNING
#define XVDR_ERROR   cXVDRCallbacks::ERROR
#define XVDR_DEBUG   cXVDRCallbacks::DEBUG

typedef void XVDRPacket;

class cXVDRCallbacks
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

  cXVDRCallbacks();

  virtual ~cXVDRCallbacks();

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

  virtual void TransferChannelEntry(PVR_CHANNEL* channel) = 0;

  virtual void TransferEpgEntry(EPG_TAG* tag) = 0;

  virtual void TransferTimerEntry(PVR_TIMER* timer) = 0;

  virtual void TransferRecordingEntry(PVR_RECORDING* rec) = 0;

  virtual void TransferChannelGroup(PVR_CHANNEL_GROUP* group) = 0;

  virtual void TransferChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* member) = 0;

  // packet allocation

  virtual XVDRPacket* AllocatePacket(int length) = 0;

  virtual uint8_t* GetPacketPayload(XVDRPacket* packet) = 0;

  virtual void SetPacketData(XVDRPacket* packet, uint8_t* data = NULL, int streamid = 0, uint64_t dts = 0, uint64_t pts = 0) = 0;

  virtual void FreePacket(XVDRPacket* p) = 0;

  // static registration and access

  static void Register(cXVDRCallbacks* callbacks);

  static cXVDRCallbacks* Get();

private:

  static cXVDRCallbacks* m_callbacks;

};
