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
    | Adjustmets for the V2 driver
   */
   dmaInitMaskBytes(mask);
   //dmaAddMaskBytes(mask,128); // HLS[0]
   //dmaAddMaskBytes(mask,129); // HLS[1]
   dmaAddMaskBytes(mask,0); // HLS[0]
   dmaAddMaskBytes(mask,1); // HLS[1]

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



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Primitive hex dump of the received frame
 *
 *  \param[in]   data The frame data
 *  \param[in] nbytes The number bytes in the data
 *
\* ---------------------------------------------------------------------- */
static inline void dump_frame (void const *data, int nbytes)
{
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
 *
\* ---------------------------------------------------------------------- */
static void check_frame (uint64_t const *data, int nbytes, unsigned int dest, int sample)
{
#  define N64_PER_FRAME 30
#  define NBYTES (unsigned int)(((1 + N64_PER_FRAME * 1024) + 1) * sizeof (uint64_t))


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
      else if (1 && ((Counter % (1024/sample)) == 0) && ((frame % 256) == 0))
      {
         // --------------------------------------
         // Print reassuring message at about 2 Hz
         // --------------------------------------
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
   // Keep track of the number of time called and
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
static inline bool post (CommQueue   *rxQueue,
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

      printf ("Posted\n");
      return posted;
   }




   return false;
}
/* ---------------------------------------------------------------------- */
      




#define MAX_DEST 1

// Class method for rx thread running
void DaqBuffer::rxRun () {
   struct timeval timeout;   
   fd_set         fds;
   uint32_t       index;
   int32_t        rxSize;
   uint32_t       dest;
   uint32_t       lastUser;
   uint32_t       flags;

   int32_t       naccept = 0;
   int32_t         nwait = 0;
   
   // Init 
   _rxPend      = 0;   


   static uint32_t DumpCounter[2] = {0, 0};

   struct timeval cur;
   gettimeofday (&cur, NULL);
   fprintf (stderr, "Rxrun time = %lu.%06lu\n",
            cur.tv_sec, cur.tv_usec);
      

   char *line __attribute__ ((unused));

   line = (char *)malloc (80);
   size_t  n __attribute__ ((unused));
   fprintf (stderr, ">");
   getline (&line, &n, stdin);


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
   int nbufs     =    _bCount;
   int nrxbufs   =   _rxCount;
   AxiBufChecker abc[_bCount];

   for (int idx = 0; idx < 2; idx++)
   {
      int nbad = AxiBufChecker::extreme_vetting (abc 
                                                 ,_fd,
                                                 idx, 
                                                 _sampleData, 
                                                 nrxbufs,
                                                 nbufs, 
                                                 _bSize);
      nrxbufs -= nbad;
   }

   fprintf (stderr, "Done\n");
   // -------------------------------------------------------


   struct timeval curTime;
   struct timeval prvTime;
   struct timeval difTime;
   uint32_t     count[3] = {0, 0, 0};
   uint32_t     empty    =  0;

   gettimeofday (&prvTime, NULL);



   // Run while enabled
   while (_rxThreadEn) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = WaitTime;
      FD_ZERO(&fds);
      FD_SET(_fd,&fds);

      // Wait for data or timeout
      select(_fd+1,&fds,NULL, NULL, &timeout); 

      // Attempt to read
      if ((rxSize = dmaReadIndex(_fd,&index,&flags,NULL, &dest)) > 0 )
      {
         gettimeofday (&curTime, NULL);
         timersub (&curTime, &prvTime, &difTime); 

         uint32_t interval = 1000 * 1000 * difTime.tv_sec 
                           +               difTime.tv_usec;
         if      (dest == 0) count[0] += 1;
         else if (dest == 1) count[1] += 1;
         else                count[2] += 1;
         if (interval >= 10* 1000 * 1000)
         {
            uint32_t total = count[0] + count[1] + count[2];
            fprintf (stderr, "Empty = %6u Rate = %6u+%6u+%6u=%6u/%9u\n", 
                     empty, count[0], count[1], count[2], total, interval);
            prvTime  = curTime;
            count[0] = 0;
            count[1] = 0;
            count[2] = 0;
         }

         lastUser = axisGetLuser(flags);
         uint32_t print_it = ((DumpCounter[dest] & 0xfffff) < 4);
         DumpCounter[dest] += 1;
         if (0 && print_it)
         {
            printf ("Index:%4u  dest: %3u rxSize = %6u\n", index, dest, rxSize);
         }

         //////////////////////////////////////////////////////////////////////
         // Check if blowing off the DMA data
         if ( (_config._blowOffDmaData != 0) || ((dest != 0) && (dest != 1)) ) {
            // Return index value 
            dmaRetIndex(_fd,index);         
            
         //////////////////////////////////////////////////////////////////////
         // Check for a external trigger
         } else if ( dest == 8 ) {
            printf ("qWeird dest = %u\n", dest);
            _counters._triggers++;
            dmaRetIndex(_fd,index);
            
         //////////////////////////////////////////////////////////////////////
         // Else this is HLS data (dest < MAX_DEST) 
         } else {
            _counters._rxCount++;
            _counters._rxTotal += rxSize;

            _rxSequence += 1;

            int check_it = ((DumpCounter[dest] & 0xff) == 0);
            if  (check_it)
            {
               check_frame ((uint64_t const *)_sampleData[index],
                            rxSize,
                            dest,
                            256);
            }

            if (0 & print_it)
            {
               dump_frame (_sampleData[index], rxSize);
            }

            _rxSize = rxSize; 

            // Check if there is error
            if( lastUser & TUserEOFE ){
               _counters._rxErrors++;
               dmaRetIndex(_fd,index);

            // Try to Move the data
            } else {

               bool posted;

                // Check if the wait period has expired
                if (nwait-- <= 0)
                {
                   // Rearm

                   naccept = _config._naccept - 1;
                   nwait   = _config._nframes - 1;
                   printf ("Arming: naccept/nwait = %4" PRId32 "/%4" PRId32 "\n",
                           naccept, nwait);
                }


                // Should this frame be accepted
               if (naccept >= 0)
               {
                  printf ("Posting[%3d]: naccept/nwait = %4" PRId32 "/%4" PRId32 "\n",
                         index, naccept, nwait);

                  posted = post (_rxQueue, 
                                 _workQueue, 
                                 _sampleData[index], 
                                 index, 
                                 rxSize,
                                 _rxSequence);
                  naccept -= 1;
               }
               else
               {
                  posted = false;
               }
                  
               // --------------------------------------------------
               // If couldn't post the data, return it and count the 
               // number of dropped packets.
               // --------------------------------------------------
               if (!posted)
               {
                  // We are saturated, drop this frame
                  _counters._dropCount++;            
                  dmaRetIndex(_fd,index);
               }
            }            
         }
      }
      else
      {
         // Placeholder for empty reads
         empty += 1;
      }


      // Process TX release queue
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
   struct iovec        msg_iov[2];
   struct DaqHeader    header;
   FrameBuffer       * tempBuffer;



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
      if ( (tempBuffer = (FrameBuffer *)_txReqQueue->pop(WaitTime)) == NULL ) 
      {
         continue;
      }

      int disconnect_wait = 10;
      if (_config._blowOffTxEth == 0)
      {
         int      frame_idx = tempBuffer->index ();
         uint32_t txSize    = tempBuffer->size  ();
         void    *ptr       = tempBuffer->baseAddr ();


         // Init header
         header.frame_size  = sizeof(struct DaqHeader) + txSize;
         header.rx_sequence = tempBuffer->rx_sequence ();
         header.tx_sequence = _txSequence++;
         header.type_id     = SoftwareVersion & 0xffff;
         msg.msg_iovlen     = 1;


         // Setup buffer pointers if valid payload
         msg_iov[msg.msg_iovlen].iov_base = ptr;
         msg_iov[msg.msg_iovlen].iov_len  = txSize;
         msg.msg_iovlen++;

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
                        "1. iov_base: %p  iov_len: %zu idx: %d\n",
                        ret, errno,
                        header.frame_size,  header.tx_sequence, 
                        header.rx_sequence, header.type_id,
                        msg.msg_iovlen,
                        msg_iov[0].iov_base, msg_iov[0].iov_len, 
                        msg_iov[1].iov_base, msg_iov[1].iov_len, frame_idx);
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

      // Return entry
      _txAckQueue->push(tempBuffer);
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
