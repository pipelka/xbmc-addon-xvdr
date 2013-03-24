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

#include <cstddef>
#include "xvdr/packetbuffer.h"

class Node {
public:
  MsgPacket* _packet;
  Node* _next;
  Node* _prev;

  Node(MsgPacket* packet) {
    _packet = packet;
    _prev = NULL;
    _next = NULL;
  }

  ~Node() {
    delete _packet;
  }

  inline size_t size() {
    return _packet->getPacketLength() + sizeof(Node);
  }
};

class MemPacketBuffer: public PacketBuffer {

public:

  MemPacketBuffer(size_t max_size) {
    _size = 0;
    _count = 0;
    _head = NULL;
    _tail = NULL;
    _current = NULL;
    _max_size = max_size;
  }

  ~MemPacketBuffer() {
    clear();
  }

  void write(MsgPacket* p) {
    Node* n = new Node(p);
    size_t size = n->size();
    ensure_size(size);

    if (_tail == NULL) {
      _head = _tail = _current = n;
    } else {
      n->_prev = _tail;
      _tail->_next = n;
      _tail = n;

      if (_current == NULL) {
        _current = n;
      }
    }

    _count++;
    _size += size;
  }

  MsgPacket* read_next() {
    if (_current != NULL) {
      MsgPacket* p = _current->_packet;
      _current = _current->_next;
      return p;
    }
    return NULL;
  }

  MsgPacket* read_prev() {
    if (_current != NULL) {
      if (_current != _head) {
        _current = _current->_prev;
        return _current->_packet;
      }
    } else if (_tail != NULL) {
      _current = _tail;
      return _current->_packet;
    }
    return NULL;
  }

  void clear() {
    for (Node* n = _head; n != NULL;) {
      Node* next = n->_next;
      delete n;
      n = next;
    }

    _size = _count = 0;
    _head = _tail = _current = NULL;
  }

  inline size_t size() {
    return _size;
  }

  inline size_t count() {
    return _count;
  }

private:
  size_t _size;
  size_t _count;
  Node* _head;
  Node* _tail;
  Node* _current;

  void ensure_size(uint32_t size) {
    size_t max = get_max_size();

    for (Node* n = _head; (n != NULL) && (_size + size) > max; n = _head) {
      size_t node_size = n->size();
      _head = n->_next;

      if (_current == n) {
        _current = _head;
      }
      if (_tail == n) {
        _tail = _head;
      }
      if (_head != NULL) {
        _head->_prev = NULL;
      }

      delete n;
      _count--;
      _size -= node_size;
    }
  }
};

PacketBuffer* PacketBuffer::create(size_t max_size, char* file) {
  if (file != NULL) {
    // Todo: implement
    return NULL;
  } else {
    return new MemPacketBuffer(max_size);
  }
}
