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

/*
 * Most of this code is taken from thread.c in the Video Disk Recorder ('VDR')
 */

#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>

#include "xvdr/thread.h"
#include "xvdr/clientinterface.h"

#include "os-config.h"

using namespace XVDR;

static bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0) {           // get current time
     now.tv_sec  += MillisecondsFromNow / 1000;  // add full seconds
     now.tv_usec += (MillisecondsFromNow % 1000) * 1000;  // add microseconds
     if (now.tv_usec >= 1000000) {               // take care of an overflow
        now.tv_sec++;
        now.tv_usec -= 1000000;
        }
     Abstime->tv_sec = now.tv_sec;          // seconds
     Abstime->tv_nsec = now.tv_usec * 1000; // nano seconds
     return true;
     }
  return false;
}

// --- TimeMs ---------------------------------------------------------------

TimeMs::TimeMs(int Ms)
{
  Set(Ms);
}

uint64_t TimeMs::Now(void)
{
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}

void TimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool TimeMs::TimedOut(void)
{
  return Now() >= begin;
}

uint64_t TimeMs::Elapsed(void)
{
  return Now() - begin;
}

// --- CondWait -------------------------------------------------------------

struct CondWait::props_t {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

CondWait::CondWait(void) : signaled(false), props(new props_t)
{
  pthread_mutex_init(&props->mutex, NULL);
  pthread_cond_init(&props->cond, NULL);
}

CondWait::~CondWait()
{
  pthread_cond_broadcast(&props->cond); // wake up any sleepers
  pthread_cond_destroy(&props->cond);
  pthread_mutex_destroy(&props->mutex);
  delete props;
}

void CondWait::SleepMs(int TimeoutMs)
{
  CondWait w;
  w.Wait(TimeoutMs);
}

bool CondWait::Wait(int TimeoutMs)
{
  pthread_mutex_lock(&props->mutex);
  if (!signaled) {
     if (TimeoutMs) {
        struct timespec abstime;
        if (GetAbsTime(&abstime, TimeoutMs)) {
           while (!signaled) {
                 if (pthread_cond_timedwait(&props->cond, &props->mutex, &abstime) == ETIMEDOUT)
                    break;
                 }
           }
        }
     else
        pthread_cond_wait(&props->cond, &props->mutex);
     }
  bool r = signaled;
  signaled = false;
  pthread_mutex_unlock(&props->mutex);
  return r;
}

void CondWait::Signal(void)
{
  pthread_mutex_lock(&props->mutex);
  signaled = true;
  pthread_cond_broadcast(&props->cond);
  pthread_mutex_unlock(&props->mutex);
}

// --- Mutex ----------------------------------------------------------------

struct Mutex::props_t {
	pthread_mutex_t mutex;
};

Mutex::Mutex(void) : props(new props_t)
{
  locked = 0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
#if defined(__APPLE__) || defined(__FreeBSD__)
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
  pthread_mutex_init(&props->mutex, &attr);
  mutex = &props->mutex;
}

Mutex::~Mutex()
{
  pthread_mutex_destroy(&props->mutex);
  delete props;
}

void Mutex::Lock(void)
{
  pthread_mutex_lock(&props->mutex);
  locked++;
}

void Mutex::Unlock(void)
{
 if (!--locked)
    pthread_mutex_unlock(&props->mutex);
}

// --- Thread ---------------------------------------------------------------

struct Thread::props_t {
  pthread_t childTid;
};

Thread::Thread() : props(new props_t)
{
  active = running = false;
}

Thread::~Thread()
{
  Cancel(); // just in case the derived class didn't call it
  delete props;
}

void *Thread::StartThread(Thread *Thread)
{
  Thread->props->childTid = pthread_self();
  Thread->Action();
  Thread->running = false;
  Thread->active = false;
  return NULL;
}

#define THREAD_STOP_TIMEOUT  3000 // ms to wait for a thread to stop before newly starting it
#define THREAD_STOP_SLEEP      30 // ms to sleep while waiting for a thread to stop

bool Thread::Start(void)
{
  if (!running) {
     if (active) {
        // Wait until the previous incarnation of this thread has completely ended
        // before starting it newly:
        TimeMs RestartTimeout;
        while (!running && active && RestartTimeout.Elapsed() < THREAD_STOP_TIMEOUT)
              CondWait::SleepMs(THREAD_STOP_SLEEP);
        }
     if (!active) {
        active = running = true;
        if (pthread_create(&props->childTid, NULL, (void *(*) (void *))&StartThread, (void *)this) == 0) {
           pthread_detach(props->childTid); // auto-reap
           }
        else {
           active = running = false;
           return false;
           }
        }
     }
  return true;
}

bool Thread::Active(void)
{
  if (active) {
     //
     // Single UNIX Spec v2 says:
     //
     // The pthread_kill() function is used to request
     // that a signal be delivered to the specified thread.
     //
     // As in kill(), if sig is zero, error checking is
     // performed but no signal is actually sent.
     //
     int err;
     if ((err = pthread_kill(props->childTid, 0)) != 0) {
        active = running = false;
        }
     else
        return true;
     }
  return false;
}

void Thread::Cancel(int WaitSeconds)
{
  running = false;
  if (active && WaitSeconds > -1)
  {
    if (WaitSeconds > 0)
    {
      for (time_t t0 = time(NULL) + WaitSeconds; time(NULL) < t0; )
      {
        if (!Active())
          return;
        CondWait::SleepMs(10);
      }
    }
    // ANDROID doesnt know how to cancel streams
#ifndef ANDROID
    pthread_cancel(props->childTid);
#endif
    active = false;
  }
}

// --- MutexLock ------------------------------------------------------------

MutexLock::MutexLock(Mutex *Mutex)
{
  mutex = NULL;
  locked = false;
  Lock(Mutex);
}

MutexLock::~MutexLock()
{
  if (mutex && locked)
    mutex->Unlock();
}

bool MutexLock::Lock(Mutex *Mutex)
{
  if (Mutex && !mutex)
  {
    mutex = Mutex;
    Mutex->Lock();
    locked = true;
    return true;
  }
  return false;
}

// --- ThreadLock -----------------------------------------------------------

ThreadLock::ThreadLock(Thread *Thread)
{
  thread = NULL;
  locked = false;
  Lock(Thread);
}

ThreadLock::~ThreadLock()
{
  if (thread && locked)
    thread->Unlock();
}

bool ThreadLock::Lock(Thread *Thread)
{
  if (Thread && !thread)
  {
  thread = Thread;
  Thread->Lock();
  locked = true;
  return true;
  }
  return false;
}
