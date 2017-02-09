//-----------------------------------------------------------------------------
// File          : DaqBuffer.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 09/18/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    Class which implements a large circular buffer for incoming front end
//    time slices. This module supports three modes of operation.
//
//    1: Fill entire buffer with continous data and then read it out
//
//    2: Read out a single channel continously. 
//
//    3: Accept trigger record from firmware or software and read out data 
//       around the trigger point.
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
//
// Modification history :
//
//       DATE  WHO  WHAT
// ----------  ---  -----------------------------------------------------------
// 2016.11.05  jjr  Added receive sequence number
// 2016.11.05  jjr  Incorporated new DaqHeader
// 2016.10.27  jjr  Added throttling of the output in the configuation block
//                  These are the naccept and nframe field members
//    Unknown   lr  Created
//  
//-----------------------------------------------------------------------------
#ifndef __ART_DAQ_BUFFER_H__
#define __ART_DAQ_BUFFER_H__

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <queue>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <CommQueue.h>

#include <RunMode.h>

#include <string.h>

using namespace std;

// Status counters
struct BufferStatus {
   uint32_t buffCount;
   uint32_t rxPend;

   uint32_t rxCount;
   uint32_t rxErrors;
   uint32_t rxSize;
   uint32_t dropCount;
   uint32_t triggers;

   uint32_t txErrors;
   uint32_t txSize;
   uint32_t txCount;
   uint32_t txPend;

   float    triggerRate;
   float    rxBw;
   float    rxRate;
   float    txBw;
   float    txRate;
};


/*---------------------------------------------------------------------- *//*!
 *
 * \struct DaqHeader
 * \brief  The transport header that prefaces outgoing data
 *
\*---------------------------------------------------------------------- */ 
struct DaqHeader
{
   uint32_t     frame_size;  /*!< Frame size, in bytes                   */
   uint32_t    tx_sequence;  /*!< Transmit frame sequence number         */
   uint32_t    rx_sequence;  /*!< Received frame sequence number         */
   uint32_t        type_id;  /*!< Reserved ...                           */
};
/*---------------------------------------------------------------------- */



/*---------------------------------------------------------------------- *//*!
 *
 * \class  DaqBuffer
 * \brief  The control structure for taking data
 *
\* ---------------------------------------------------------------------- */ 
class DaqBuffer {


   /* ------------------------------------------------------------------ *//*!
    *
    * \struct Config
    * \brief  DAQ Configuration control parameters
    *
    * \par
    *  These parameters control the
    *    -# RunMode
    *    -# Whether to pitch the incoming DMA into the bit bucker
    *    -# Whthher to transmit or pitch the accepted data to the host
    *    -# Control which frames are accepted
    *
    * Since not all frames can be promoted to the host (way too much
    * data. a simple scheme of accepting M frames out of every N frames
    * is used to throttle the rate.  Examples 
    *
    *       - 10/2048  Accept ten consecutive frames out of every 2048,
    *                  essentially 10 frames/second
    *       - 20/4096  Still 10 frames/second, but with 20 consecutive
    *                  frames every 2 seconds.
    *
    * Given that each frame is roughly 245KBytes and the output bandwidth
    * is around 50 MBytes/sec, the average sustainable rate is around
    * 50 MBytes / .245MByes = 204 Hz.
    *
    * There is also a somewhat softer limit of around 1000 consecutive 
    * frames.  At this point the internal buffering will be exceeded
    * meaning there is no place to place new events.
    *
    * There are 800 packets being consumed a 1953 packets/sec
    * Each packet contains around 245KBytes being drained at 50 MBytes/sec
    * The below gives at what time 't' does one run out of packets
    *
    *       (1953 - 50x10e6/245x10e3) * t = 800
    *     = (1953 - 203) * t              = 800
    *     => t = .457 secs
    *     or 1953 packets/sec * .457 sec = 892 packets
    * 
    * It will take approximately 4 seconds to drain this memory so
    *       892/8028 is about the limit for getting consecutive packets
    *
    * Rule of thumb: Keep below a 10% duty cycle and don't exceed around
    * 800 packets in a cycle.
    */
   struct Config
   {
      RunMode         _runMode;  /*!< The run mode                       */      
      uint32_t _blowOffDmaData;  /*!< If non-zero, pitch incoming data   */
      uint32_t   _blowOffTxEth;  /*!< If non-zero, do not transmit       */
      uint32_t        _naccept;  /*!< # of consecutive frames to accetp  */
      uint32_t        _nframes;  /*!< # the accept frame duty cycle      */
   };
   /* ------------------------------------------------------------------ */


   // Statistics Counters
   class Counters
   {
   public:
      Counters () { return; }

      void reset () volatile
      {
         memset  ((void *)this, 0, sizeof (*this));
         return;
      }

   public:
      uint32_t _rxCount;
      uint32_t _rxTotal;
      uint32_t _rxErrors;

      uint32_t _dropCount;
      uint32_t _triggers;

      uint32_t _txCount;
      uint32_t _txTotal;
      uint32_t _txErrors;
   };


   private:
      // Software Device Configurations
      static const uint32_t SoftwareVersion = 0x1;
      static const uint32_t TxFrameCount    = 100;
      static const uint32_t RxFrameCount    = 10000;
      static const uint32_t WaitTime        = 1000;      
      static const uint32_t TUserEOFE       = 0x1;

      // Thread tracking
      pthread_t _rxThread;
      pthread_t _workThread;
      pthread_t _txThread;

      // Thread Control
      bool _rxThreadEn;
      bool _workThreadEn;
      bool _txThreadEn;

      // RX queue
      CommQueue * _rxQueue;
      uint32_t    _rxPend;

      // Work 
      CommQueue * _workQueue;
      CommQueue * _relQueue;

      // TX Queue
      CommQueue * _txReqQueue;
      CommQueue * _txAckQueue;
      uint32_t    _txPend;

      // Config
      Config volatile _config;

      uint32_t _rxSize;
      uint32_t _txSize;

      // Status Counters
      Counters volatile      _counters;
      Counters volatile _last_counters;

      uint32_t _rxSequence;

      struct timeval _lastTime;
 
      // Device interfaces
      int32_t     _fd;
      uint32_t    _bSize;
      uint32_t    _bCount;
      uint8_t  ** _sampleData;

      // Static methods for threads
      static void * rxRunRaw   ( void *p );
      static void * workRunRaw ( void *p );
      static void * txRunRaw   ( void *p );

      // Class methods for threads
      void rxRun();
      void workRun();
      void txRun();

      // Network interfaces
      int32_t             _txFd;
      uint32_t            _txSequence;
      struct sockaddr_in  _txServerAddr;

   public:

      // Class create
      DaqBuffer ();

      // Class destroy
      ~DaqBuffer ();

      // Method to reset configurations to default values
      void hardReset ();

      // Open a dma interface and start threads
      bool open ( string devPath);

      // Close and stop threads
      void close ();

      // Set config
      void setConfig ( 
            uint32_t blowOffDmaData,
            uint32_t blowOffTxEth,
            uint32_t naccept,
            uint32_t nframes);

      // Set run mode
      void setRunMode ( RunMode mode );

      // Get Status
      void getStatus(struct BufferStatus *status);

      // Reset counters
      void resetCounters();

      // Open connection to the server
      bool enableTx ( const char * addr, const uint16_t port);

      // Close connection to the server
      void disableTx ( );

      // Start of run
      void startRun () 
      {
         _rxSequence  = 0;
         _txSequence  = 0;
         return;
      }
};

#endif


