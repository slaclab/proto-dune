//-----------------------------------------------------------------------------
// File          : CommQueue.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Communications Queue
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <CommQueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
using namespace std;

// Constructor
CommQueue::CommQueue(uint32_t size, bool threaded) {
   _size     = size; 
   _read     = 0;
   _write    = 0;
   _count    = 0;
   _threaded = threaded;

   _data = (void ** volatile)malloc(sizeof(void *)*_size);

   pthread_cond_init(&_qCondition,NULL);
   pthread_mutex_init(&_qMutex,NULL);
}

// DeConstructor
CommQueue::~CommQueue() {
   free(_data);
}

// Push single element to queue
bool CommQueue::push ( void *ptr, uint32_t wait ) {
   struct timespec timeout;
   uint32_t  next;
   bool            ret;

   if ( _threaded ) pthread_mutex_lock(&_qMutex);

   next = (_write + 1) % _size;

   if ( _threaded && (wait > 0) && (next == _read) ) {
      clock_gettime(CLOCK_REALTIME,&timeout);
      timeout.tv_sec  += wait / 1000000;
      timeout.tv_nsec += (wait % 1000000) * 1000;
      pthread_cond_timedwait(&_qCondition,&_qMutex,&timeout);
   }

   if ( next == _read ) ret = false;
   else {
      _data[_write] = ptr;
      _write = next;
      _count++;
      ret = true;
   } 

   if ( _threaded ) {
      pthread_cond_signal(&_qCondition);
      pthread_mutex_unlock(&_qMutex);
   }

   return(ret);
}

// Pop single element from queue
void *CommQueue::pop (uint32_t wait ) {
   struct timespec timeout;
   uint32_t  next;
   void *          ptr;

   if ( _threaded ) pthread_mutex_lock(&_qMutex);

   if ( _threaded && (wait > 0) && (_read == _write) ) {
      clock_gettime(CLOCK_REALTIME,&timeout);
      timeout.tv_sec  += wait / 1000000;
      timeout.tv_nsec += (wait % 1000000) * 1000;
      pthread_cond_timedwait(&_qCondition,&_qMutex,&timeout);
   }

   if ( _read == _write ) ptr = NULL;
   else {
      next = (_read + 1) % _size;
      ptr = _data[_read];
      _read = next;
      _count--;
   }

   if ( _threaded ) {
      pthread_cond_signal(&_qCondition);
      pthread_mutex_unlock(&_qMutex);
   }

   return(ptr);
}


// Queue has data
bool CommQueue::ready () {
   bool ret;

   if ( _threaded ) pthread_mutex_lock(&_qMutex);
   ret = ( _read != _write);
   if ( _threaded ) pthread_mutex_unlock(&_qMutex);
   return(ret);
}

// Size
uint32_t CommQueue::entryCnt () {
   uint32_t ret;
   if ( _threaded ) pthread_mutex_lock(&_qMutex);
   ret = _count;
   if ( _threaded ) pthread_mutex_unlock(&_qMutex);
   return(ret);
}


