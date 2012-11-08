#pragma once

#include <stdint.h>
#include <string>

#include "xvdr/dataset.h"
#include "xvdr/thread.h"

namespace XVDR {

typedef enum
{
  INFO,
  NOTICE,
  WARNING,
  FAILURE,
  DEBUG
} LOGLEVEL;

typedef void Packet;

class ClientInterface
{
public:

  ClientInterface();

  virtual ~ClientInterface();

  // log and notification

  void Log(LOGLEVEL level, const std::string& text, ...);

  void Notification(LOGLEVEL level, const std::string& text, ...);

  virtual void Recording(const std::string& line1, const std::string& line2, bool on);

  // connection lost notification

  virtual void OnDisconnect();

  virtual void OnReconnect();

  // tv signal lost notification

  virtual void OnSignalLost();

  virtual void OnSignalRestored();

  // localisation

  virtual std::string GetLanguageCode() = 0;

  // triggers

  virtual void TriggerChannelUpdate() = 0;

  virtual void TriggerRecordingUpdate() = 0;

  virtual void TriggerTimerUpdate() = 0;

  // data transfer functions

  virtual void TransferChannelEntry(const Channel& channel) = 0;

  virtual void TransferEpgEntry(const EpgItem& tag) = 0;

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

  // access locking

  void Lock();

  void Unlock();

protected:

  virtual void OnLog(LOGLEVEL level, const char* msg);

  virtual void OnNotification(LOGLEVEL level, const char* msg);

private:

  char* m_msg;

  Mutex m_msgmutex;

  Mutex m_mutex;
};

} // namespace XVDR
