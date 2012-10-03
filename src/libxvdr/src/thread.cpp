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

/*
 * Most of this code is taken from thread.c in the Video Disk Recorder ('VDR')
 */

#include <errno.h>
#include "xvdr/thread.h"
#include "xvdr/clientinterface.h"

#ifndef __APPLE__
#include <malloc.h>
#endif

#if !defined(__WINDOWS__)
#include <sys/signal.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

#include "libPlatform/os-dependent.h"

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

// --- CondWait -------------------------------------------------------------

CondWait::CondWait(void)
{
  signaled = false;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
}

CondWait::~CondWait()
{
  pthread_cond_broadcast(&cond); // wake up any sleepers
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&mutex);
}

void CondWait::SleepMs(int TimeoutMs)
{
  CondWait w;
  w.Wait(max(TimeoutMs, 3)); // making sure the time is >2ms to avoid a possible busy wait
}

bool CondWait::Wait(int TimeoutMs)
{
  pthread_mutex_lock(&mutex);
  if (!signaled) {
     if (TimeoutMs) {
        struct timespec abstime;
        if (GetAbsTime(&abstime, TimeoutMs)) {
           while (!signaled) {
                 if (pthread_cond_timedwait(&cond, &mutex, &abstime) == ETIMEDOUT)
                    break;
                 }
           }
        }
     else
        pthread_cond_wait(&cond, &mutex);
     }
  bool r = signaled;
  signaled = false;
  pthread_mutex_unlock(&mutex);
  return r;
}

void CondWait::Signal(void)
{
  pthread_mutex_lock(&mutex);
  signaled = true;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

// --- CondVar --------------------------------------------------------------

CondVar::CondVar(void)
{
  pthread_cond_init(&cond, 0);
}

CondVar::~CondVar()
{
  pthread_cond_broadcast(&cond); // wake up any sleepers
  pthread_cond_destroy(&cond);
}

void CondVar::Wait(Mutex &Mutex)
{
  if (Mutex.locked) {
     int locked = Mutex.locked;
     Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_wait
                       // does an implicit unlock of the mutex
     pthread_cond_wait(&cond, &Mutex.mutex);
     Mutex.locked = locked;
     }
}

bool CondVar::TimedWait(Mutex &Mutex, int TimeoutMs)
{
  bool r = true; // true = condition signaled, false = timeout

  if (Mutex.locked) {
     struct timespec abstime;
     if (GetAbsTime(&abstime, TimeoutMs)) {
        int locked = Mutex.locked;
        Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_timedwait
                          // does an implicit unlock of the mutex.
        if (pthread_cond_timedwait(&cond, &Mutex.mutex, &abstime) == ETIMEDOUT)
           r = false;
        Mutex.locked = locked;
        }
     }
  return r;
}

void CondVar::Broadcast(void)
{
  pthread_cond_broadcast(&cond);
}

// --- Mutex ----------------------------------------------------------------

Mutex::Mutex(void)
{
  locked = 0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
#ifndef __APPLE__
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#else
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
  pthread_mutex_init(&mutex, &attr);
}

Mutex::~Mutex()
{
  pthread_mutex_destroy(&mutex);
}

void Mutex::Lock(void)
{
  pthread_mutex_lock(&mutex);
  locked++;
}

void Mutex::Unlock(void)
{
 if (!--locked)
    pthread_mutex_unlock(&mutex);
}

// --- Thread ---------------------------------------------------------------

tThreadId Thread::mainThreadId = 0;

Thread::Thread(const char *Description)
{
  active = running = false;
#if !defined(__WINDOWS__)
  childTid = 0;
#endif
  childThreadId = 0;
  description = NULL;
  if (Description)
     SetDescription(Description);
}

Thread::~Thread()
{
  Cancel(); // just in case the derived class didn't call it
  free(description);
}

void Thread::SetPriority(int Priority)
{
#if !defined(__WINDOWS__)
  setpriority(PRIO_PROCESS, 0, Priority);
#endif
}

void Thread::SetIOPriority(int Priority)
{
#if !defined(__WINDOWS__)
#ifdef HAVE_LINUXIOPRIO
  syscall(SYS_ioprio_set, 1, 0, (Priority & 0xff) | (2 << 13));
#endif
#endif
}

void Thread::SetDescription(const char *Description, ...)
{
  free(description);
  description = NULL;
  if (Description)
  {
     va_list ap;

     // get string size
     va_start(ap, Description);
     int s = vsnprintf(NULL, 0, Description, ap);
     va_end(ap);

     if(s <= 0)
       return;

     va_start(ap, Description);
     description = (char*)malloc(s+1);
     vsnprintf(description, s+1, Description, ap);
     va_end(ap);
  }
}

void *Thread::StartThread(Thread *Thread)
{
  Thread->childThreadId = ThreadId();
  if (Thread->description) {
#ifdef PR_SET_NAME
  prctl(PR_SET_NAME, Thread->description, 0, 0, 0);
#endif
     }
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
        if (pthread_create(&childTid, NULL, (void *(*) (void *))&StartThread, (void *)this) == 0) {
           pthread_detach(childTid); // auto-reap
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
     if ((err = pthread_kill(childTid, 0)) != 0) {
#if !defined(__WINDOWS__)
        childTid = 0;
#endif
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
    pthread_cancel(childTid);
#if !defined(__WINDOWS__)
    childTid = 0;
#endif
    active = false;
  }
}

tThreadId Thread::ThreadId(void)
{
#ifdef __APPLE__
    return (int)pthread_self();
#else
#ifdef __WINDOWS__
  return GetCurrentThreadId();
#else
  return syscall(__NR_gettid);
#endif
#endif
}

void Thread::SetMainThreadId(void)
{
  if (mainThreadId == 0)
     mainThreadId = ThreadId();
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
