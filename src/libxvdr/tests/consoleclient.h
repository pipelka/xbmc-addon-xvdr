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

#ifndef CONSOLECLIENT_H
#define CONSOLECLIENT_H

#include "xvdr/clientinterface.h"
#include "xvdr/connection.h"
#include <map>

using namespace XVDR;

class ConsoleClient : public ClientInterface, public Connection {
public:

  ConsoleClient();

  std::string GetLanguageCode();

  void TriggerChannelUpdate();
  void TriggerRecordingUpdate();
  void TriggerTimerUpdate();

  void TransferChannelEntry(const XVDR::Channel& channel);

  void TransferEpgEntry(const XVDR::EpgItem&) {}
  void TransferTimerEntry(const XVDR::Timer&) {}
  void TransferRecordingEntry(const XVDR::RecordingEntry&) {}
  void TransferChannelGroup(const XVDR::ChannelGroup&) {}
  void TransferChannelGroupMember(const XVDR::ChannelGroupMember&) {}

  XVDR::Packet* StreamChange(const XVDR::StreamProperties& streams);

  XVDR::Packet* AllocatePacket(int length);
  void SetPacketData(XVDR::Packet* packet, uint8_t* data, int index, uint64_t pts, uint64_t dts, uint32_t duration);
  void FreePacket(XVDR::Packet* packet);

  std::map<int, XVDR::Channel> m_channels;

  struct Packet {
    Packet() : pts(0), dts(0), index(-1), data(NULL), length(0) {}

    uint64_t pts;
    uint64_t dts;
    uint32_t duration;
    int index;
    int pid;
    uint8_t* data;
    int length;
  };

};

#endif // CONSOLECLIENT_H
