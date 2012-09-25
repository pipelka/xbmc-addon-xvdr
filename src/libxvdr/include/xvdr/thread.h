/*
 *      Copyright (C) 2000, 2003, 2006, 2008 Klaus Schmidinger
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef __THREAD_H
#define __THREAD_H

#include <stdio.h>
#include <sys/types.h>
#include "stdint.h"

uint64_t ntohll(uint64_t a);
uint64_t htonll(uint64_t a);

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
  pthread_mutex_t mutex;
  pthread_cond_t cond;
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

class CondVar {
private:
  pthread_cond_t cond;
public:
  CondVar(void);
  ~CondVar();
  void Wait(Mutex &Mutex);
  bool TimedWait(Mutex &Mutex, int TimeoutMs);
  void Broadcast(void);
  };

class Mutex {
  friend class CondVar;
private:
  pthread_mutex_t mutex;
  int locked;
public:
  Mutex(void);
  ~Mutex();
  void Lock(void);
  void Unlock(void);
  };

typedef pid_t tThreadId;

class Thread {
  friend class ThreadLock;
private:
  bool active;
  bool running;
  pthread_t childTid;
  tThreadId childThreadId;
  Mutex mutex;
  char *description;
  static tThreadId mainThreadId;
  static void *StartThread(Thread *Thread);
protected:
  void SetPriority(int Priority);
  void SetIOPriority(int Priority);
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
  Thread(const char *Description = NULL);
       ///< Creates a new thread.
       ///< If Description is present, a log file entry will be made when
       ///< the thread starts and stops. The Start() function must be called
       ///< to actually start the thread.
  virtual ~Thread();
  void SetDescription(const char *Description, ...);
  bool Start(void);
       ///< Actually starts the thread.
       ///< If the thread is already running, nothing happens.
  bool Active(void);
       ///< Checks whether the thread is still alive.
  static tThreadId ThreadId(void);
  static tThreadId IsMainThread(void) { return ThreadId() == mainThreadId; }
  static void SetMainThreadId(void);
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

#endif //__THREAD_H
