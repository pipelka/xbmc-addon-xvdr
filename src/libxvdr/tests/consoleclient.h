#ifndef CONSOLECLIENT_H
#define CONSOLECLIENT_H

#include "xvdr/clientinterface.h"
#include <map>

using namespace XVDR;

class ConsoleClient : public ClientInterface {
public:

  std::string GetLanguageCode();

  void TriggerChannelUpdate() {}
  void TriggerRecordingUpdate() {}
  void TriggerTimerUpdate() {}

  void TransferChannelEntry(const XVDR::Channel& channel) {
    m_channels[channel.Number] = channel;
  }

  void TransferEpgEntry(const XVDR::EpgItem&) {}
  void TransferTimerEntry(const XVDR::Timer&) {}
  void TransferRecordingEntry(const XVDR::RecordingEntry&) {}
  void TransferChannelGroup(const XVDR::ChannelGroup&) {}
  void TransferChannelGroupMember(const XVDR::ChannelGroupMember&) {}

  XVDR::Packet* StreamChange(const XVDR::StreamProperties& streams);

  XVDR::Packet* AllocatePacket(int length);
  void SetPacketData(XVDR::Packet* packet, uint8_t* data, int index, uint64_t pts, uint64_t dts);
  void FreePacket(XVDR::Packet* packet);

  std::map<int, XVDR::Channel> m_channels;

  struct Packet {
    Packet() : pts(0), dts(0), index(-1), data(NULL), length(0) {}

    uint64_t pts;
    uint64_t dts;
    int index;
    int pid;
    uint8_t* data;
    int length;
  };
};

#endif // CONSOLECLIENT_H
