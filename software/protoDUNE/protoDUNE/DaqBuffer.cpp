//////////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////


// ----------------------------------------------------------------------
// 
// HISTORY
// 
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2017.05.11 jjr Added code to monitor the WIB packets for errors in the
//                timestamp and convert counts.
// 2016.10.28 jjr Added the triggering configuration parameters naccept
//                and nframe
// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
// -- Must ensure the the print macros are defined
// ----------------------------------------------------------------------
#define   __STDC_FORMAT_MACROS

 
#include "DaqBuffer.h"
#include "FrameBuffer.h"
#include <AxisDriver.h>
#include "AxiBufChecker.h"

#include "List-Single.hh"

typedef uint32_t __s32;
typedef uint32_t __u32;


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#ifdef RTEMS
#include <rtems/libio.h>
#endif

using namespace std;

class Data
{
public:
   uint64_t data[1];
};


#define MAX_DEST 2

/* ---------------------------------------------------------------------- */
class TimingMsg
{
public:
   TimingMsg () { return; }

public:
   enum Type
   {
      SpillStart = 0,  /*!< Start of spill                               */
      SpillEnd   = 1,  /*!< End   of spill                               */
      Calib      = 2,  /*!< Calibration trigger                          */
      Trigger    = 3,  /*!< Physics trigger                              */
      TimeSync   = 4   /*!< Timing resynchonization request              */
   };

public:
   enum State
   {
      Reset         = 0x0 , /*!< W_RST, -- Starting state after reset    */
      WaitingSfpLos = 0x1,  /*!< when W_SFP, -- Waiting for SFP LOS 
                                 to go low                               */
      WaitingCdrLock= 0x2,  /*!< when W_CDR, -- Waiting for CDR lock     */
      WaitingAlign  = 0x3,  /*!< when W_ALIGN, -- Waiting for comma 
                                  alignment, stable 50MHz phase          */
      WaitingFreq   = 0x4,  /*!< W_FREQ, -- Waiting for good frequency 
                                 check                                   */
      WaitingLock   = 0x5,  /*!< when W_LOCK, -- Waiting for 8b10 decoder
                                 good packet                             */
      WaitingGpsTs  = 0x6,  /*!< when W_RDY, -- Waiting for time stamp
                                 initialisation                          */
      Running       = 0x8,  /*!< when RUN, -- Good to go                 */
      ErrRx         = 0xc,  /*!< when ERR_R, -- Error in rx              */
      ErrGpsTs      = 0xd,  /*!< when ERR_T; -- Error in time stamp check*/
   };
 

public:
   uint64_t timestamp () const { return m_timestamp; }
   uint32_t  sequence () const { return  m_sequence; }
   int           type () const { return (m_tsw >> 0) & 0xf; }
   int          state () const { return (m_tsw >> 4) & 0xf; }
   bool    is_trigger () const 
   { 
      return (m_tsw & 0xff) == ((Running << 4) | Trigger);
   };

public:
   uint64_t m_timestamp;  /*!< 64-bit GPS timestamp                       */
   uint32_t  m_sequence;  /*!< Messaage sequence number                   */
   uint32_t       m_tsw;  /*!< Type and state word                        */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class LatencyList : public List<FrameBuffer>
{
   static const int Depth = 10 - 1;
   static const bool Debug = false;
public:
   LatencyList () :
      List<FrameBuffer> (),
      m_remaining (Depth)
      {
         fprintf (stderr,
                  "Initializing latency list: %p %p:%p\n",
                  this,
                  m_flnk,
                  m_blnk);
         return;
      }

public:
   void init ()
   {
      List<FrameBuffer>::init ();
      m_remaining = Depth;
   }


   void resetToEmpty ()
   {
       m_remaining = Depth;
       return;
   }

   void replace (List<FrameBuffer>::Node *node, int fd)
   {
      // Add this node
      if (Debug) fprintf (stderr, "Adding[%3d] %p", node->m_body.index (), node);
      insert_tail (node);

      // Check if filled the latency depth
      if (m_remaining <= 0)
      {
         // Depth exceeded, remove the oldest node and free it
         node = remove_head ();

         if (Debug) fprintf (stderr, "  Returning[%3d] %p\n", node->m_body.index (), node);
         dmaRetIndex (fd, node->m_body.index());
      }
      else
      {
         // Not filled yet, but have eaten up a spot
         m_remaining -= 1;
         if (Debug) fprintf (stderr, " Remaining %d\n", m_remaining);
      }

      return;
   }

public:
   int  m_remaining;  /*!< The current number of nodes                        */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class Event
{
public:
   Event () { return; }
   Event (uint64_t beg, uint64_t end) : m_limits (beg, end) 
   {
      m_list[0].init ();
      m_list[1].init ();
      return; 
   }


public:
   typedef List<FrameBuffer>::Node Contribution; 

public:
   class TimestampLimits
   {
   public:

      TimestampLimits () : 
         m_beg (0),
         m_end (0)
      {
         return;
      }

      TimestampLimits (uint64_t beg, uint64_t end) :
         m_beg (beg),
         m_end (end)
      {
         return;
      }

      void set (uint64_t beg, uint64_t end)
      {
         m_beg = beg;
         m_end = end;
         return;
      }

   public:
      uint64_t m_beg;
      uint64_t m_end;
   };


   void init (int index)
   {
      m_list[0].init ();
      m_list[1].init ();
      m_index = index;
      m_ctb   = 0;
   }      

   void setWindow (uint64_t beg, uint64_t end)
   {
      fprintf (stderr,
               "Trigger Window %16.16" PRIx64 " -> %16.16" PRIx64 "\n", beg, end);
      m_limits.set (beg, end);
      return;
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- */
   bool seedAndDrain (LatencyList *lists, int fd)
   {
      bool found = false;

      // Initialize the pending list to waiting for all possible destinations
      m_ctb = (1 << MAX_DEST) - 1;

      // Loop over the latency lists of all the destinations
      for (int idx = 0; idx < MAX_DEST; idx++)
      {
         LatencyList             *list =  &lists[idx];
         List<FrameBuffer>::Node *flnk = list->m_flnk;

         fprintf (stderr,
                  "SeedAndDrain[%d] list = %p : %p:%p (check empty)\n", 
                  idx,
                  list,
                  flnk,
                  list->m_blnk);


         // If the latency list is empty, need to initialize the event list
         if (flnk == reinterpret_cast<decltype (flnk)>(list))
         {
            m_list[idx].init ();
            fprintf (stderr,
                     "SeedAndDrain[%d] list = %p : %p (empty)\n", 
                     idx,
                     m_list[idx].m_flnk,
                     m_list[idx].m_blnk);

            continue;
         }


         // ----------------------------------------------------------------
         // Find the first node that has a 
         //       timestamp >= lower limit 
         // of the event window
         // 
         // Because these occur before this trigger and all future triggers
         // occur at a later time and are not allowed to overlap, nodes 
         // before the lower limit of the event window can be freed.  
         // ---------------------------------------------------------------
         while (1)
         {
            // Is the beginning timestamp of this packet > lower limit of event window
            uint64_t begTime = flnk->m_body._ts_range[0];
            if (begTime >= m_limits.m_beg)
            {
               m_list[idx] = *list;
               fprintf (stderr,
                        "SeedAndDrain[%d] list = %p : %p\n", 
                        idx,
                        m_list[idx].m_flnk,
                        m_list[idx].m_blnk);

               // ------------------------------------------
               // Since triggers are not allowed to overlap, 
               // the latency list must be reset to empty
               // ------------------------------------------
               list->init ();
               found  = true;
               break;
            }
            else
            {
               // This packet occurred before the trigger window opened
               int index = flnk->m_body.index ();
            
               // Get the next node
               flnk = flnk->m_flnk;

               fprintf (stderr,
                        "Discarding[%1d] node = %p beg: %16.16" PRIx64 "\n",
                        idx, flnk, flnk->m_body._ts_range[0]);
               
               // Remove and free the node we were working on
               list->remove_head ();
               dmaRetIndex (fd, index);

               // -------------------------------------------
               // Check if the latency list is exhausted.
               // 
               // This really should not happen.  Either
               //   1. This contributor is not enabled.
               //      -> This means the latency list 
               //         will be empty and this is checked
               //         for before this loop is entered.
               //   2. Lower trigger window edge is after
               //      the last entry.
               //      entry.
               //      -> This probably means the trigger
               //         arrived too late or possibly that
               //         the lower limit of the trigger
               //         window is nearly 0, so one does not
               //         have to reach back in time
               // -------------------------------------------
               if (flnk == reinterpret_cast<decltype (flnk)>(list))
               {
                  break;
               }
            }
         }
      }

      return found;
   }
   /* ---------------------------------------------------------------------- */

   enum Fate 
   {
      Rejected  = -1,
      Added     =  0,
      Completed =  1,
      Overrun   =  2
   };

   Fate add (List<FrameBuffer>::Node *node, int dest)
   {
      static int Count[2] = {0, 0};
      uint32_t ctb_mask = (1 << dest);

      //fprintf (stderr, 
      //         "Adding: %2d:%1.1x %16.16" PRIx64 " >= %16.16" PRIx64 "\n",
      //         dest, m_ctb, endTime, m_limits.m_end);


      // Check if this contribution is pending
      if (m_ctb & ctb_mask) 
      {
         uint64_t  begTime = node->m_body._ts_range[0];
         uint64_t  endTime = node->m_body._ts_range[1];

         // Is this within the trigger window
         if (begTime <= m_limits.m_end)
         {
            // Yes, add to the list 
            fprintf (stderr, 
                     "Adding node[%1d.%3d] %p flnk:%p blnk:%p end: %16.16" PRIx64 " -> "
                     "%16.16" PRIx64 "\n",
                     dest,
                     Count[dest]++,
                     node, 
                     m_list[dest].m_flnk,
                     m_list[dest].m_blnk,
                     begTime,
                     endTime);
            m_list[dest].insert_tail (node);


            // Is this end time after the trigger window
            if (endTime >= m_limits.m_end)
            {
               // ---------------------------------
               // Yes, this contributor is complete
               // Remove it from the pending list
               // ---------------------------------
               m_ctb &= ~ctb_mask;
               if (m_ctb == 0) { Count[0] = Count[1] = 0; return Completed; }

               // This did not complete the event
               else            {                          return     Added; }
            }
            else
            {
               // ------------------------------------
               // No, this contributor is not complete
               // It was just added
               // -----------------------------------
               return Added;
            }

         }
         else
         {
            // ------------------------------------------------
            // Outside the window
            // This should happen iff there are dropped packets.
            // If all is in sequence, the previous should have
            // completed this contributor.
            // ------------------------------------------------
            return Rejected;
         }




      }
      else
      {
         // --------------------------------------------------
         // This contributor Is already completed
         // Just waiting for the other contributors to come in
         // --------------------------------------------------
         if (true)
         {
            // If have waited too long, then declare this event complete
            fprintf (stderr, "Overrun\n");
            Count[0] = 0;
            Count[1] = 0;
            return Overrun;
         }
         else
         {
            // --------------------
            // Willing to wait more
            // --------------------
            return Rejected;
         }
      }
   }


   void free (int fd)
   {
      int dest;
      int  cnt;

      // Return the frames
      for (int idx = 0; idx < MAX_DEST; idx++)
      {
         List<FrameBuffer>::Node *node = m_list[idx].m_flnk;
         List<FrameBuffer>::Node *end  = reinterpret_cast<decltype(end)>
                                                          (&m_list[idx]);

         int count = 0;
         while (node != end)
         {
            // Return the underlying DMA buffer
            int index = node->m_body.index ();

            if ((unsigned int)index > 900)
            {
               fprintf (stderr, "Bad [%d.%d] index on free %8.8x\n", 
                        idx, count++, index);
               break;
            }


            // The frame buffer associated with this event 
            // must be the last to be freed
            if (index != m_index)
            {
               fprintf (stderr, "Node[%d.%d] freeing index %d\n", idx, count++, index);
               dmaRetIndex (fd, index);
            }
            else
            {
               dest =   idx;
               cnt  = count++;
            }

            if (count > 32) break;

            node = node->m_flnk;
         }
      }

      fprintf (stderr, "Node[%d.%d] freeing index %d \n", dest, cnt, m_index);
      dmaRetIndex (fd, m_index);

   }


   class Trigger
   {
   public:
      Trigger () { return; }

      enum Type
      {
         Software = 0,  /*!< Software trigger */
         Hardware = 1   /*!< Hardware trigger */
      };

   public:
      void init (uint64_t timestamp, uint32_t sequence, uint32_t opaque)
      {
         m_timestamp = timestamp;
         m_sequence  = sequence;
         m_opaque    = opaque;
         m_type      = Software;
         return;
      }

      void init (TimingMsg const *msg)
      {
         m_timestamp = msg->m_timestamp;
         m_sequence  = msg->m_sequence;
         m_opaque    = msg->m_tsw;
         m_type      = Hardware;
      }

   public:
      uint64_t m_timestamp;  /*!< The trigger timestamp                   */
      uint32_t  m_sequence;  /*!< The trigger sequence number             */
      uint32_t    m_opaque;  /*!< An 32-bit value that is type dependent  */
      enum Type     m_type;  /*!< The trigger type                        */
   };

      
      
public:
   TimestampLimits    m_limits;  /*!< Event time window                   */
   Trigger           m_trigger;  /*!< Event triggering information        */
   List<FrameBuffer> m_list[2];  /*!< List of the contributors            */
   uint32_t              m_ctb;  /*!< Bit mask of incomplete contributors */
   uint16_t            m_index;  /*!< Associated frame buffer index       */
};
/* ---------------------------------------------------------------------- */

#define DEBUG_PRINT 0

// Class create
DaqBuffer::DaqBuffer () {
   _rxThreadEn     = false;
   _workThreadEn   = false;
   _txThreadEn     = false;
   _rxPend         = 0;
   _rxQueue        = NULL;
   _relQueue       = NULL;
   _txReqQueue     = NULL;
   _txAckQueue     = NULL;
   _txPend         = 0;

   hardReset();

   _fd             = -1;
   _bSize          = 0;
   _bCount         = 0;
   _sampleData     = NULL;

   _txFd       = -1;
   _txSequence = 0;

   resetCounters ();
}



// Class destroy
DaqBuffer::~DaqBuffer () {
   this->disableTx();
   this->close();
}


/* ---------------------------------------------------------------------- */
class SoftTrigger
{
public:
   SoftTrigger () : m_naccept (0), m_nwait (0) { return; }

   void configure (int32_t naccept, int32_t nwait)
   {
      m_naccept = naccept;
      m_nwait   =   nwait;
   }

   bool check (uint64_t timestamp[2], volatile DaqBuffer::Config &config)
   {
      uint32_t beg = timestamp[0] % (1024 * 1000 * 1000);
      uint32_t end = timestamp[1] % (1024 * 1000 * 1000);

      // If the end phase < beg phase, then phase went through 0
      if (end < beg) 
      {
         uint64_t delta = timestamp[1] - timestamp[0];
         fprintf (stderr,
                  "Check %16.16" PRIx64 ":%8.8" PRIx32 ":"
                  "%16.16" PRIx64 ":%8.8" PRIx32 " trigger -> yes  %16.16" PRIx64 "\n",
                  timestamp[0], beg, timestamp[1], end, delta);
         //fprintf (stderr, " -> yes\n");
         return true;
      }
      else 
      {
         //fprintf (stderr, " -> no\n");
         return false;
      }
   }
      


   bool declare (volatile DaqBuffer::Config &config)
   {
      // Check if the wait period has expired
      if (m_nwait-- <= 0)
      {
         // Rearm
         m_naccept = config._naccept - 1;
         m_nwait   = config._nframes - 1;

         //printf ("Arming: naccept/nwait = %4" PRId32 "/%4" PRId32 "\n",
         //        m_naccept, m_nwait);
      }

      if (m_naccept >= 0)
      {
         //printf ("Posting[%3d]: naccept/nwait = %4" PRId32 "/%4" PRId32 "\n",
         //       index, m_naccept, m_nwait);
         return true;
      }
      else
      {
         return false;
      }
   }

   void reduce ()
   {
      m_naccept -= 1;
   }
   

private:
   int32_t  m_naccept;
   int32_t    m_nwait;
};
/* ---------------------------------------------------------------------- */




// Method to reset configurations to default values
void DaqBuffer::hardReset () {
   _config._runMode         = IDLE;
   _config._blowOffDmaData  = 0;
   _config._blowOffTxEth    = 0;
   _config._naccept         = 10;
   _config._nframes         = 2048;
}

// Open a dma interface and start threads
bool DaqBuffer::open ( string devPath ) {
   struct sched_param txParam;
   uint8_t mask[DMA_MASK_SIZE];

   puts ("-------------------------\n"
         "JJ's version of rceServer\n"
         "-------------------------\n");

   this->close();

   fprintf (stderr, "Dma device = %s\n", devPath.c_str ());

   // Open device
#ifndef RTEMS // FIX
//   if ( (_fd = ::open(devPath.c_str(),O_RDWR|O_NONBLOCK)) < 0 ) {
   //if ( (_fd = ::open("/dev/axi_stream_dma_0", O_RDWR|O_NONBLOCK)) < 0 ) {
   if ( (_fd = ::open("/dev/axi_stream_dma_2", O_RDWR|O_NONBLOCK)) < 0 ) {
      fprintf(stderr,"DaqBuffer::open -> Erroro pening device\n");
      return(false);
   }
#else
   _fd = -1;
#endif

   /*
    | 2017.04.08 -- jjr
    | -----------------
    | Adjustments for the V2 driver
   */
   dmaInitMaskBytes(mask);
   //dmaAddMaskBytes(mask,128); // HLS[0]
   //dmaAddMaskBytes(mask,129); // HLS[1]

   fprintf (stderr, "Setting masks for 0,1,2\n");
   dmaAddMaskBytes(mask,0); // HLS[0]
   dmaAddMaskBytes(mask,1); // HLS[1]
   dmaAddMaskBytes(mask,2); // trigger ??

   if  ( dmaSetMaskBytes(_fd,mask) < 0 ) {
      ::close(_fd);
      fprintf(stderr,"DaqBuffer::open -> Unable to set mask\n");
      return(false);
   }


   // Get DMA buffers
   if ( (_sampleData = (uint8_t **)dmaMapDma(_fd,&_bCount,&_bSize)) == NULL ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to map to dma buffers\n");
      this->close();
      return(false);
   }

   _rxCount = dmaGetRxBuffCount (_fd);
   _txCount = dmaGetTxBuffCount (_fd);

   // Create queues
   _workQueue    = new CommQueue(RxFrameCount+5,true);
   _relQueue     = new CommQueue(RxFrameCount+5,true);
   _rxQueue      = new CommQueue(RxFrameCount+5,false);
   _txReqQueue   = new CommQueue(TxFrameCount+5,true);
   _txAckQueue   = new CommQueue(TxFrameCount+5,true);


   // Create and populate the RX Queue Entries
   FrameBuffer *frames = new FrameBuffer[RxFrameCount];
   if ( frames == NULL ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to allocate FrameBuffers\n");
      this->close();
      return(false);
   }

   for ( uint32_t x=0; x < RxFrameCount; x++ ) {
      _rxQueue->push (&frames[x]);
   }


   // Create RX thread
   _rxThreadEn = true;
   if ( pthread_create(&_rxThread,NULL,rxRunRaw,(void *)this) ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to create rxThread\n");
      _rxThreadEn = false;
      this->close();
      return(false);
   }


#ifdef ARM
   else pthread_setname_np(_rxThread,"buffRxThread");
#endif


   
   // Set priority
#if 1
   struct sched_param rxParam;
   int policy;
   struct timeval cur;
   gettimeofday (&cur, NULL);
   pthread_getschedparam (_rxThread, &policy, &rxParam);
   fprintf (stderr, "Policy = %d priority = %d time = %lu.%06lu\n", 
            policy, rxParam.sched_priority, cur.tv_sec, cur.tv_usec);

   //rxParam.sched_priority = 3;
   //policy                 = SCHED_FIFO;
   //pthread_setschedparam(_rxThread, policy, &rxParam);
   //pthread_getschedparam (_rxThread, &policy, &rxParam);
   //fprintf (stderr, "Policy = %d priority = %d\n", 
   //         policy, rxParam.sched_priority);


#endif

   // Create Work Thread
   _workThreadEn = true;
   if ( pthread_create(&_workThread,NULL,workRunRaw,(void *)this) ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to create workThread\n");
      _workThreadEn = false;
      this->close();
      return(false);
   } 


#ifdef ARM
   else pthread_setname_np(_workThread,"buffWorkThread");
#endif




   // Set priority
#if 0
   struct sched_param workParam;
   workParam.sched_priority = 2;
   pthread_setschedparam(_workThread, SCHED_FIFO, &workParam);
#endif



   // Create TX thread
   _txThreadEn = true;
   if ( pthread_create(&_txThread,NULL,txRunRaw,(void *)this) ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to create txThread\n");
      _txThreadEn = false;
      this->close();
      return(false);
   } 

#ifdef ARM
   else pthread_setname_np(_txThread,"buffTxThread");
#endif

   // Set priority
   txParam.sched_priority = 1;
   pthread_setschedparam(_txThread, SCHED_FIFO, &txParam);

   // Init
   resetCounters ();

   printf("DaqBuffer::open -> Running with %i + %i = %i (tx+rx=total) buffers of %i bytes\n",
          _txCount, _rxCount, _bCount,_bSize);
   return(true);
}


// Close and stop threads
void DaqBuffer::close () {
   FrameBuffer *tempBuffer;

   // Disable transmit
   disableTx();

   // Stop tx thread
   if ( _txThreadEn ) {
      _txThreadEn = false;
      pthread_join(_txThread, NULL);
   }

   // Stop work thread
   if ( _workThreadEn ) {
      _workThreadEn = false;
      pthread_join(_workThread, NULL);
   }

   // Stop rx thread
   if ( _rxThreadEn ) {
      _rxThreadEn = false;
      pthread_join(_rxThread, NULL);
   }

   // Unmap user space
   if ( _sampleData != NULL ) dmaUnMapDma(_fd,(void **)_sampleData);
   _sampleData = NULL;

   // Close the device
   if ( _fd >= 0 ) ::close(_fd);
   _fd = -1;

   // Delete Frame Queue Entries
   if ( _rxQueue   != NULL ) {
      while ( (tempBuffer = (FrameBuffer *)_rxQueue->pop()) != NULL )
         delete (tempBuffer);
      delete _rxQueue;
   }
   _rxQueue   = NULL;

   // Delete queues
   if ( _workQueue    != NULL ) delete _workQueue;
   if ( _relQueue     != NULL ) delete _relQueue;
   if ( _txReqQueue   != NULL ) delete _txReqQueue;
   if ( _txAckQueue   != NULL ) delete _txAckQueue;
   _workQueue    = NULL;
   _relQueue     = NULL;
   _txReqQueue   = NULL;
   _txAckQueue   = NULL;
}

// Static raw rx thread run
void * DaqBuffer::rxRunRaw ( void *p ) {
   DaqBuffer *buff = (DaqBuffer *)p;
   buff->rxRun();
   pthread_exit(NULL);
   return(NULL);
}
/* ---------------------------------------------------------------------- */




/* ====================================================================== */
/* BEGIN: FrameDiagnostics                                                */
/* ---------------------------------------------------------------------- *//*!

   \brief Collect all the frame diagnostics in one place
                                                                          */
/* ---------------------------------------------------------------------- */
class FrameDiagnostics
{
public:
   FrameDiagnostics (uint32_t   freqReceived, 
                     uint32_t freqTimingDump,
                     uint32_t   freqDataDump,
                     uint32_t  freqDataCheck);

   void dump_received    (int           index,
                          int            dest,
                          int          rxSize);

   void dump_timingFrame (void const     *data,
                          int           nbytes);

   void dump_dataFrame   (void const     *data,
                          int           nbytes,
                          int             dest);

   void check_dataFrame  (uint64_t const *data,
                          int           nbytes,
                          unsigned int    dest,
                          int           sample);
public:
   struct Activator
   {
      uint32_t  m_freq;
      uint32_t  m_count;

      void init (uint32_t freq)
      {
         m_freq  = freq;
         m_count = freq == 0xffffffff ? freq : 0;
      }


      bool declare ()
      {
         if (m_count--) return false;
         else           { m_count = m_freq; } return true;
      }
      

   };
   
   Activator         m_received;
   Activator  m_timingFrameDump;
   Activator  m_dataFrameDump[2];
   Activator m_dataFrameCheck[2];
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief  Constructor for the various frame diagnostic routines

   \param[in] freqReceived   The frequency to dump received information,
   \param[in] freqTimingDump The frequency to dump timing frames
   \param[in] freqDataDump   The frequency to dump a data frame, 
   \param[in] freqDataCheck  The frequency to check a data frame, 

   \par
    If a dump frequency is set to 100, this means to activate the
    diagnostic every 100th time.
                                                                          */
/* ---------------------------------------------------------------------- */
FrameDiagnostics::FrameDiagnostics (uint32_t freqReceived, 
                                    uint32_t freqTimingDump,
                                    uint32_t freqDataDump, 
                                    uint32_t freqDataCheck)
{
   // ----------------------------------
   // Received DMA dumper
   // -------------------
   m_received.init (freqReceived);
   // ----------------------------------


   // ----------------------------------
   // Timing frame dumper
   // -------------------
   m_timingFrameDump.init (freqTimingDump);
   // ----------------------------------
      

   // ----------------------------------
   // Data frame dumper
   // -------------------
   m_dataFrameDump[0].init (freqDataDump);
   m_dataFrameDump[1].init (freqDataDump);
   // ----------------------------------


   // ----------------------------------
   // Data frame checker
   // -------------------
   m_dataFrameCheck[0].init (freqDataCheck);
   m_dataFrameCheck[1].init (freqDataCheck);
   // ----------------------------------


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Dump the information about a received DMA transaction

   \param[in]  index  The frame index
   \param[in]   dest  The destination stream
   \param[in  rxSize  The received size
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_received (int index, int dest, int rxSize)
{
   if (!m_received.declare ()) return;

   printf ("Index:%4u  dest: %3u rxSize = %6u\n", index, dest, rxSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Primitive hex dump of the received timing frame
 *
 *  \param[in]   data The timing frame data
 *  \param[in] nbytes The number bytes in the data
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_timingFrame (void const *data, 
                                                int       nbytes)
{
   struct TimingMsg
   {
      uint64_t gps;
      uint32_t seq;
      uint32_t ctl;
   };

   static struct TimingMsg LastMsg = { 0, 0, 0};

   if (!m_timingFrameDump.declare ()) return;

   TimingMsg const *s = reinterpret_cast<TimingMsg const *>(data);

   int64_t deltaGps = s->gps - LastMsg.gps;
   int32_t deltaSeq = s->seq - LastMsg.seq;

   printf ("t = %16.16" PRIx64 " %8.8" PRIx32 " %8.8" PRIx32 " "
           "%16" PRId64 " %8" PRId32" \n",
           s->gps, s->seq, s->ctl, deltaGps, deltaSeq);

   LastMsg = *s;

   /*
   for (int idx = 0; idx < ns; idx++)
   {
      if ((idx%16) == 0) printf ("t[%3d]:", idx);
      printf (" %4.4" PRIx16, s[idx]);
      if ((idx%16) == 15) putchar ('\n');
   }

   // If have left off in the middle of a line
   if (ns % 16) putchar ('\n');
   */
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Primitive hex dump of the received data frame
 *
 *  \param[in]   data The frame data
 *  \param[in] nbytes The number bytes in the data
 *  \param[in]   dest Which stream
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_dataFrame (void const *data, 
                                              int       nbytes,
                                              int         dest)
{
   if (!m_dataFrameDump[dest].declare ()) return;

   uint16_t const *s = (uint16_t const *)data;
   int            ns = nbytes / sizeof (*s);

   for (int idx = 0; idx < ns; idx++)
   {
      if ((idx%16) == 0) printf ("d[%3d]:", idx);
      printf (" %4.4" PRIx16, s[idx]);
      if ((idx%16) == 15) putchar ('\n');
   }

   // If have left off in the middle of a line
   if (ns % 16) putchar ('\n');
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Performs basic integrity checks on the frame.
 *
 *  \param[in]   data The frame data
 *  \param[in] nbytes The number bytes in the data
 *  \param[in]   dest The originating module, 0 or 1
 *  \param[in] sample The sampling interval. Since not all received 
 *                    packets are checked, this is used to predict the
 *                    values of the next packet that will be checked.
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::check_dataFrame (uint64_t const *data,
                                               int           nbytes,
                                               unsigned int    dest,
                                               int           sample)
{
#  define N64_PER_FRAME 30
#  define NBYTES (unsigned int)(((1 + N64_PER_FRAME * 1024) + 1) \
                * sizeof (uint64_t))

   if (!m_dataFrameCheck[dest].declare ()) return;

   struct History_s
   {
      uint64_t  timestamp;
      uint16_t convert[2];
   };

   static struct History_s History[2] = { {0, {0, 0}}, {0, {0,0}} };
   static unsigned int        Counter = 0;
   int                            n64 = nbytes / sizeof (*data) - 2;
  
   //uint64_t hdr = data[0];
   data += 1;


   // ------------------------------------
   // Check for the correct amount of data
   // ------------------------------------
   if (nbytes != NBYTES)
   {
      printf ("Aborting @ %2u.%6u %u != %u incorrect amount of data received\n",
              dest, Counter, nbytes, NBYTES);
      return;
   }

   
   // -------------------------------------------------------------
   // Seed predicted sequence number with either
   //   1) The GPS timestamp of the previous packet
   //   2) The GPS timestamp of the first frame
   // -------------------------------------------------------------
   uint16_t   predicted_cvt_0;
   uint16_t   predicted_cvt_1;
   uint64_t   predicted_ts = History[dest].timestamp;
   if (predicted_ts)
   {
      predicted_ts    = data[ 1];
      predicted_cvt_0 = data[ 2] >> 48;
      predicted_cvt_1 = data[16] >> 48;
   }
   else
   {
      predicted_cvt_0 = History[dest].convert[0];
      predicted_cvt_1 = History[dest].convert[1];
   }


   // --------------------
   // Loop over each frame
   // --------------------
   unsigned int    frame = 0;
   for (int idx = 0; idx < n64; idx += N64_PER_FRAME)
   {
      uint64_t d = data[idx];

      // -------------------------
      // Check the comma character
      // -------------------------
      if ((d & 0xff) != 0xbc)
      {
         printf ("Error frame @ %2u.%6u.%4u: %16.16" PRIx64 "\n",
                 dest, Counter, frame, d);
      }

      // -----------------------------------
      // Form and check the sequence counter
      // ----------------------------------
      uint64_t timestamp = data[idx+1];
      uint16_t     cvt_0 = data[idx +  2] >> 48;
      uint16_t     cvt_1 = data[idx + 16] >> 48;

      int error = ((timestamp != predicted_ts   ) << 0)
                | ((cvt_0     != predicted_cvt_0) << 1)
                | ((cvt_1     != predicted_cvt_1) << 2);

      if (error)
      {
         printf ("Error  seq @ %2u.%6u.%4u: "
                 "ts: %16.16" PRIx64 " %c= %16.16" PRIx64 " "
                 "cvt: %4.4" PRIx16 " %c= %4.4" PRIx16 " "
                 "cvt: %4.4" PRIx16 " %c= %4.4" PRIx16 "\n",
                 dest, Counter, frame, 
                 timestamp, (error&1) ? '!' : '=', predicted_ts,
                 cvt_0    , (error&2) ? '!' : '=', predicted_cvt_0,
                 cvt_1    , (error&4) ? '!' : '=', predicted_cvt_1);

         // -----------------------------------------------------
         // In case of error, resynch the predicted to the actual
         // -----------------------------------------------------
         predicted_ts    = timestamp;
         predicted_cvt_0 = cvt_0;
         predicted_cvt_1 = cvt_1;
      }
      else if (1 && ((Counter % (32768/sample)) == 0) && ((frame % 256) == 0))
      {
         // ------------------------------------------
         // Print reassuring message at about 1/2-1 Hz
         // ------------------------------------------
         printf ("Spot check @ %2u.%6u.%4u: "
                 "ts: %16.16" PRIx64 " == %16.16" PRIx64 " "
                 "cvt: %4.4" PRIx16 " == %4.4" PRIx16 " "
                 "cvt: %4.4" PRIx16 " == %4.4" PRIx16 "\n",
                 dest, Counter, frame, 
                 timestamp, predicted_ts,
                 cvt_0    , predicted_cvt_0,
                 cvt_1    , predicted_cvt_1);
      }


      // -------------------------------------
      // Advance the predicted sequence number
      // Advance the frame counter
      // -------------------------------------
      predicted_ts     = timestamp + 500;
      predicted_cvt_0 += 1;
      predicted_cvt_1 += 1;
      frame           += 1;
   }

   // -----------------------------------------------
   // Keep track of the number of times called and
   // the expected sequence number of the next packet
   // for this destination.
   // -----------------------------------------------
   Counter                 += sample;
   History[dest].timestamp  = predicted_ts    + (sample - 1) * 500;
   History[dest].convert[0] = predicted_cvt_0 +  sample - 1;
   History[dest].convert[1] = predicted_cvt_1 +  sample - 1;

   return;
}
/* ---------------------------------------------------------------------- */
/* END:  FrameDiagnostic                                                  */
/* ====================================================================== */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief   Post the data to an output queue
 * \return  True, if successful, else false
 *
 *
 * \param[in:out]    rxQueue  The source of frame buffers
 * \param[in:out]  workQueue  The destination queue
 * \param[in]           data  The data to be posted
 * \param[in]          index  The sample index
 * \param[in]         nbytes  The number of bytes in the data to be posted
 * \param[in]    rx_sequence  The received sequence number     
 *
\* ---------------------------------------------------------------------- */
static inline bool post (Event          event,
                         CommQueue   *rxQueue,
                         CommQueue *workQueue,
                         uint8_t        *data,
                         uint32_t       index,
                         uint32_t      nbytes,
                         uint32_t rx_sequence)
{
   // Attempt to allocate a frame buffer
   FrameBuffer  *tempBuffer = (FrameBuffer *)rxQueue->pop();

   // Check if allocation was successful
   if (tempBuffer)
   {
      // Successfully allocated a buffer, try to prompt the data
      tempBuffer->setData (index, data, nbytes, rx_sequence);
      bool posted = workQueue->push (tempBuffer);

      return posted;
   }


   return false;
}
/* ---------------------------------------------------------------------- */
      






#  undef  MONITOR_RATE
#  define MONITOR_RATE 0


/* ====================================================================== */
#  if     MONITOR_RATE
/* ---------------------------------------------------------------------- *//*!

  \brief A debugging aid only.  It should be disable in production 
         releases.
                                                                          */
/* ---------------------------------------------------------------------- */
class MonitorRate
{
public:
   MonitorRate ()
   {
      gettimeofday (&m_begTime, NULL);
      memset (m_count, 0, sizeof (m_count));
   }


   void monitor (int dest)
   {
      struct timeval   difTime;
      struct timeval   curTime;
      gettimeofday (&curTime, NULL);
      timersub     (&curTime, &m_begTime, &difTime); 

      uint32_t interval = 1000 * 1000 * difTime.tv_sec + difTime.tv_usec;

      int idx = (dest <= MAX_DEST) ? dest+1 : 1+MAX_DEST+1;
      m_count[idx] += 1;

      if (interval >= 10* 1000 * 1000)
      {
         if ( (m_total & 0x1f) == 0)
         {
            fprintf (stderr,
                     " Empty + Dest=0 + Dest=1 + Dest=2 + Unknown = "
                     " Total/    usecs\n");
         }

         m_total       += 1;
         uint32_t total = m_count[0] + m_count[1] 
                        + m_count[2] + m_count[3] + m_count[4];
         fprintf (stderr, "%6u + %6u + %6u + %6u +  %6u = %6u/%9u\n", 
                  m_count[0], m_count[1], m_count[2], m_count[3], m_count[4],
                  total, interval);

         // Reset for next batch 
         m_begTime = curTime;
         memset (m_count, 0, sizeof (m_count));
      }
   }
   

private:
   struct timeval     m_begTime; /*!< Begin time of this sample           */
   uint32_t             m_total; /*!< Total number of calls               */
   uint32_t m_count[MAX_DEST+3]; /*!< Count of dest types                 */
};

/* ---------------------------------------------------------------------- */
# else
/* ---------------------------------------------------------------------- */

class MonitorRate
{
public:
   MonitorRate  ()         { return; }
   void monitor (int dest) { return; }
};

/* ---------------------------------------------------------------------- */
#  endif
/* ====================================================================== */




/* ---------------------------------------------------------------------- *//*!

  \brief Check the DMA buffers for readability

  \warning
   This method should be expunged when the root cause of these unreadable
   DMA buffers is found.

   Until then, this method will remove any such buffers from circulation
                                                                          */
/* ---------------------------------------------------------------------- */
void DaqBuffer::vetDmaBuffers ()
{
   // ----------------------------------------------
   // !!! KLUDGE !!! CHECK READBILITY OF THE BUFFERS
   // ----------------------------------------------
   uint16_t bad[_bCount];
   AxiBufChecker::check_buffers (bad, _sampleData, _bCount, _bSize);
   // ----------------------------------------------
      
      
   // -------------------------------------------------------
   // !!! KLUDGE !!!
   // Get rid of the bad buffers
   // ------------------------------
   int nbufs     =   _bCount;
   int nrxbufs   =   _rxCount;
   AxiBufChecker abc[_bCount];

   for (int idx = 0; idx < 2; idx++)
   {
      int nbad = AxiBufChecker::extreme_vetting (abc, 
                                                 _fd,
                                                 idx, 
                                                 _sampleData, 
                                                 nrxbufs,
                                                 nbufs, 
                                                 _bSize);
      nrxbufs -= nbad;
   }

   fprintf (stderr, "Done\n");

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Hokey routine to get the timestamp range of the packet.
  \return  The  timestamp of the initial WIB frame

  \param[in] range The timestamp range of this packet
  \param[in]  data The data frame

                                                                          */
/* ---------------------------------------------------------------------- */
static inline void getTimestampRange (uint64_t range[2],
                                      uint8_t const *data,
                                      uint32_t nbytes)
{
   uint64_t const *d64 =   reinterpret_cast<uint64_t const *>(data);



   // Skip header
   d64 += 1;
   uint64_t  begin = d64[1];


   d64 += nbytes/sizeof (uint64_t) - 1 - 30 - 1;
   uint64_t end = d64[1] + 1024 * 500;

   //  fprintf (stderr, 
   //         "Beg %16.16" PRIx64 " End %16.16" PRIx64 " %16.16" PRIx64 
   //         " %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 "\n",
   //           begin, end, d64[-1], d64[0], d64[1], d64[2]);

   range[0] = begin;
   range[1] =   end;

   return;
}
/* ---------------------------------------------------------------------- */






// Class method for rx thread running
void DaqBuffer::rxRun ()
{

   // Check for bad DMA buffers
   vetDmaBuffers ();

   const uint64_t WindowSize = 5 * 1000 * 1000;
   fd_set                    fds;
   bool        trgActive __attribute__ ((unused));

   trgActive = false;
   FD_ZERO (&fds);

   LatencyList latency[MAX_DEST];


   // Init 
   SoftTrigger      softTrigger;
   uint32_t      softTriggerCnt;

   //FrameDiagnostics diag (-1, 1, 16, 256);
   _rxPend = 0;   



   // -------------------------------------------------------------------
   // Request array of events, one for every possible trigger
   // Given that 
   //   1. there can't be more triggers than front-end frame buffers
   //   2. event buffers are cheap
   //
   // one event buffer will be allocated for every front-end frame buffer
   // -------------------------------------------------------------------
   Event *events = 
      reinterpret_cast<decltype(events)>(malloc (_bCount * sizeof (*events)));


   // Allocate enough frame buffer nodes to capture an all possible events
   List<FrameBuffer >::Node *fbs = 
      reinterpret_cast<decltype (fbs)>(malloc (_bCount * sizeof (*fbs)));
   for (decltype(_bCount)(idx) = 0; idx < _bCount; idx++)
   {
      // Initialize the one time only fields
      fbs[idx].m_body.setData  (_sampleData[idx]);
      fbs[idx].m_body.setIndex (idx);
      events[idx].init (idx);
   }

   fprintf (stderr, "Starting\n");

   // Debugging aid for monitoring the rates of various dest DMA buffers
   MonitorRate rate;

   FD_ZERO (&fds);

   Event::Trigger trigger;
   Event           *event = NULL;

   // Run while enabled
   while (_rxThreadEn) 
   {

      struct timeval timeout;   
      uint32_t         index;
      uint32_t         flags;
      uint32_t          dest;
      int32_t         rxSize;


      timeout.tv_sec  = 0;
      timeout.tv_usec = WaitTime;
      FD_SET  (_fd, &fds);


      // Wait for data or timeout and attempt to readout
      int nfds = select (_fd+1, &fds, NULL, NULL, &timeout); 
      if (nfds > 0)
      {
         rxSize = dmaReadIndex (_fd, &index, &flags, NULL, &dest);
      }
      else
      {
         rxSize = 0;
      }


      // Was this anything but a timeout
      if (rxSize > 0)
      {
         rate.monitor (dest);

         uint32_t lastUser = axisGetLuser(flags);


         //diag.dump_received (index, dest, rxSize);

         //////////////////////////////////////////////////////////////////////
         // Check if blowing off the DMA data or unrecognized packet
         if ((_config._blowOffDmaData != 0) || (dest > MAX_DEST) ) {

            // Return index value 
            dmaRetIndex(_fd, index);         
            
         //////////////////////////////////////////////////////////////////////
         // Check for an external trigger message
         } else if ( dest == 2 ) {

            // Count external triggers
            _counters._triggers++;
            //diag.dump_timingFrame ((uint64_t const *)_sampleData[index], rxSize);

            if (_config._runMode == TRIG)
            {
               // Allocate a buffer
               TimingMsg const *tmsg = reinterpret_cast<decltype (tmsg)>
                                                      (&_sampleData[index]);

               // Is this a trigger message?
               if (tmsg->is_trigger ())
               {
                  if (!trgActive)
                  {
                     trigger.init (tmsg);
                     event     = NULL;
                     trgActive = true;
                  }

                  // Return the trigger message frame
                  dmaRetIndex (_fd, index);
               }
            }
            else
            {
               // External trigger not active
               dmaRetIndex(_fd, index);
            }
            
         //////////////////////////////////////////////////////////////////////
         // Else this is HLS data (dest < MAX_DEST) 
         } else {
            _counters._rxCount++;
            _counters._rxTotal += rxSize;


            _rxSequence += 1;

            // -----------------------------------------------
            // Check every 256 received packets in each stream
            // -----------------------------------------------
            //diag.check_dataFrame ((uint64_t const *)_sampleData[index],
            //                      rxSize,
            //                      dest,
            //                      256);

            //diag.dump_dataFrame (_sampleData[index], rxSize, dest);

            _rxSize = rxSize; 


            // Check if there is error
            if( lastUser & TUserEOFE )
            {
               _counters._rxErrors++;
               dmaRetIndex(_fd,index);

            // Try to Move the data
            } else {

               uint8_t  *data = _sampleData[index];
               uint64_t *dbeg = reinterpret_cast<decltype(dbeg)>(data);
               uint64_t *dend = reinterpret_cast<decltype(dbeg)>(data +
                                                              + rxSize - sizeof (*dend));

               uint32_t tlrSize = *reinterpret_cast<uint32_t const *>(dend) & 0xffffff;
               if (tlrSize != _rxSize)
               {
                  fprintf (stderr, 
                           "Error Frame %16.16" PRIx64 " -> " "%16.16" PRIx64 " %8.8x" PRIx32 "\n", 
                     *dbeg, *dend, _rxSize);

                  _counters._rxErrors++;
                  dmaRetIndex(_fd,index);
                  goto RELEASE;
               }

               // Patch in the length
               dbeg[0] |= rxSize; 

               uint64_t timestampRange[2];
               getTimestampRange (timestampRange, _sampleData[index], rxSize);
               fbs[index].m_body.setTimeRange (timestampRange[0],
                                               timestampRange[1]);
               fbs[index].m_body.setSize (rxSize);
               fbs[index].m_body.setRxSequence (_rxSequence++);
            
                                  

               // If using software BURST mode and no trigger active
               static int Last = -5;

               if (_config._runMode != Last)
               {
                  fprintf (stderr, "RunMode = %d\n", _config._runMode);
                  Last = _config._runMode;
               }
               if (_config._runMode == BURST && !trgActive)
               {
                  // Check if this is a soft trigger
                  bool accepted = softTrigger.check (timestampRange, _config);
                  
                  if (accepted) 
                  {
                     event = &events[index];
                     event->m_trigger.init (timestampRange[0], softTriggerCnt++, 0); 
                     event->setWindow (timestampRange[0] - WindowSize/2, 
                                       timestampRange[0] + WindowSize/2);
                     event->seedAndDrain (latency, _fd);
                     trgActive = true;
                  }
               }
               else if (_config._runMode == TRIG && trgActive && event == NULL)
               {
                  event            = &events[index];
                  event->m_trigger = trigger;
                  event->setWindow (trigger.m_timestamp - WindowSize/2, 
                                    trigger.m_timestamp + WindowSize/2);
                  event->seedAndDrain (latency, _fd);
               }


               ////fprintf (stderr, "Trigger active %d\n", trgActive);


               if (trgActive)
               {
                  Event::Fate fate = event->add (&fbs[index], dest);
                  if (fate == Event::Added)
                  {
                     // Do nothing
                  }
                  else if (fate > 0)
                  {
                     // -------------------------------------------------------------
                     // This was an accepted event either
                     //      Completed -- All contributions present and accounted for
                     //      Overrun   -- The time of at least one contributor was
                     //                   already completed.  Basicall the other
                     //                   contribution failed to show up.
                     // -------------------------------------------------------------
                     trgActive   = false;
                      _workQueue->push (event);
                     
                     latency[dest].resetToEmpty ();


                     if (fate == Event::Overrun)
                     {
                        latency[dest].replace (&fbs[index], _fd);
                     }
                  }
                  else 
                  {
                     latency[dest].replace (&fbs[index], _fd);
                  }
               }
               else
               {
                  //fprintf (stderr, "Latency[%d] @ %p fbs[%d] @ %p\n", 
                  //           dest, &latency[dest], index, &fbs[index]);
                  latency[dest].replace (&fbs[index], _fd);
               }

                  
               // --------------------------------------------------
               // If couldn't post the data, return it and count the 
               // number of dropped packets.
               // --------------------------------------------------
               //if (!posted)
               //{
                  // We are saturated, drop this frame
                  //   _counters._dropCount++;            
                  //dmaRetIndex(_fd,index);
                  //}
            }
         }
      }
      else
      {
         // Placeholder for empty reads
         rate.monitor (-1);
      }


      // Process TX release queue
   RELEASE:
      {
         FrameBuffer *tempBuffer;
         while ( (tempBuffer = (FrameBuffer *)_relQueue->pop()) != NULL ) {

            // Return the descriptor
            _rxQueue->push(tempBuffer);

            // Return the data buffer
            dmaRetIndex(_fd,tempBuffer->index());

            _rxPend--;
         } 
      }     
   }

   return;
}

// Static work thread run
void * DaqBuffer::workRunRaw ( void *p ) {
   DaqBuffer *buff = (DaqBuffer *)p;
   buff->workRun();
   pthread_exit(NULL);
   return(NULL);
}

// Class method for work thread
void DaqBuffer::workRun () {
   FrameBuffer * tempBuffer;
   FrameBuffer * txBuffer;
   _txPend      = 0;



   // Run while enabled
   while (_workThreadEn) {

      // Process TX ack queue, wait on it if we are saturated
      while ( (tempBuffer = (FrameBuffer *)_txAckQueue->pop((_txPend == TxFrameCount)?WaitTime:0)) != NULL ) {
         _relQueue->push(tempBuffer);
         _txPend--;
      }

      // Go back around if we are saturated
      if ( _txPend == TxFrameCount ) continue;

      // Wait for an entry in pend buffer
      if ( (tempBuffer = (FrameBuffer *)_workQueue->pop(WaitTime)) == NULL) continue;

      // Custom code here
      txBuffer = tempBuffer;     
      _txReqQueue->push(txBuffer);
      _txPend++;
   }
}

// Static raw tx thread run
void * DaqBuffer::txRunRaw ( void *p ) {
   DaqBuffer *buff = (DaqBuffer *)p;
   buff->txRun();
   pthread_exit(NULL);
   return(NULL);
}



// Transmit thread
void DaqBuffer::txRun () {
   struct msghdr       msg;
   struct iovec        msg_iov[32];
   struct DaqHeader    header;

   // Setup message header
   msg.msg_name       = &_txServerAddr;
   msg.msg_namelen    = sizeof(struct sockaddr_in);
   msg.msg_iov        = msg_iov;
   msg.msg_iovlen     = 1;
   msg.msg_control    = NULL;
   msg.msg_controllen = 0;
   msg.msg_flags      = 0;


   // Init iov
   msg_iov[0].iov_base = &header;
   msg_iov[0].iov_len  = sizeof(struct DaqHeader);


   while ( _txThreadEn ) 
   {
      // Wait for data in the pend buffer
      Event *event = reinterpret_cast<decltype (event)>(_txReqQueue->pop(WaitTime));

      if (event == NULL) 
      {
         continue;
      }

      msg.msg_iovlen = 1;

      // Init header
      header.frame_size  = sizeof(struct DaqHeader);
      header.rx_sequence = 0; ////tempBuffer->rx_sequence (); there are many rxsequences
      header.tx_sequence = _txSequence++;
      header.type_id     = SoftwareVersion & 0xffff;


      fprintf (stderr, "Got Event %p\n", event);
      for (int idx = 0; idx < MAX_DEST; idx++)
      {
         List<FrameBuffer>::Node *node = event->m_list[idx].m_flnk;
         List<FrameBuffer>::Node *end  = reinterpret_cast<decltype(end)>
                                                          (&event->m_list[idx]);

         int count = 0;
         while (node != end)
         {
            uint32_t txSize    = node->m_body.size  ();
            void    *ptr       = node->m_body.baseAddr ();

            fprintf (stderr, 
                     "Node[%1d.%1d] %p flnk:%p blnk:%p end: %16.16" 
                     PRIx64 " -> %16.16" PRIx64 " index: %3d size: %8.8x\n",
                     idx,
                     count++,
                     node, 
                     event->m_list[idx].m_flnk,
                     event->m_list[idx].m_blnk,
                     node->m_body._ts_range[0],
                     node->m_body._ts_range[1],
                     node->m_body.index (),
                     txSize);

            node = node->m_flnk;

            // Accumulate the total size
            header.frame_size  += txSize;


            // Setup buffer pointers if valid payload
            msg_iov[msg.msg_iovlen].iov_base = ptr;
            msg_iov[msg.msg_iovlen].iov_len  = txSize;
            msg.msg_iovlen++;
  
            if (count > 15) break;

            // Return the underlying DMA buffer
            //int index = node->m_body.index ();
            //dmaRetIndex (_fd, index);
         }
      }


      for (unsigned int idx = 0; idx < msg.msg_iovlen; idx++)
      {
         void *ptr = msg_iov[idx].iov_base;
         int   len = msg_iov[idx].iov_len;

         fprintf (stderr,
                  "msg[%2d] %p:%8.8x\n", idx, ptr, len);
      }
      
      fprintf (stderr, "Header             %8.8x %8d:%d %8.8x\n",
               header.frame_size,
               header.rx_sequence,
               header.tx_sequence,
               header.type_id);

                           

      int disconnect_wait = 10;
      if (_config._blowOffTxEth == 0)
      {
         ssize_t ret;
         if ( (_txFd < 0)  || 
              (ret = sendmsg(_txFd,&msg,0) != (int32_t)header.frame_size )) 
         {
            if (_txFd >= 0)
            {
               /// Kill the ethernet 
               _config._blowOffTxEth = 1;

               fprintf (stderr,
                        "Have error, waiting %d seconds to disconnect\n",
                        disconnect_wait);

               fprintf (stderr, 
                        "sendmsg: %zu errno: %d\n"
                        "Header; %8.8" PRIx32 " %8.8" PRIx32
                               " %8.8" PRIx32 " %8.8" PRIx32 "\n"
                        "msg_iovlen = %2zu\n"
                        "0. iov_base: %p  iov_len: %zu\n"
                        "1. iov_base: %p  iov_len: %zu\n",
                        ret, errno,
                        header.frame_size,  header.tx_sequence, 
                        header.rx_sequence, header.type_id,
                        msg.msg_iovlen,
                        msg_iov[0].iov_base, msg_iov[0].iov_len, 
                        msg_iov[1].iov_base, msg_iov[1].iov_len);
               _counters._txErrors++;


               sleep (disconnect_wait);
               disableTx ();
               fputs ("Disconnected", stderr);
            }
         }
         else
         {
            _counters._txCount++;
            _txSize             = header.frame_size;
            _counters._txTotal += header.frame_size;
         }
      }

      event->free (_fd);

      // Return entry
      _txAckQueue->push (event);
   }
}

// Get status
void DaqBuffer::getStatus(struct BufferStatus *status) {
   struct timeval currTime;
   struct timeval diffTime;
   float  rxRate;
   float  triggerRate;
   float  rxBw;
   float  txBw;
   float  txRate;
   float  interval;

   gettimeofday (&currTime, NULL);
   timersub     (&currTime, &_lastTime, &diffTime); 

   interval = (float)diffTime.tv_sec + ((float)diffTime.tv_usec / 1000000.0);

   // 
   // 2016.09.15 -- jjr
   // -----------------
   // The original rate calculation assumed fixed size receive packets
   // The rxRate is in Hz.
   // The rxBw   is in MBits/sec
   //
   rxRate = (_counters._rxCount - _last_counters._rxCount) / interval;
   rxBw   = ((((float)(_counters._rxTotal - _last_counters._rxTotal)) / interval) * 8.0) / 1e6;


   triggerRate = ((float)(_counters._triggers - _last_counters._triggers)) / interval;

   // txRate is in Hz
   // txBw   in in Mbits/sec
   //
   txRate = (_counters._txCount - _last_counters._txCount) / interval;
   txBw   = ((((float)(_counters._txTotal - _last_counters._txTotal)) / interval) * 8.0) / 1e6;


   // These variables are static or updated every received packets
   status->buffCount    = _bCount;
   status->rxSize       = _rxSize;
   status->rxPend       = _rxPend;
   status->txSize       = _txSize;
   status->txPend       = _txPend;


   // These are the accumulated counters
   status->rxCount      = _counters._rxCount;
   status->rxErrors     = _counters._rxErrors;
   status->dropCount    = _counters._dropCount; 
   status->triggers     = _counters._triggers;
   status->txCount      = _counters._txCount;
   status->txErrors     = _counters._txErrors;


   // These are derived rates
   status->triggerRate  = triggerRate;
   status->rxRate       = rxRate;
   status->rxBw         = rxBw;
   status->txRate       = txRate;
   status->txBw         = txBw;


   // Save the time and counters for the next go-around
   _lastTime      = currTime;
   memcpy ((void *)&_last_counters, (void *)&_counters, sizeof (_last_counters));
}


bool DaqBuffer::enableTx ( const char *addr, uint16_t port ) {
   this->disableTx();

   memset(&_txServerAddr,0,sizeof(struct sockaddr_in));
   _txServerAddr.sin_family = AF_INET;
   _txServerAddr.sin_addr.s_addr=inet_addr(addr);
   _txServerAddr.sin_port=htons(port);

   if ( (_txFd = socket(AF_INET,SOCK_STREAM,0) ) < 0 ) {
      fprintf(stderr,"DaqBuffer::enableTx -> Failed to create socket\n");
      return false;
   }

   if ( connect(_txFd,(struct sockaddr *)&_txServerAddr, sizeof(struct sockaddr_in)) != 0 ) {
     fprintf(stderr,"DaqBuffer::enableTx -> Failed to connect to server at %s port %u\n",addr,port);
      this->disableTx();
      return false;
   }


   int       sndSize;
   socklen_t optSize;

   /* Retrieve the socket's send buffer size */
   optSize = sizeof (sndSize);

   sndSize = 1024*1024;
   setsockopt (_txFd, SOL_SOCKET, SO_SNDBUF, &sndSize, optSize);
   fprintf (stderr, "Send buffer size (request)  = %d\n", sndSize);


   getsockopt (_txFd, SOL_SOCKET, SO_SNDBUF, &sndSize, &optSize);
   fprintf (stderr, "Send buffer size (readback) = %d\n", sndSize);

   _txSequence = 0;
   return(true);
}

void DaqBuffer::resetCounters() {

   _counters.reset      ();
   _last_counters.reset ();

   _rxSize        = 0;
   _txSize        = 0;

   gettimeofday (&_lastTime, NULL);
}


// Close
void DaqBuffer::disableTx () {
   if ( _txFd >= 0 ) {
      ::close(_txFd);
      _txFd = -1;
   }
}

// Set config
void DaqBuffer::setConfig ( 
            uint32_t blowOffDmaData,
            uint32_t blowOffTxEth,
            uint32_t naccept,
            uint32_t nframes) {
   _config._blowOffDmaData  = blowOffDmaData;
   _config._blowOffTxEth    = blowOffTxEth;
   _config._naccept         = naccept;
   _config._nframes         = nframes;
}

// Set run mode
void DaqBuffer::setRunMode ( RunMode mode ) {
   _config._runMode        = mode;
}
