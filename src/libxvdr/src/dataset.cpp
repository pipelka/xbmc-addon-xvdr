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

#include "xvdr/dataset.h"
#include "xvdr/msgpacket.h"

using namespace XVDR;

EpgItem::EpgItem() : UID(0), BroadcastID(0), StartTime(0), EndTime(0), GenreType(0), GenreSubType(0), ParentalRating(0) {
}

EpgItem::EpgItem(MsgPacket* p) {
  (*this) << p;
}

EpgItem& XVDR::operator<< (EpgItem& lhs, MsgPacket* rhs) {
  lhs.UID = 0;
  lhs.BroadcastID = rhs->get_U32();
  lhs.StartTime = rhs->get_U32();
  lhs.EndTime = lhs.StartTime + rhs->get_U32();
  uint32_t content = rhs->get_U32();
  lhs.GenreType = content & 0xF0;
  lhs.GenreSubType = content & 0x0F;
  lhs.ParentalRating = rhs->get_U32();
  lhs.Title = rhs->get_String();
  lhs.PlotOutline = rhs->get_String();
  lhs.Plot = rhs->get_String();

  return lhs;
}


Channel::Channel() : UID(0), Number(0), EncryptionSystem(0), IsHidden(false), IsRadio(false) {
}

Channel::Channel(MsgPacket* p) {
  (*this) << p;
}

Channel& XVDR::operator<< (Channel& lhs, MsgPacket* rhs) {
  lhs.Number = rhs->get_U32();
  lhs.Name = rhs->get_String();
  lhs.UID = rhs->get_U32();
  lhs.EncryptionSystem = rhs->get_U32();
  lhs.IconPath = rhs->get_String();
  lhs.IsHidden = false;

  return lhs;
}


Timer::Timer() {
  Index = 0;
  EpgUID = 0;
  State = 0;
  Priority = 0;
  LifeTime = 0;
  ChannelUID = 0;
  StartTime = 0;
  EndTime = 0;
  FirstDay = 0;
  WeekDays = 0;
  IsRepeating = false;
}

Timer::Timer(MsgPacket* p) {
  (*this) << p;
}

Timer& XVDR::operator<< (Timer& lhs, MsgPacket* rhs) {
  lhs.Index = rhs->get_U32();

  // TIMER STATES:

  // FLAGS   DESCRIPTION
  // 0       timer disabled
  // 1       timer scheduled
  // 4       VPS enabled
  // 8       timer recording now
  // 1024    conflict warning
  // 2048    conflict error

  lhs.State = rhs->get_U32();
  lhs.Priority = rhs->get_U32();
  lhs.LifeTime = rhs->get_U32();
  lhs.ChannelUID = rhs->get_U32();
  lhs.StartTime = rhs->get_U32();
  lhs.EndTime = rhs->get_U32();
  lhs.FirstDay = rhs->get_U32();
  lhs.WeekDays = rhs->get_U32();
  lhs.IsRepeating = (lhs.WeekDays != 0);

  const char* title = rhs->get_String();

  char* p = (char*)strrchr(title, '~');
  if(p == NULL || *p == 0) {
    lhs.Title = title;
    lhs.Directory.clear();
  }
  else {
    const char* name = p + 1;

    p[0] = 0;
    const char* dir = title;

    // replace dir separators
    for(p = (char*)dir; *p != 0; p++)
      if(*p == '~') *p = '/';

    lhs.Title = name;
    lhs.Directory = dir;
  }

  return lhs;
}

MsgPacket& XVDR::operator<< (MsgPacket& lhs, const Timer& rhs) {

  std::string dir = rhs.Directory;
  while(dir[dir.size()-1] == '/' && dir.size() > 1)
    dir = dir.substr(0, dir.size()-1);

  std::string name = rhs.Title;
  std::string title;

  if(!dir.empty() && dir != "/")
    title = dir + "/";

  title += name;

  // replace dir separators
  for(std::string::iterator i = title.begin(); i != title.end(); i++)
    if(*i == '/') *i = '~';

  lhs.put_U32(rhs.Index);
  lhs.put_U32(rhs.State);
  lhs.put_U32(rhs.Priority);
  lhs.put_U32(rhs.LifeTime);
  lhs.put_U32(rhs.ChannelUID);
  lhs.put_U32(rhs.StartTime);
  lhs.put_U32(rhs.EndTime);
  lhs.put_U32(rhs.IsRepeating ? rhs.FirstDay : 0);
  lhs.put_U32(rhs.WeekDays);
  lhs.put_String(title.c_str());
  lhs.put_String("");

  return lhs;
}


RecordingEntry::RecordingEntry() {
  Time = 0;
  Duration = 0;
  LifeTime = 0;
  GenreType = 0;
  GenreSubType = 0;
  PlayCount = 0;
}

RecordingEntry::RecordingEntry(MsgPacket* p) {
  (*this) << p;
}

RecordingEntry& XVDR::operator<< (RecordingEntry& lhs, MsgPacket* rhs) {
  lhs.Time = rhs->get_U32();
  lhs.Duration = rhs->get_U32();
  lhs.Priority = rhs->get_U32();
  lhs.LifeTime = rhs->get_U32();
  lhs.ChannelName = rhs->get_String();
  lhs.Title = rhs->get_String();
  lhs.PlotOutline = rhs->get_String();
  lhs.Plot = rhs->get_String();
  lhs.Directory = rhs->get_String();
  lhs.Id = rhs->get_String();
  lhs.PlayCount = rhs->get_U32();

  uint32_t content = rhs->get_U32();
  lhs.GenreType = content & 0xF0;
  lhs.GenreSubType = content & 0x0F;

  lhs.ThumbNailPath = rhs->get_String();
  lhs.IconPath = rhs->get_String();

  return lhs;
}

ChannelGroup::ChannelGroup() : IsRadio(false) {
}

ChannelGroup::ChannelGroup(MsgPacket* p) : IsRadio(false) {
  (*this) << p;
}

ChannelGroup& XVDR::operator<< (ChannelGroup& lhs, MsgPacket* rhs) {
  lhs.Name = rhs->get_String();
  lhs.IsRadio = rhs->get_U8();

  return lhs;
}

ChannelGroupMember::ChannelGroupMember() : UID(0), Number(0) {
}

ChannelGroupMember::ChannelGroupMember(MsgPacket* p) {
  (*this) << p;
}

ChannelGroupMember& XVDR::operator<< (ChannelGroupMember& lhs, MsgPacket* rhs) {
  lhs.UID = rhs->get_U32();
  lhs.Number = rhs->get_U32();

  return lhs;
}

SignalStatus::SignalStatus() {
  SNR = 0;
  Strength = 0;
  BER = 0;
  UNC = 0;
}

SignalStatus::SignalStatus(MsgPacket* p) {
  (*this) << p;
}

SignalStatus& XVDR::operator<< (SignalStatus& lhs, MsgPacket* rhs) {
  lhs.AdapterName = rhs->get_String();
  lhs.AdapterStatus = rhs->get_String();
  lhs.SNR = rhs->get_U32();
  lhs.Strength = rhs->get_U32();
  lhs.BER = rhs->get_U32();
  lhs.UNC = rhs->get_U32();

  return lhs;
}

Stream::Stream() {
  Index = 0;
  Identifier = 0;
  PhysicalId = 0;
  CodecId = 0;
  CodecType = 0;
  FpsScale = 0;
  FpsRate = 0;
  Aspect = 0.0;
  Height = 0;
  Width = 0;
  Channels = 0;
  SampleRate = 0;
  BlockAlign = 0;
  BitRate = 0;
  BitsPerSample = 0;
}


bool XVDR::operator==(Stream const& lhs, Stream const& rhs) {
  return
    lhs.Index == rhs.Index &&
    lhs.Identifier == rhs.Identifier &&
    lhs.PhysicalId == rhs.PhysicalId &&
    lhs.CodecId == rhs.CodecId &&
    lhs.CodecType == rhs.CodecType &&
    lhs.Language == rhs.Language &&
    lhs.FpsScale == rhs.FpsScale &&
    lhs.FpsRate == rhs.FpsRate &&
    lhs.Aspect == rhs.Aspect &&
    lhs.Height == rhs.Height &&
    lhs.Width == rhs.Width &&
    lhs.Channels == rhs.Channels &&
    lhs.SampleRate == rhs.SampleRate &&
    lhs.BlockAlign == rhs.BlockAlign &&
    lhs.BitRate == rhs.BitRate &&
    lhs.BitsPerSample == rhs.BitsPerSample;
}
