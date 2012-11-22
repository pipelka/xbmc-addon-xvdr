#include <stdlib.h>
#include <string.h>

#include "consoleclient.h"

ConsoleClient::ConsoleClient() : Connection(this), m_gotstreamprops(false) {
}

std::string ConsoleClient::GetLanguageCode() {
  return "de";
}

void ConsoleClient::TransferChannelEntry(const XVDR::Channel& channel) {
  m_channels[channel.Number] = channel;
  CondWait::SleepMs(10);
}

void ConsoleClient::TriggerChannelUpdate() {
  GetChannelsList();
}

void ConsoleClient::TriggerRecordingUpdate() {
  GetRecordingsList();
}

void ConsoleClient::TriggerTimerUpdate() {
  GetTimersList();
}

XVDR::Packet* ConsoleClient::StreamChange(const XVDR::StreamProperties& streams) {
  Log(INFO, "Received stream information:");
  for(XVDR::StreamProperties::const_iterator i = streams.begin(); i != streams.end(); i++) {
    Log(INFO, "Stream #%i %s (%s) PID: %i", i->second.Index, i->second.Type.c_str(), i->second.Language.c_str(), i->first);
  }
  return NULL;
}

/*XVDR::Packet* ConsoleClient::ContentInfo(const XVDR::StreamProperties& streams) {
  Log(INFO, "Got content info ...");
  m_gotstreamprops = true;
  return NULL;
}*/

XVDR::Packet* ConsoleClient::AllocatePacket(int length) {
  Packet* p = new Packet;

  if(length > 0) {
    p->data = (uint8_t*)malloc(length);
    p->length = length;
  }

  return (XVDR::Packet*)p;
}

void ConsoleClient::SetPacketData(XVDR::Packet* packet, uint8_t* data, int index, uint64_t pts, uint64_t dts) {
  Packet* p = (Packet*)packet;

  p->pts = pts;
  p->dts = dts;
  p->index = index;

  memcpy(p->data, data, p->length);
}

void ConsoleClient::FreePacket(XVDR::Packet* packet) {
  if(packet == NULL) {
    return;
  }

  Packet* p = (Packet*)packet;
  free(p->data);
  delete p;
}
