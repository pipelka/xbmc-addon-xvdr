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

#include "XBMCAddon.h"
#include "xvdr/clientinterface.h"

#include "xbmc_pvr_types.h"

class cXBMCCallbacks : public XVDR::ClientInterface
{
public:

  cXBMCCallbacks();

  ~cXBMCCallbacks();

  void OnLog(XVDR::LOGLEVEL level, const char* msg);

  void OnNotification(XVDR::LOGLEVEL level, const char* msg);

  void Recording(const std::string& line1, const std::string& line2, bool on);

  void OnDisconnect();

  void OnReconnect();

  void OnSignalLost();

  void OnSignalRestored();

  std::string GetLanguageCode();

  void TriggerChannelUpdate();

  void TriggerRecordingUpdate();

  void TriggerTimerUpdate();

  void SetHandle(ADDON_HANDLE handle);

  void TransferChannelEntry(const XVDR::Channel& channel);

  void TransferEpgEntry(const XVDR::EpgItem& tag);

  void TransferTimerEntry(const XVDR::Timer& timer);

  void TransferRecordingEntry(const XVDR::RecordingEntry& rec);

  void TransferChannelGroup(const XVDR::ChannelGroup& group);

  void TransferChannelGroupMember(const XVDR::ChannelGroupMember& member);

  XVDR::Packet* AllocatePacket(int length);

  void SetPacketData(XVDR::Packet* packet, uint8_t* data = NULL, int streamid = 0, uint64_t dts = 0, uint64_t pts = 0);

  void FreePacket(XVDR::Packet* packet);

  XVDR::Packet* StreamChange(const XVDR::StreamProperties& p);

  XVDR::Packet* ContentInfo(const XVDR::StreamProperties& p);

private:

  ADDON_HANDLE m_handle;
};

PVR_CHANNEL& operator<< (PVR_CHANNEL& lhs, const XVDR::Channel& rhs);

EPG_TAG& operator<< (EPG_TAG& lhs, const XVDR::EpgItem& rhs);

XVDR::Timer& operator<< (XVDR::Timer& lhs, const PVR_TIMER& rhs);

PVR_TIMER& operator<< (PVR_TIMER& lhs, const XVDR::Timer& rhs);

XVDR::RecordingEntry& operator<< (XVDR::RecordingEntry& lhs, const PVR_RECORDING& rhs);

PVR_RECORDING& operator<< (PVR_RECORDING& lhs, const XVDR::RecordingEntry& rhs);

PVR_CHANNEL_GROUP& operator<< (PVR_CHANNEL_GROUP& lhs, const XVDR::ChannelGroup& rhs);

PVR_CHANNEL_GROUP_MEMBER& operator<< (PVR_CHANNEL_GROUP_MEMBER& lhs, const XVDR::ChannelGroupMember& rhs);

PVR_STREAM_PROPERTIES& operator<< (PVR_STREAM_PROPERTIES& lhs, const XVDR::StreamProperties& rhs);

PVR_STREAM_PROPERTIES::PVR_STREAM& operator<< (PVR_STREAM_PROPERTIES::PVR_STREAM& lhs, const XVDR::Stream& rhs);

PVR_SIGNAL_STATUS& operator<< (PVR_SIGNAL_STATUS& lhs, const XVDR::SignalStatus& rhs);
