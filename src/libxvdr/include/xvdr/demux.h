#pragma once
/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include <string>
#include <queue>

#include "xvdr/clientinterface.h"
#include "xvdr/connection.h"
#include "xvdr/dataset.h"
#include "xvdr/command.h"
#include "xvdr/packetbuffer.h"

class MsgPacket;

namespace XVDR {

class ClientInterface;

class Demux : public Connection
{
public:

  // channel switch return codes
  typedef enum {
    SC_OK = XVDR_RET_OK,                            /* !< channel switch successful */
    SC_ACTIVE_RECORDING = XVDR_RET_RECRUNNING,      /* !< active recording blocks channel switch */
    SC_DEVICE_BUSY = XVDR_RET_DATALOCKED,           /* !< all devices busy */
    SC_ENCRYPTED = XVDR_RET_ENCRYPTED,              /* !< encrypted channel cannot be decrypted */
    SC_ERROR = XVDR_RET_ERROR,                      /* !< server (communication) error */
    SC_INVALID_CHANNEL = XVDR_RET_DATAINVALID       /* !< invalid channel */
  } SwitchStatus;

public:

  Demux(ClientInterface* client, PacketBuffer* buffer);
  ~Demux();

  SwitchStatus OpenChannel(const std::string& hostname, uint32_t channeluid, const std::string& clientname = "");
  void CloseChannel();

  void Abort();

  Packet* Read();

  template<class T>T* Read() {
    return (T*)Read();
  }

  SwitchStatus SwitchChannel(uint32_t channeluid);
  void SetPriority(int priority);
  void SetStartWithIFrame(bool on);

  StreamProperties GetStreamProperties();
  SignalStatus GetSignalStatus();

  void Pause(bool on);

  void RequestSignalInfo();

  bool CanSeekStream();

  bool SeekTime(int time, bool backwards, double *startpts);

protected:

  void OnDisconnect();
  void OnReconnect();

  bool OnResponsePacket(MsgPacket *resp);

  void StreamChange(MsgPacket *resp);
  void StreamStatus(MsgPacket *resp);
  void StreamSignalInfo(MsgPacket *resp);

private:

  void GetContentFromType(const std::string& type, std::string& content);

  void CleanupPacketQueue();

  StreamProperties m_streams;
  SignalStatus m_signal;
  int m_priority;
  uint32_t m_channeluid;
  std::queue<Packet*> m_queue;
  PacketBuffer* m_buffer;
  Mutex m_lock;
  CondWait m_cond;
  bool m_paused;
  bool m_timeshiftmode;
  TimeMs m_lastsignal;
  bool m_iframestart;
};

} // namespace XVDR
