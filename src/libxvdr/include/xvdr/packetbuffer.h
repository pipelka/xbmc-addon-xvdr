#pragma once
/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2013 Andrey Pavlenko
 *      Copyright (C) 2013 Alexander Pipelka
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

#ifndef XVDR_PACKETBUFFER_H
#define XVDR_PACKETBUFFER_H

#include "xvdr/msgpacket.h"

namespace XVDR {

class PacketBuffer {

public:

  virtual ~PacketBuffer(){}

  /**
   * Create new PacketBuffer.
   *
   * @param max_size  Maximum buffer size in bytes.
   * @param file      Path to a file to store buffer data in.
   *                  If omitted or empty - in-memory storage will be used.
   */
  static PacketBuffer* create(size_t max_size, const std::string& file = "");

  /**
   * Put packet into the buffer.
   */
  virtual void put(MsgPacket* p) = 0;

  /**
   * Get next packet from the buffer,
   */
  virtual MsgPacket* get() = 0;

  /**
   * Signal the buffer that this packet isn't needed currently
   */
  virtual void release(MsgPacket* packet) {}

  /**
   * Try to seek to a position in the buffer
   */
  virtual bool seek(int time, bool backwards, double* startpts) = 0;

  /**
   * Clear the buffer.
   */
  virtual void clear() = 0;

  /**
   * Returns buffer's size in bytes.
   */
  virtual size_t size() = 0;

  /**
   * Returns number of packets in the buffer.
   */
  virtual size_t count() = 0;

  /**
   * Set maximum buffer size in bytes.
   */
  inline void set_max_size(size_t max_size) {
    _max_size = max_size;
  }

  /**
   * Returns the maximum buffer size.
   */
  inline size_t get_max_size() {
    return _max_size;
  }

protected:

  /**
   * Maximum buffer size.
   */
  size_t _max_size;

};

} // namespace XVDR

#endif // XVDR_PACKETBUFFER_H
