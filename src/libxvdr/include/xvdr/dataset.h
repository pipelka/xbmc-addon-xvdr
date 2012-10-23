#pragma once
/*
 *      Copyright (C) 2012 Alexander Pipelka
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdint.h>
#include <string>
#include <sstream>
#include <map>

class MsgPacket;

namespace XVDR {

class EpgItem {
public:

  EpgItem();
  EpgItem(MsgPacket* p);

  uint32_t    UID;
  uint32_t    BroadcastID;
  uint32_t    StartTime;
  uint32_t    EndTime;
  uint8_t     GenreType;
  uint8_t     GenreSubType;
  uint32_t    ParentalRating;
  std::string Title;
  std::string PlotOutline;
  std::string Plot;
};

EpgItem& operator<< (EpgItem& lhs, MsgPacket* rhs);


class Channel {
public:

  Channel();
  Channel(MsgPacket* p);

  uint32_t    UID;
  std::string Name;
  int         Number;
  uint32_t    EncryptionSystem;
  std::string IconPath;
  bool        IsHidden;
  bool        IsRadio;
};

Channel& operator<< (Channel& lhs, MsgPacket* rhs);


class Timer {
public:

  Timer();
  Timer(MsgPacket* p);

  uint32_t    Index;
  uint32_t    EpgUID;
  uint32_t    State;
  uint8_t     Priority;
  uint8_t     LifeTime;
  uint32_t    ChannelUID;
  uint32_t    StartTime;
  uint32_t    EndTime;
  uint8_t     FirstDay;
  uint8_t     WeekDays;
  bool        IsRepeating;
  std::string Title;
  std::string Directory;
  std::string Summary;
};

Timer& operator<< (Timer& lhs, MsgPacket* rhs);
MsgPacket& operator<< (MsgPacket& lhs, const Timer& rhs);


class RecordingEntry {
public:

  RecordingEntry();
  RecordingEntry(MsgPacket* p);

  uint32_t    Time;
  uint32_t    Duration;
  uint8_t     Priority;
  uint8_t     LifeTime;
  std::string ChannelName;
  std::string Title;
  std::string PlotOutline;
  std::string Plot;
  std::string Directory;
  std::string Id;
  uint8_t     GenreType;
  uint8_t     GenreSubType;
  uint8_t     PlayCount;
};

RecordingEntry& operator<< (RecordingEntry& lhs, MsgPacket* rhs);


class ChannelGroup {
public:

  ChannelGroup();
  ChannelGroup(MsgPacket* p);

  std::string Name;
  bool        IsRadio;
};

ChannelGroup& operator<< (ChannelGroup& lhs, MsgPacket* rhs);


class ChannelGroupMember {
public:

  ChannelGroupMember();
  ChannelGroupMember(MsgPacket* p);

  std::string Name;
  uint32_t    UID;
  uint32_t    Number;
};

ChannelGroupMember& operator<< (ChannelGroupMember& lhs, MsgPacket* rhs);


class SignalStatus {
public:

  SignalStatus();
  SignalStatus(MsgPacket* p);

  std::string AdapterName;
  std::string AdapterStatus;
  uint32_t    SNR;
  uint32_t    Strength;
  uint32_t    BER;
  uint32_t    UNC;
};

SignalStatus& operator<< (SignalStatus& lhs, MsgPacket* rhs);


class Stream {
public:

  Stream();

  int         Index;
  int         Identifier;
  uint32_t    PhysicalId;
  int         CodecId;
  int         CodecType;
  std::string Language;
  uint32_t    FpsScale;
  uint32_t    FpsRate;
  double      Aspect;
  uint32_t    Height;
  uint32_t    Width;
  uint32_t    Channels;
  uint32_t    SampleRate;
  uint32_t    BlockAlign;
  uint32_t    BitRate;
  uint32_t    BitsPerSample;
  std::string Type;
};

class StreamProperties : public std::map<uint32_t, Stream> {
};

bool operator==(Stream const& lhs, Stream const& rhs);

} // namespace XVDR
