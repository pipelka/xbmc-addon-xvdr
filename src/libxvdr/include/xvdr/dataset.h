#pragma once
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

#include <stdint.h>
#include <string>
#include <sstream>
#include <map>
#include <list>

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
  std::string ServiceReference;
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
  std::string ThumbNailPath;
  std::string IconPath;
};

RecordingEntry& operator<< (RecordingEntry& lhs, MsgPacket* rhs);


class RecordingCutMark {
public:

  RecordingCutMark();
  RecordingCutMark(MsgPacket* p);

  double      Fps;
  uint64_t    FrameBegin;
  uint64_t    FrameEnd;
  std::string Type;
  std::string Description;
};

RecordingCutMark& operator<< (RecordingCutMark& lhs, MsgPacket* rhs);


class RecordingEdl : public std::list<RecordingCutMark> {
};


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
  std::string ProviderName;
  std::string ServiceName;
};

SignalStatus& operator<< (SignalStatus& lhs, MsgPacket* rhs);


class Stream {
public:

  Stream();

  int         Index;
  int         Identifier;
  uint32_t    PhysicalId;
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
  std::string Content;
};

class StreamProperties : public std::map<uint32_t, Stream> {
};

bool operator==(Stream const& lhs, Stream const& rhs);


class ChannelScannerSetup {
public:

  typedef enum {
    LOGLEVEL_ERRORSONLY = 0,
    LOGLEVEL_1 = 1,
    LOGLEVEL_2 = 2,
    LOGLEVEL_DEFAULT = 3,
    LOGLEVEL_DEBUG = 4,
    LOGLEVEL_EXTENDEDDEBUG = 5
  } Verbosity;

  typedef enum {
    LOGTYPE_OFF = 0,
    LOGTYPE_STDOUT = 1,
    LOGTYPE_SYSLOG = 2
  } LogType;

  typedef enum {
    DVBTYPE_DVBT = 0,
    DVBTYPE_DVBC = 1,
    DVBTYPE_DVBS = 2,
    DVBTYPE_ATSC = 5
  } DVBType;

  typedef enum {
    DVBINVERSION_AUTO = 0,
    DVBINVERSION_OFF = 0,
    DVBINVERSION_ON = 1
  } DVBInversion;

  typedef enum {
    SYMBOLRATE_AUTO = 0,
    SYMBOLRATE_6900 = 1,
    SYMBOLRATE_6875 = 2,
    SYMBOLRATE_6111 = 3,
    SYMBOLRATE_6250 = 4,
    SYMBOLRATE_6790 = 5,
    SYMBOLRATE_6811 = 6,
    SYMBOLRATE_5900 = 7,
    SYMBOLRATE_5000 = 8,
    SYMBOLRATE_3450 = 9,
    SYMBOLRATE_4000 = 10,
    SYMBOLRATE_6950 = 11,
    SYMBOLRATE_7000 = 12,
    SYMBOLRATE_6952 = 13,
    SYMBOLRATE_5156 = 14,
    SYMBOLRATE_4583 = 15,
    SYMBOLRATE_ALL = 16
  } SymbolRate;

  typedef enum {
    QAM_AUTO = 0,
    QAM_64 = 1,
    QAM_128 = 2,
    QAM_256 = 3,
    QAM_ALL = 4
  } QAM;

  typedef enum {
    ATSCTYPE_VSB = 0,
    ATSCTYPE_QAM = 1,
    ATSCTYPE_BOTH = 2
  } ATSCType;

  typedef enum {
    FLAG_TV = 1,
    FLAG_RADIO = 2,
    FLAG_FTA = 4,
    FLAG_SCRAMBLED = 8,
    FLAG_HDTV = 16
  } Flags;

  Verbosity verbosity;
  LogType logtype;
  DVBType dvbtype;
  DVBInversion dvbt_inversion;
  DVBInversion dvbc_inversion;
  SymbolRate  dvbc_symbolrate;
  QAM dvbc_qam;
  uint16_t countryid;
  uint16_t satid;
  ATSCType atsc_type;
  uint32_t flags;
};

ChannelScannerSetup& operator<< (ChannelScannerSetup& lhs, MsgPacket* rhs);
MsgPacket& operator<< (MsgPacket& lhs, const ChannelScannerSetup& rhs);


class ChannelScannerListItem {
public:

  uint32_t id;
  std::string shortname;
  std::string fullname;
};

typedef std::map<uint16_t, ChannelScannerListItem> ChannelScannerList;

ChannelScannerListItem& operator<< (ChannelScannerListItem& lhs, MsgPacket* rhs);

ChannelScannerList& operator<< (ChannelScannerList& lhs, MsgPacket* rhs);


class ChannelScannerStatus {
public:

  typedef enum {
    UNKNOWN = 0,
    SCANNING = 1,
    STOPPED = 2,
    BUSY = 3
  } Status;

  Status status;
  int progress;
  uint16_t strength;
  int numChannels;
  int newChannels;
  std::string device;
  std::string transponder;
};

ChannelScannerStatus& operator<< (ChannelScannerStatus& lhs, MsgPacket* rhs);

} // namespace XVDR
