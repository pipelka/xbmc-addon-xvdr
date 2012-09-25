#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#include "xvdr/callbacks.h"
#include "xvdr/session.h"
#include <string>

#include "xvdr/dataset.h"

namespace XVDR {

class ResponsePacket;

class Demux : public Session
{
public:

  Demux();
  ~Demux();

  bool OpenChannel(const std::string& hostname, uint32_t channeluid);
  void Abort();
  const StreamProperties& GetStreamProperties();
  Packet* Read();
  bool SwitchChannel(uint32_t channeluid);
  //int CurrentChannel() { return m_channelinfo.iChannelNumber; }
  const SignalStatus& GetSignalStatus();
  void SetPriority(int priority);

protected:

  void OnReconnect();

  void StreamChange(ResponsePacket *resp);
  void StreamStatus(ResponsePacket *resp);
  void StreamSignalInfo(ResponsePacket *resp);
  bool StreamContentInfo(ResponsePacket *resp);

private:

  StreamProperties m_streams;
  SignalStatus     m_signal;
  int                   m_priority;
  uint32_t				m_channeluid;
};

} // namespace XVDR
