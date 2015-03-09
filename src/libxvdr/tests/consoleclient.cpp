/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2012 Alexander Pipelka
 *
 *      https://github.com/pipelka/xbmc-addon-xvdr
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdlib.h>
#include <string.h>

#include "consoleclient.h"

ConsoleClient::ConsoleClient() : Connection(this) {
}

std::string ConsoleClient::GetLanguageCode() {
  return "de";
}

void ConsoleClient::TransferChannelEntry(const XVDR::Channel& channel) {
  m_channels[channel.Number] = channel;
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

  return AllocatePacket(0);
}

XVDR::Packet* ConsoleClient::AllocatePacket(int length) {
  Packet* p = new Packet;

  if(length > 0) {
    p->data = (uint8_t*)malloc(length);
    p->length = length;
  }

  return (XVDR::Packet*)p;
}

void ConsoleClient::SetPacketData(XVDR::Packet* packet, uint8_t* data, int index, uint64_t pts, uint64_t dts, uint32_t duration) {
  Packet* p = (Packet*)packet;

  p->pts = pts;
  p->dts = dts;
  p->duration = duration;
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
