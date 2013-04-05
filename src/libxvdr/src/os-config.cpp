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

#include "os-config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// WINDOWS

#ifdef WIN32

#if !defined(SIO_KEEPALIVE_VALS)
#define SIO_KEEPALIVE_VALS  _WSAIOW(IOC_VENDOR,4)
struct tcp_keepalive {
  u_long onoff;
  u_long keepalivetime;
  u_long keepaliveinterval;
};
#endif

uint32_t htobe32(uint32_t u) {
	return (u >> 24) | ((u >> 8) & 0xff00) | ((u << 8) & 0xff0000) | (u << 24);
}

uint64_t htobe64(uint64_t u) {
	return ((uint64_t) htobe32((uint32_t) u) << 32) | htobe32((uint32_t)(u >> 32));
}

uint16_t htobe16(uint16_t u) {
	return (u >> 8) | ((u & 0xff) << 8);
}

uint32_t be32toh(uint32_t u) {
	return (u << 24) | ((u << 8) & 0xff0000) | ((u >> 8) & 0xff00) | (u >> 24);
}

uint64_t be64toh(uint64_t u) {
	return ((uint64_t) be32toh((uint32_t) u) << 32) | be32toh((uint32_t)(u >> 32));
}

uint16_t be16toh(uint16_t u) {
	return (u >> 8) | (u & 0xff) << 8;
}

bool pollfd(int fd, int timeout_ms, bool in) {
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = 1000 * (timeout_ms % 1000);

	if(in) {
		return (select(fd + 1, &fds, NULL, NULL, &tv) > 0);
	}

	return (select(fd + 1, NULL, &fds, NULL, &tv) > 0);
}

void setsock_keepalive(int sock) {
  struct tcp_keepalive param;
  param.onoff = 1;
  param.keepalivetime = 1000;
  param.keepaliveinterval = 3000;

  DWORD bytes = 0;

  WSAIoctl(
    sock,
    SIO_KEEPALIVE_VALS,
    &param,
    sizeof(param),
    NULL,
    0,
    &bytes,
    NULL,
    NULL
  );
}

// LINUX / OTHER

#else

#include <poll.h>

bool pollfd(int fd, int timeout_ms, bool in) {
	struct pollfd p;
	p.fd = fd;
	p.events = in ? POLLIN : POLLOUT;
	p.revents = 0;

	return (::poll(&p, 1, timeout_ms) > 0);
}

void setsock_keepalive(int sock) {
  int val = 1;
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (sockval_t*)&val, sizeof(val));

#ifndef __APPLE__
  val = 1;
  setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (sockval_t*)&val, sizeof(val));

  val = 1;
  setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (sockval_t*)&val, sizeof(val));

  val = 3;
  setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (sockval_t*)&val, sizeof(val));
#endif
}


#endif

// GENERAL

bool setsock_nonblock(int fd, bool nonblock) {
#ifdef WIN32
	u_long sval = nonblock;
	return (ioctlsocket(fd, FIONBIO, &sval) == 0);
#else
	return (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) != -1);
#endif
}

int socketread(int fd, uint8_t* data, int datalen, int timeout_ms) {
        int read = 0;

        while(read < datalen) {
                if(pollfd(fd, timeout_ms, true) == 0) {
                        return ETIMEDOUT;
                }

                int rc = recv(fd, (char*)(data + read), datalen - read, MSG_DONTWAIT);

                if(rc == -1 && sockerror() == ENOTSOCK) {
                        rc = ::read(fd, data + read, datalen - read);
                }

                if(rc == 0) {
                        return ECONNRESET;
                }
                else if(rc == -1) {
                        if(sockerror() == SEWOULDBLOCK) {
                                continue;
                        }

                        return sockerror();
                }

                read += rc;
        }

        return 0;
}

const char* os_gettempfolder() {
  char* temp = NULL;

#if defined(WIN32)
  temp = getenv("APPDATA");
  if(temp == NULL) {
    temp = getenv("TEMP");
  }

#elseif defined(__APPLE__) || defined(__FreeBSD__)
  temp = getenv("TMPDIR");
  if(temp == NULL) {
    temp = getenv("DARWIN_USER_TEMP_DIR");
  }

#else
  temp = P_tmpdir;
#endif

  return temp;
}
