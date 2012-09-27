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
#include "xvdr/responsepacket.h"
#include "xvdr/requestpacket.h"
#include "xvdr/thread.h"
#include "xvdr/command.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

extern "C" {
#include "libTcpSocket/os-dependent_socket.h"
}

/* Needed on Mac OS/X */
 
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

using namespace XVDR;

Session::Session()
  : m_timeout(3000)
  , m_fd(INVALID_SOCKET)
  , m_connectionLost(false)
{
  m_port = 34891;
}

Session::~Session()
{
  Close();
}

void Session::Abort()
{
  tcp_shutdown(m_fd);
}

void Session::Close()
{
  if(!IsOpen())
    return;

  Abort();

  tcp_close(m_fd);
  m_fd = INVALID_SOCKET;
}

bool Session::Open(const std::string& hostname)
{
  Close();

  char errbuf[128];
  errbuf[0] = 0;

  m_fd = tcp_connect(hostname.c_str(), m_port, errbuf, sizeof(errbuf), m_timeout);

  if (m_fd == INVALID_SOCKET)
    return false;

  // store connection data
  m_hostname = hostname;

  return true;
}

bool Session::IsOpen()
{
  return m_fd != INVALID_SOCKET;
}

ResponsePacket* Session::ReadMessage()
{
  uint32_t channelID = 0;
  uint32_t requestID;
  uint32_t userDataLength = 0;
  uint8_t* userData = NULL;
  uint32_t streamID;
  uint32_t duration;
  uint32_t opCodeID;
  int64_t  dts = 0;
  int64_t  pts = 0;

  ResponsePacket* vresp = NULL;

  if(!readData((uint8_t*)&channelID, sizeof(uint32_t)))
    return NULL;

  // Data was read

  bool compressed = (channelID & htonl(0x80000000));

  if(compressed)
    channelID ^= htonl(0x80000000);

  channelID = ntohl(channelID);

  if (channelID == XVDR_CHANNEL_STREAM)
  {
    if (!readData((uint8_t*)&m_streamPacketHeader, sizeof(m_streamPacketHeader))) return NULL;

    opCodeID = ntohl(m_streamPacketHeader.opCodeID);
    streamID = ntohl(m_streamPacketHeader.streamID);
    duration = ntohl(m_streamPacketHeader.duration);
    pts = ntohll(*(int64_t*)m_streamPacketHeader.pts);
    dts = ntohll(*(int64_t*)m_streamPacketHeader.dts);
    userDataLength = ntohl(m_streamPacketHeader.userDataLength);

    /*if(opCodeID == XVDR_STREAM_MUXPKT) {
      Packet* p = client->AllocatePacket(userDataLength);
      userData = (uint8_t*)p;
      uint8_t* payload = client->GetPacketPayload(p);
      if (userDataLength > 0)
      {
        if (!userData) return NULL;
        if (payload == NULL || !readData(payload, userDataLength))
        {
          client->FreePacket(p);
          return NULL;
        }
      }
    }
    else*/ if (userDataLength > 0) {
      userData = (uint8_t*)malloc(userDataLength);
      if (!userData) return NULL;
      if (!readData(userData, userDataLength))
      {
        free(userData);
        return NULL;
      }
    }

    vresp = new ResponsePacket();
    vresp->setStream(opCodeID, streamID, duration, dts, pts, userData, userDataLength);
  }
  else
  {
    if (!readData((uint8_t*)&m_responsePacketHeader, sizeof(m_responsePacketHeader))) return NULL;

    requestID = ntohl(m_responsePacketHeader.requestID);
    userDataLength = ntohl(m_responsePacketHeader.userDataLength);

    if (userDataLength > 5000000) return NULL; // how big can these packets get?
    userData = NULL;
    if (userDataLength > 0)
    {
      userData = (uint8_t*)malloc(userDataLength);
      if (!userData) return NULL;
      if (!readData(userData, userDataLength)) {
        free(userData);
        return NULL;
      }
    }

    vresp = new ResponsePacket();
    if (channelID == XVDR_CHANNEL_STATUS)
      vresp->setStatus(requestID, userData, userDataLength);
    else
      vresp->setResponse(requestID, userData, userDataLength);

    if(compressed)
      vresp->uncompress();
  }

  return vresp;
}

bool Session::SendMessage(RequestPacket* vrp)
{
  return (tcp_send_timeout(m_fd, vrp->getPtr(), vrp->getLen(), m_timeout) == 0);
}

ResponsePacket* Session::ReadResult(RequestPacket* vrp)
{
  if(!SendMessage(vrp))
  {
    SignalConnectionLost();
    return NULL;
  }

  ResponsePacket *pkt = NULL;

  while((pkt = ReadMessage()))
  {
    /* Discard everything other as response packets until it is received */
    if (pkt->getChannelID() == XVDR_CHANNEL_REQUEST_RESPONSE && pkt->getRequestID() == vrp->getSerial())
    {
      return pkt;
    }
    else
      delete pkt;
  }

  SignalConnectionLost();
  return NULL;
}

bool Session::ReadSuccess(RequestPacket* vrp) {
  uint32_t rc;
  return ReadSuccess(vrp, rc);
}

bool Session::ReadSuccess(RequestPacket* vrp, uint32_t& rc)
{
  ResponsePacket *pkt = NULL;
  if((pkt = ReadResult(vrp)) == NULL)
    return false;

  rc = pkt->extract_U32();
  delete pkt;

  if(rc != XVDR_RET_OK)
  {
    //XVDRLog(XVDR_ERROR, "%s - failed with error code '%i'", __FUNCTION__, rc);
    return false;
  }

  return true;
}

void Session::OnReconnect() {
}

void Session::OnDisconnect() {
}

void Session::SignalConnectionLost()
{
  if(m_connectionLost)
    return;

  m_connectionLost = true;
  Abort();
  Close();

  OnDisconnect();
}

bool Session::readData(uint8_t* buffer, int totalBytes)
{
  return (tcp_read_timeout(m_fd, buffer, totalBytes, m_timeout) == 0);
}

bool Session::TryReconnect() {
	return false;
}

void Session::SleepMs(int ms)
{
#ifdef __WINDOWS__
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

bool Session::ConnectionLost() {
  return m_connectionLost;
}
