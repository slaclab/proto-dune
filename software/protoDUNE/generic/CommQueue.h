//-----------------------------------------------------------------------------
// File          : CommQueue.h
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
#ifndef __COMM_QUEUE_H__
#define __COMM_QUEUE_H__
#include <pthread.h>
#include <stdint.h>
using namespace std;

// Class For IBIOS Messages
class CommQueue {

      // Constants
      uint32_t _size;

      // Read and write pointer
      volatile uint32_t _read;
      volatile uint32_t _write;

      // Entry level counter
      volatile uint32_t _count;
 
      // Circular Buffer
      void ** volatile _data;

      // Lock and conditions for inter-thread
      pthread_cond_t  _qCondition;
      pthread_mutex_t _qMutex;

      // Lock configuration
      bool _threaded;

   public:

      // Constructor
      CommQueue(uint32_t size = 8192, bool threaded=true);

      // DeConstructor
      ~CommQueue();

      // Push single element to queue
      bool push ( void *ptr, uint32_t wait=0 );

      // Pop single element from queue
      void *pop (uint32_t wait=0);

      // Queue has data
      bool ready ();      

      // Queue depth
      uint32_t entryCnt ();      
};
#endif

