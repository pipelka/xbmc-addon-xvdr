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

#include <stdint.h>
#include <string>

class MsgPacket;

namespace XVDR {

class Callbacks;

class Session
{
public:

  Session();

  virtual ~Session();

  virtual bool Open(const std::string& hostname);

  virtual void Close();

  virtual void Abort();

  MsgPacket* ReadMessage();

  bool TransmitMessage(MsgPacket* vrp);

  MsgPacket* ReadResult(MsgPacket* vrp);

  bool ConnectionLost();

protected:

  virtual bool TryReconnect();

  bool IsOpen();

  virtual void OnDisconnect();

  virtual void OnReconnect();

  virtual void SignalConnectionLost();

  bool SendPing();

  std::string m_hostname;

  int m_port;

  int m_timeout;

  bool m_connectionLost;

private:

  int OpenSocket(const std::string& hostname, int port);

  bool readData(uint8_t* buffer, int totalBytes);

  int m_fd;

  /*struct streamPacketHeader;

  struct streamPacketHeader* m_streamPacketHeader;

  struct responsePacketHeader;

  struct responsePacketHeader* m_responsePacketHeader;*/
};

} // namespace XVDR
