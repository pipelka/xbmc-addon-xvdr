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

#include <cstddef>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <set>

#include "packetbuffermodel.h"

template<class PBufferType>
class FileNode : public Node {
protected:

  int64_t m_offset;
  PBufferType m_buffer;
  size_t m_size;
  uint8_t m_frametype;
  int64_t m_pts;
  int64_t m_dts;

public:

  FileNode(MsgPacket* packet, PacketBuffer* buffer) :
  Node(packet, buffer),
  m_offset(0),
  m_buffer((PBufferType)buffer),
  m_pts(0),
  m_dts(0) {
    m_frametype = packet->getClientID() & 0xFF;
    m_size = packet->getPacketLength();

    if(packet->getMsgID() == XVDR_STREAM_MUXPKT) {
      packet->rewind();
      packet->get_U16();
      m_pts = packet->get_S64();
      m_dts = packet->get_S64();
      packet->rewind();
    }

    // get offset
    if(m_buffer->tail() != NULL) {
      m_offset = m_buffer->tail()->m_offset + m_buffer->tail()->size();
    }

    // check for ring-buffer wrap
    if(m_offset > m_buffer->get_max_size()) {
      m_offset = 0;
    }

    // set file position
    if(lseek(m_buffer->fd(), m_offset, SEEK_SET) == -1) {
      return;
    }

    // write packet to buffer
    packet->write(m_buffer->fd(), m_buffer->timeout());
    release();
  }

  MsgPacket* packet() {
    if(_packet == NULL) {
      if(lseek(m_buffer->fd(), m_offset, SEEK_SET) == -1) {
        return NULL;
      }
      _packet = MsgPacket::read(m_buffer->fd(), m_buffer->timeout());
    }

    return _packet;
  }

  void release() {
    delete _packet;
    _packet = NULL;
  }

  size_t size() {
      return m_size;
  }

  uint8_t frametype() {
    return m_frametype;
  }

  int64_t pts() {
    return m_pts;
  }

  int64_t dts() {
    return m_dts;
  }

};


class DiskPacketBuffer;
typedef FileNode<DiskPacketBuffer*> DiskNode;

class DiskPacketBuffer : public PacketBufferModel<DiskNode> {
public:

  DiskPacketBuffer(size_t max_mem, const std::string& file, int timeout_ms = 3000) :
  PacketBufferModel<DiskNode>::PacketBufferModel(max_mem),
  m_fd(-1),
  m_timeout(timeout_ms) {
    m_filename = file;
    m_fd = open(m_filename.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
  }

  virtual ~DiskPacketBuffer() {
    if(m_fd != -1) {
      close(m_fd);
      unlink(m_filename.c_str());
    }
  }

  MsgPacket* get() {
    DiskNode* n = current();
    MsgPacket* p = PacketBufferModel<DiskNode>::get();

    if(p != NULL) {
      m_used.insert(n);
    }

    return p;
  }

  void release(MsgPacket* p) {
    std::set<DiskNode*>::iterator i = m_used.begin();
    while(i != m_used.end()) {
      if((*i)->packet() == p) {
        (*i)->release();
        m_used.erase(i);
        return;
      }
    }
  }

  inline int fd() {
    return m_fd;
  }

  inline int timeout() {
    return m_timeout;
  }

private:

  std::string m_filename;

  int m_fd;

  int m_timeout;

  std::set<DiskNode*> m_used;
};


class MemNode : public Node {
public:

  MemNode(MsgPacket* packet, PacketBuffer* buffer) : Node(packet, buffer) {}

  size_t size() {
    if(_packet == NULL) {
      return 0;
    }
    return _packet->getPacketLength();
  }

  uint8_t frametype() {
    if(_packet == NULL) {
      return 0;
    }
    return _packet->getClientID() && 0xFF;
  }

  int64_t pts() {
    if(_packet == NULL) {
      return 0;
    }

    if(_packet->getMsgID() == XVDR_STREAM_MUXPKT) {
      _packet->rewind();
      _packet->get_U16();
      int64_t pts = _packet->get_S64();
      _packet->rewind();
      return pts;
    }

    return 0;
  }

  int64_t dts() {
    if(_packet == NULL) {
      return 0;
    }

    if(_packet->getMsgID() == XVDR_STREAM_MUXPKT) {
      _packet->rewind();
      _packet->get_U16();
      _packet->get_S64();
      int64_t dts = _packet->get_S64();
      _packet->rewind();
      return dts;
    }

    return 0;
  }
};

class MemPacketBuffer : public PacketBufferModel<MemNode> {
public:
  MemPacketBuffer(size_t max_mem) : PacketBufferModel<MemNode>::PacketBufferModel(max_mem) {};
};


namespace XVDR {

PacketBuffer* PacketBuffer::create(size_t max_size, const std::string& file) {
  PacketBuffer* buf = NULL;

  if (!file.empty()) {
    buf = new DiskPacketBuffer(max_size, file);
  } else {
    buf = new MemPacketBuffer(max_size);
  }

  return buf;
}

} // namespace XVDR
