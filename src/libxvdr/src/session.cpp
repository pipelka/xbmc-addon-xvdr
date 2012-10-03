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

#include "xvdr/clientinterface.h"
#include "xvdr/session.h"
#include "xvdr/responsepacket.h"
#include "xvdr/requestpacket.h"
#include "xvdr/thread.h"
#include "xvdr/command.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "os-config.h"

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
  shutdown(m_fd, SHUT_RDWR);
}

void Session::Close()
{
  if(!IsOpen())
    return;

  Abort();

  closesocket(m_fd);
  m_fd = INVALID_SOCKET;
}

int Session::OpenSocket(const std::string& hostname, int port) {
	int sock = INVALID_SOCKET;
	char service[10];
	snprintf(service, sizeof(service), "%i", port);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* result;

	if(getaddrinfo(hostname.c_str(), service, &hints, &result) != 0) {
		return INVALID_SOCKET;
	}

	// loop through results
	struct addrinfo* info = result;

	while(sock == INVALID_SOCKET && info != NULL) {
		sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		// try to connect
		if(sock != INVALID_SOCKET) {
			break;
		}

		info = info->ai_next;
	}

	if(sock == INVALID_SOCKET) {
		freeaddrinfo(result);
		return false;
	}

	setsock_nonblock(sock);

	int rc = 0;

	if(connect(sock, info->ai_addr, info->ai_addrlen) == -1) {
		if(sockerror() == EINPROGRESS || sockerror() == SEWOULDBLOCK) {

			if(!pollfd(sock, m_timeout, false)) {
				freeaddrinfo(result);
				return false;
			}

			socklen_t optlen = sizeof(int);
			getsockopt(sock, SOL_SOCKET, SO_ERROR, (sockval_t*)&rc, &optlen);
		}
		else {
			rc = sockerror();
		}
	}

	if(rc != 0) {
		freeaddrinfo(result);
		return INVALID_SOCKET;
	}

	setsock_nonblock(sock, false);

	int val = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (sockval_t*)&val, sizeof(val));

	freeaddrinfo(result);

	return sock;
}

bool Session::Open(const std::string& hostname)
{
  Close();

  m_fd = OpenSocket(hostname, m_port);

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

  bool compressed = (channelID & htobe32(0x80000000));

  if(compressed)
    channelID ^= htobe32(0x80000000);

  channelID = be32toh(channelID);

  if (channelID == XVDR_CHANNEL_STREAM)
  {
    if (!readData((uint8_t*)&m_streamPacketHeader, sizeof(m_streamPacketHeader))) return NULL;

    opCodeID = be32toh(m_streamPacketHeader.opCodeID);
    streamID = be32toh(m_streamPacketHeader.streamID);
    duration = be32toh(m_streamPacketHeader.duration);
    pts = be64toh(*(int64_t*)m_streamPacketHeader.pts);
    dts = be64toh(*(int64_t*)m_streamPacketHeader.dts);
    userDataLength = be32toh(m_streamPacketHeader.userDataLength);

    if (userDataLength > 0) {
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

    requestID = be32toh(m_responsePacketHeader.requestID);
    userDataLength = be32toh(m_responsePacketHeader.userDataLength);

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

bool Session::TransmitMessage(RequestPacket* vrp)
{
	int written = 0;
	int length = vrp->getLen();
	uint8_t* data = vrp->getPtr();

	while(written < length) {
		if(pollfd(m_fd, m_timeout, false) == 0) {
			return false;
		}

		int rc = send(m_fd, (sendval_t*)(data + written), length - written, MSG_DONTWAIT | MSG_NOSIGNAL);

		if(rc == -1 || rc == 0) {
			if(sockerror() == SEWOULDBLOCK) {
				continue;
			}

			return false;
		}

		written += rc;
	}

	return true;
}

ResponsePacket* Session::ReadResult(RequestPacket* vrp)
{
  if(!TransmitMessage(vrp))
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
	int read = 0;

	while(read < totalBytes) {
		if(pollfd(m_fd, m_timeout, true) == 0) {
			return false;
		}

		int rc = recv(m_fd, (char*)(buffer + read), totalBytes - read, MSG_DONTWAIT);

		if(rc == 0) {
			return false;
		}
		else if(rc == -1) {
			if(sockerror() == SEWOULDBLOCK) {
				continue;
			}

			return false;
		}

		read += rc;
	}

	return true;
}

bool Session::TryReconnect() {
	return false;
}

bool Session::ConnectionLost() {
  return m_connectionLost;
}

bool Session::SendPing()
{
  RequestPacket vrp(XVDR_PING);
  ResponsePacket* vresp = Session::ReadResult(&vrp);
  delete vresp;

  return (vresp != NULL);
}
