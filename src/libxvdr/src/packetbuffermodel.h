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

#include "xvdr/packetbuffer.h"
#include "xvdr/command.h"

using namespace XVDR;

class Node {
public:
  Node* _next;
  Node* _prev;
  MsgPacket* _packet;

  Node(MsgPacket* packet, PacketBuffer* buffer) {
    _packet = packet;
    _prev = NULL;
    _next = NULL;
  }

  virtual ~Node() {
    delete _packet;
  }

  virtual size_t size() = 0;

  virtual uint8_t frametype() = 0;

  virtual int64_t pts() = 0;

  virtual int64_t dts() = 0;

  virtual MsgPacket* packet() {
    return _packet;
  }

  virtual void release() {
  }
};

template<class NodeType>
class PacketBufferModel: public PacketBuffer {

public:

  PacketBufferModel(size_t max_size) {
    _size = 0;
    _count = 0;
    _head = NULL;
    _tail = NULL;
    _current = NULL;
    _current_last = NULL;
    _max_size = max_size;
  }

  ~PacketBufferModel() {
    clear();
  }

  void put(MsgPacket* p) {
    NodeType* n = new NodeType(p, this);

    size_t size = n->size();
    ensure_size(size);

    if (_tail == NULL) {
      _head = _tail = _current = n;
    }
    else {
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

  virtual MsgPacket* get() {
    MsgPacket* p = NULL;
    if (_current != NULL) {
      p = _current->packet();
      next();
    }
    if(p != NULL) {
      p->rewind();
    }
    return p;
  }

  NodeType* next() {
    if (_current != NULL) {
      _current = (NodeType*)_current->_next;
    }
    return _current;
  }

  NodeType* prev() {
    if (_current != NULL) {
      if (_current != _head) {
        _current = (NodeType*)_current->_prev;
        return _current;
      }
    }
    else if (_tail != NULL) {
      _current = (NodeType*)_tail;
      return _current;
    }
    return NULL;
  }

  void current_push() {
    _current_last = _current;
  }

  void current_pop() {
    _current = _current_last;
    _current_last = NULL;
  }

  bool seek(int time, bool backwards, double *startpts) {
    NodeType* p = NULL;

    int64_t t = (int64_t)time * 1000;
    *startpts = t;

    current_push();

    // rewind
    if (backwards) {
      while((p = prev()) != NULL) {
        if(p->frametype() == 1 && t >= p->pts()) {
          *startpts = p->dts();
          return true;
        }
      }

      current_pop();
      return true;
    }

    // fast-forward
    while((p = next()) != NULL) {
      if(p->frametype() == 1 && t <= p->pts()) {
        *startpts = p->dts();
        return true;
      }
    }

    current_pop();
    return true;
  }

  void clear() {
    for (NodeType* n = _head; n != NULL;) {
      NodeType* next = (NodeType*)n->_next;
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

  inline NodeType* tail() {
    return _tail;
  }

protected:

  inline NodeType* current() {
    return _current;
  }

private:
  size_t _size;
  size_t _count;
  NodeType* _head;
  NodeType* _tail;
  NodeType* _current;
  NodeType* _current_last;

  void ensure_size(uint32_t size) {
    size_t max = get_max_size();

    for (NodeType* n = _head; (n != NULL) && (_size + size) > max; n = _head) {
      size_t node_size = n->size();
      _head = (NodeType*)n->_next;

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
