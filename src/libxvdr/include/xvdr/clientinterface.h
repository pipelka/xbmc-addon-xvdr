#pragma once

#include <stdint.h>
#include <string>
#include "xvdr/dataset.h"

#define XVDR_INFO    XVDR::ClientInterface::INFO
#define XVDR_NOTICE  XVDR::ClientInterface::NOTICE
#define XVDR_WARNING XVDR::ClientInterface::WARNING
#define XVDR_ERROR   XVDR::ClientInterface::ERROR
#define XVDR_DEBUG   XVDR::ClientInterface::DEBUG

namespace XVDR {

typedef void Packet;

class ClientInterface
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

  ClientInterface();

  virtual ~ClientInterface();

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

  virtual void SetPacketData(Packet* packet, uint8_t* data = NULL, int streamid = 0, uint64_t dts = 0, uint64_t pts = 0) = 0;

  virtual void FreePacket(Packet* p) = 0;

  virtual Packet* StreamChange(const StreamProperties& p);

  virtual Packet* ContentInfo(const StreamProperties& p);

};

} // namespace XVDR
