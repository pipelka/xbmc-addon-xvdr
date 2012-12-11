#pragma once
/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2000, 2003, 2006, 2008 Klaus Schmidinger
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

#include <stdio.h>
#include <sys/types.h>
#include "stdint.h"

namespace XVDR {

class TimeMs
{
private:
  uint64_t begin;
public:
  TimeMs(int Ms = 0);
      ///< Creates a timer with ms resolution and an initial timeout of Ms.
  static uint64_t Now(void);
  void Set(int Ms = 0);
  bool TimedOut(void);
  uint64_t Elapsed(void);
};

class CondWait {
private:
  struct props_t;
  props_t* props;
  bool signaled;
public:
  CondWait(void);
  ~CondWait();
  static void SleepMs(int TimeoutMs);
       ///< Creates a CondWait object and uses it to sleep for TimeoutMs
       ///< milliseconds, immediately giving up the calling thread's time
       ///< slice and thus avoiding a "busy wait".
       ///< In order to avoid a possible busy wait, TimeoutMs will be automatically
       ///< limited to values >2.
  bool Wait(int TimeoutMs = 0);
       ///< Waits at most TimeoutMs milliseconds for a call to Signal(), or
       ///< forever if TimeoutMs is 0.
       ///< \return Returns true if Signal() has been called, false it the given
       ///< timeout has expired.
  void Signal(void);
       ///< Signals a caller of Wait() that the condition it is waiting for is met.
  };

class Mutex;

class Mutex {
  friend class CondVar;
private:
  struct props_t;
  props_t* props;
  void* mutex;
  int locked;
public:
  Mutex(void);
  ~Mutex();
  void Lock(void);
  void Unlock(void);
  };

class Thread {
  friend class ThreadLock;
private:
  bool active;
  bool running;
  struct props_t;
  props_t* props;
  Mutex mutex;
  static void *StartThread(Thread *Thread);
protected:
  void Lock(void) { mutex.Lock(); }
  void Unlock(void) { mutex.Unlock(); }
  virtual void Action(void) = 0;
       ///< A derived Thread class must implement the code it wants to
       ///< execute as a separate thread in this function. If this is
       ///< a loop, it must check Running() repeatedly to see whether
       ///< it's time to stop.
  bool Running(void) { return running; }
       ///< Returns false if a derived Thread object shall leave its Action()
       ///< function.
  void Cancel(int WaitSeconds = 0);
       ///< Cancels the thread by first setting 'running' to false, so that
       ///< the Action() loop can finish in an orderly fashion and then waiting
       ///< up to WaitSeconds seconds for the thread to actually end. If the
       ///< thread doesn't end by itself, it is killed.
       ///< If WaitSeconds is -1, only 'running' is set to false and Cancel()
       ///< returns immediately, without killing the thread.
public:
  Thread();
       ///< Creates a new thread.
       ///< The Start() function must be called to actually start the thread.
  virtual ~Thread();
  bool Start(void);
       ///< Actually starts the thread.
       ///< If the thread is already running, nothing happens.
  bool Active(void);
       ///< Checks whether the thread is still alive.
  };

// MutexLock can be used to easily set a lock on mutex and make absolutely
// sure that it will be unlocked when the block will be left. Several locks can
// be stacked, so a function that makes many calls to another function which uses
// MutexLock may itself use a MutexLock to make one longer lock instead of many
// short ones.

class MutexLock {
private:
  Mutex *mutex;
  bool locked;
public:
  MutexLock(Mutex *Mutex = NULL);
  ~MutexLock();
  bool Lock(Mutex *Mutex);
  };

// ThreadLock can be used to easily set a lock in a thread and make absolutely
// sure that it will be unlocked when the block will be left. Several locks can
// be stacked, so a function that makes many calls to another function which uses
// ThreadLock may itself use a ThreadLock to make one longer lock instead of many
// short ones.

class ThreadLock {
private:
  Thread *thread;
  bool locked;
public:
  ThreadLock(Thread *Thread = NULL);
  ~ThreadLock();
  bool Lock(Thread *Thread);
  };

#define LOCK_THREAD ThreadLock threadLock_(this)

} // namespace XVDR
