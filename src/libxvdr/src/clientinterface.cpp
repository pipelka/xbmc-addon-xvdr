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

#include <stdarg.h>
#include "xvdr/clientinterface.h"

using namespace XVDR;

#define MAX_MESSAGE_SIZE 512

ClientInterface::ClientInterface()
{
  m_msg = new char[MAX_MESSAGE_SIZE];
}

ClientInterface::~ClientInterface()
{
  delete[] m_msg;
}

void ClientInterface::Log(LOGLEVEL level, const std::string& text, ...)
{
  MutexLock lock(&m_msgmutex);

  va_list ap;
  va_start(ap, &text);

  vsnprintf(m_msg, MAX_MESSAGE_SIZE, text.c_str(), ap);
  va_end(ap);

  OnLog(level, m_msg);
}

void ClientInterface::Notification(LOGLEVEL level, const std::string& text, ...)
{
  MutexLock lock(&m_msgmutex);

  va_list ap;
  va_start(ap, &text);

  vsnprintf(m_msg, MAX_MESSAGE_SIZE, text.c_str(), ap);
  va_end(ap);

  OnNotification(level, m_msg);
}

void ClientInterface::Recording(const std::string& line1, const std::string& line2, bool on)
{
  Log(INFO, line1.c_str());
}

void ClientInterface::OnLog(LOGLEVEL level, const char* msg) {
  printf("[%s] %s\n",
      level == INFO ? "INFO" :
      level == NOTICE ? "NOTICE" :
      level == WARNING ? "WARNING" :
      level == FAILURE ? "FAILURE" :
      level == DEBUG ? "DEBUG" : "UNKNOWN",
      msg);
}

void ClientInterface::OnNotification(LOGLEVEL level, const char* msg) {
  OnLog(level, msg);
}

Packet* ClientInterface::StreamChange(const StreamProperties& p) {
	return NULL;
}

Packet* ClientInterface::ContentInfo(const StreamProperties& p) {
	return NULL;
}

void ClientInterface::OnDisconnect() {
  Log(FAILURE, "connection lost!");
}

void ClientInterface::OnReconnect() {
  Log(INFO, "connection restored.");
}

void ClientInterface::OnSignalLost() {
  Log(FAILURE, "signal lost!");
}

void ClientInterface::OnSignalRestored() {
  Log(INFO, "signal restored.");
}

void ClientInterface::Lock() {
  m_mutex.Lock();
}

void ClientInterface::Unlock() {
  m_mutex.Unlock();
}

