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
// 2018.03.26  jjr  Release 1.2.0-0
//                  Modify TimingMsg to match the V4 firmware
// 2017.02.06  jjr  Version 1.1.0-0
//                  Add methods to individually specific the opening and
//                  enabling of a DMA device
// 2017.10.19  jjr  Version 1.0.5-0
//                  Correct an error in the Table of Contents descriptor count.
// 2017.10.12  jjr  Bump software version to 1.0.4.0
//                  This corrects 2 errors in the calculation of lengths
//                  in the output TpcDataRecords. 
//                   - The length of the table of contents (TOC) record
//                     failed to include the terminating 64-bit word
//                   - The length of the Packet record failed to include
//                     the header size.
// 2017.09.15  jjr  Bump software version to 1.0.3-0
// 2017.08.29  jjr  Bump software version to 1.0.1-0
// 2017.06.19  jjr  Replaced naccept/nframes with pretrigger and dureation times
// 2017.05.26  jjr  Added checkBuffer method to check for unreadable DMA buffers
// 2017.04.05  jjr  Added rx and tx DMA buffer counts.  The new DMA driver
//                  lumps these buffers together. 
// 2016.11.05  jjr  Added receive sequence number
// 2016.11.05  jjr  Incorporated new DaqHeader
// 2016.10.27  jjr  Added throttling of the output in the configuation block
//                  These are the naccept and nframe field members
//    Unknown   lr  Created
//  
//-----------------------------------------------------------------------------
#ifndef __ART_DAQ_BUFFER_H__
#define __ART_DAQ_BUFFER_H__

#include <AxisDriver.h>

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <queue>
#include <poll.h>
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
   uint32_t trgMsgCnt;
   uint32_t disTrgCnt;
   uint32_t dropSeqCnt;

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




/* ---------------------------------------------------------------------- *//*!

   \class DaqDmaDevice
   \brief Capture the context of a AXI dma device
                                                                          */
/* ---------------------------------------------------------------------- */
class DaqDmaDevice
{
public:
   DaqDmaDevice ();
   int      open         (char const *name, uint8_t const *dests, int ndests);
   int      enable       (uint8_t const *dests, int ndests);
   int      enable       ();
   int      map          ();

   uint32_t allocateWait ();
   ssize_t  free         (int  index);

   void     wait         ();
   void     vet          ();
   int      unmap        ();
   int      close        ();

   static uint32_t allocateWait (int32_t fd);
   static void     wait         (int32_t fd);

public:
   static const uint32_t TUserEOFE = 0x1;

   int32_t             _fd;   /*!< File descriptor of DMA driver          */
   uint32_t         _bSize;   /*!< Size, in bytes, of the DMA buffers     */
   uint32_t        _bCount;   /*!< Total count of DMA buffers (tx + rx)   */
   uint32_t       _rxCount;   /*!< Count of DMA receive  buffers          */
   uint32_t       _txCount;   /*!< Count of DMA transmit buffers          */
   uint8_t          **_map;   /*!< Index -> virtual address DMA map       */
   char     const   *_name;   /*!< The device name (for error reporting)  */
   int             _ndests;   /*!< The number of destinations             */
   uint8_t  const  *_dests;   /*!< Pointer to the array of destinations   */
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief  Constructor for a AXI dma device
                                                                          */
/* ---------------------------------------------------------------------- */
inline DaqDmaDevice::DaqDmaDevice () :
   _fd (-1)
{
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief  Opens the AXI dma device

  \param[in]   name  The name of the AXI dma device
  \param[in]  dests  Array of the destination channels.
  \param[in] ndests  The number of destination channels

  Since only a pointer to the \a name and array \a dests is stored, the 
  these must persist which usually means it is in static storage.
                                                                          */
/* ---------------------------------------------------------------------- */
inline int DaqDmaDevice::open (char    const  *name, 
                               uint8_t const *dests,
                               int           ndests)
{
   _fd = ::open (name, O_RDWR | O_NONBLOCK);
   if (_fd >= 0) 
   {
      _name   =   name;
      _ndests = ndests;
      _dests  =  dests;
   }
   return _fd;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Enables the specified destinations

  \param[in]  dests  The array of destinations to enable
  \param[in] ndests  The number of destinations
                                                                          */
/* ---------------------------------------------------------------------- */
inline int DaqDmaDevice::enable (uint8_t const *dests, int ndests)
{
   uint8_t mask[DMA_MASK_SIZE];

   dmaInitMaskBytes(mask);

   for (int idx = 0; idx < ndests; idx++)
   {
      uint8_t dest = dests[idx];

      ///fprintf (stderr, "Setting masks for %u\n", dest);
      dmaAddMaskBytes(mask, dest);
   }

   if  (dmaSetMaskBytes(_fd, mask) < 0) 
   {
      this->close();
      return -1;
   }

   return 0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Enables the specified destinations

                                                                          */
/* ---------------------------------------------------------------------- */
inline int DaqDmaDevice::enable ()
{
   int    status = enable (_dests, _ndests);
   return status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Create the index -> virtual address map
                                                                          */
/* ---------------------------------------------------------------------- */
inline int DaqDmaDevice::map ()
{
   if ( (_map = (uint8_t **)dmaMapDma(_fd,&_bCount,&_bSize)) == NULL ) 
   {
      fprintf(stderr,"DaqBuffer::open -> Failed to map to dma buffers\n");
      this->close();
      return -1;
   }
   else
   {
      // Retrieve the number of receive and transmit buffers
      _rxCount = dmaGetRxBuffCount (_fd);
      _txCount = dmaGetTxBuffCount (_fd);

      return 0;
   }
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Waits for a write buffer in the DMA pool to become available
                                                                          */
/* ---------------------------------------------------------------------- */
inline void DaqDmaDevice::wait (int32_t fd)
{
   struct pollfd pollWrite;

   pollWrite.fd      = fd;
   pollWrite.events  = POLLOUT;
   pollWrite.revents = 0;
   
   int n = poll (&pollWrite, 1, -1);
   if (n != 1)
   {
      fprintf (stderr, "RSSI Poll error = %d\n", n);
      exit (-1);
   }

   if (!(pollWrite.revents & POLLOUT))
   {
      fprintf (stderr, "RSSI Poll did not return write ready %8.8x\n", 
               pollWrite.revents);
      exit (-1);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Waits for a write buffer in the DMA pool to become available
                                                                          */
/* ---------------------------------------------------------------------- */
inline void DaqDmaDevice::wait ()
{
   wait (_fd);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Allocate a writeable buffer with a wait (blocker)
  \return Index of the allocated buffer

  \param[in] fd  The file descriptor of the DMA driver
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t DaqDmaDevice::allocateWait (int32_t fd)
{
   uint32_t index = dmaGetIndex (fd);

   printf ("Index = %8.8x errno = %d\n", index, errno);


   // --------------------------------------
   // !!! KLUDGE !!!
   // --------------
   // dmaGetIndex returns -1 and errno = 1.
   // Not sure if that is the intent or not.
   // --------------------------------------
   if ((int)index < 0)
   {
      wait (fd);
      index = dmaGetIndex (fd);
   }

   return index;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Allocate a writeable buffer with a wait (blocker)
  \return Index of the allocated buffer
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t DaqDmaDevice::allocateWait ()
{
   return allocateWait (_fd);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Free the buffer associated with the specified \a index

   \param[in] index The index of the buffer to return;
                                                                          */
/* ---------------------------------------------------------------------- */
inline ssize_t DaqDmaDevice::free (int index)
{
   ssize_t status = dmaRetIndex (_fd, index);
   return  status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Unmaps the dma buffers from this processes virtual address space.
                                                                          */
/* ---------------------------------------------------------------------- */
inline int DaqDmaDevice::unmap   ()
{
   if ( _map != NULL ) dmaUnMapDma (_fd, (void **)_map);
   _map = NULL;
   return 0;
}

/* ---------------------------------------------------------------------- *//*!

  \brief Close the device
                                                                          */
/* ---------------------------------------------------------------------- */
inline int DaqDmaDevice::close ()
{
   if (_fd >= 0) 
   {
      int status = ::close (_fd);
      _fd = -1;
      return status;
   }
   else
   {
      return 0;
   }
}
/* ---------------------------------------------------------------------- */


#include "MappedMemory.h"
#include "inttypes.h"

/* ---------------------------------------------------------------------- *//*!

  \class RceInfo
  \brief Captures the static configuration of the host RCE. 

  \par
   This information for the most part only changes with a physical action
   or a reload or reboot.  The one exception to this is the ethernet mode.
                                                                          */
/* ---------------------------------------------------------------------- */
class RceInfo
{
public:
   static const uint32_t Base0 = 0x80000000;
   static const uint32_t Size0 = 0x2000;

   static const uint32_t Base1 = 0x84000000;
   static const uint32_t Size1 = 0x1000;

   /* ------------------------------------------------------------------- *//*!
   
     \brief Minimalistic constructor
                                                                          */
   /* ------------------------------------------------------------------- */
   RceInfo (): m_mem0 (0), m_mem1 (0) { return; }
   /* ------------------------------------------------------------------- */


   void open  ()
   {
      m_mem0 = new MappedMemory (1, Base0, Size0);
      m_mem0->open ();

      m_mem1 = new MappedMemory (1, Base1, Size1);
      m_mem1->open ();

      return;
   }

   uint32_t read32  (MappedMemory *mem, uint32_t addr)
   {
      bool err;
      uint32_t u32 = mem->read (addr, &err);
      return   u32;
   }

   uint64_t read64 (MappedMemory *mem, uint32_t addr)
   {
      uint64_t u64 = read32 (mem, addr + 4);
      u64 <<= 32;
      u64  |= read32 (mem, addr);
      return u64;
   }

   void readN (MappedMemory *mem, char *str, uint32_t addr, int n8s)
   {
      uint32_t   *s = reinterpret_cast<uint32_t *>(str);

      int n32s = n8s/4;

      for (int idx = 0; idx < n32s; idx++)
      {
         uint32_t c4 = read32 (mem, addr);
         *s++  = c4;

         // Quit if found null-termination
         if (((c4 >> 0x00) & 0xff) == 0) return;
         if (((c4 >> 0x08) & 0xff) == 0) return;
         if (((c4 >> 0x10) & 0xff) == 0) return;
         if (((c4 >> 0x18) & 0xff) == 0) return;

         addr += 4;
      }

      // Ensure null-termination
      str[n32s - 1] &= 0x00ffffff;
      
      return;
   }

       
   void close ()
   {
      m_mem0->close ();
      m_mem0 = NULL;

      m_mem1->close ();
      m_mem1 = NULL;
   }

   void read ()
   {
      /* ------------------------------------------------------------------ */
      /* Firmware values                                                    */
      /*                                                                    */
      m_firmware.m_fpgaVersion     = read32 (m_mem0, Base0 | 0x0000);
      m_firmware.m_rceVersion      = read32 (m_mem0, Base0 | 0x0008);
      m_firmware.m_ethMode         = read32 (m_mem0, Base0 | 0x0034);
      readN (m_mem0, m_firmware.m_buildString,       Base0 | 0x1000, 
             sizeof (m_firmware.m_buildString));
      /*                                                                    */
      /* ------------------------------------------------------------------ */



      /* ------------------------------------------------------------------ */
      /* Cluster Element values                                             */
      /*                                                                    */
      m_clusterElement.m_bsiVersion     = read32 (m_mem1, Base1 | 0x0000 * 4);
      m_clusterElement.m_networkPhyType = read32 (m_mem1, Base1 | 0x0001 * 4);
      m_clusterElement.m_macAddress     = read64 (m_mem1, Base1 | 0x0002 * 4);
      m_clusterElement.m_interconnect   = read32 (m_mem1, Base1 | 0x0004 * 4);
      readN (m_mem1, m_clusterElement.m_uBootVersion,     Base1 | 0x0005 * 4,
             sizeof (m_clusterElement.m_uBootVersion));
      readN (m_mem1, m_clusterElement.m_rptSwTag,         Base1 | 0x000D * 4,
             sizeof (m_clusterElement.m_rptSwTag));
      m_clusterElement.m_deviceDna     = read64 (m_mem1,  Base1 | 0x0015 * 4);
      m_clusterElement.m_efFuseValue   = read32 (m_mem1,  Base1 | 0x0017 * 4);
      /*                                                                    */
      /* ------------------------------------------------------------------ */



      /* ------------------------------------------------------------------ */
      /* Cluster Ipmc values                                                */
      /*                                                                    */      
      m_clusterIpmc.m_serialNumber    = read64 (m_mem1, Base1 | 0x0050 * 4);
      m_clusterIpmc.m_address         = read32 (m_mem1, Base1 | 0x0052 * 4);
      readN    (m_mem1, m_clusterIpmc.m_groupName,      Base1 | 0x0053 * 4, 
                sizeof (m_clusterIpmc.m_groupName));
      m_clusterIpmc.m_extInterconnect = read32 (m_mem1, Base1 | 0x005B * 4);
      /*                                                                    */
      /* ------------------------------------------------------------------ */
   }


public:
   void print_firmware ()
   {
      Firmware *fw = &m_firmware;
      fprintf (stderr, 
               "  Fpga  version = %8.8" PRIx32 "\n"
               "  Rce   version = %8.8" PRIx32 "\n"
               "  Ethernet Mode = %8.8" PRIx32 "\n"
               "  Build  string = %s\n",
               fw->m_fpgaVersion,
               fw->m_rceVersion,
               fw->m_ethMode,
               fw->m_buildString);

      return;
   }

   void print_clusterElement ()
   {
      ClusterElement *ce = &m_clusterElement;

      fprintf (stderr, 
               "  BSI   version = %8.8"   PRIx32 "\n"
               "  Network  Type = %8.8"   PRIx32 "\n"
               "  Mac   address = %16.16" PRIx64 "\n"
               "  Interconnect  = %8.8"   PRIx32 "\n"
               "  UBoot version = %s\n"
               "  RPT SW   tag  = %s\n"
               "  Device    Dna = %16.16" PRIx64 "\n"
               "  Fuse    value = %8.8"   PRIx32 "\n",
               ce->m_bsiVersion,
               ce->m_networkPhyType,
               ce->m_macAddress,
               ce->m_interconnect,
               ce->m_uBootVersion,
               ce->m_rptSwTag,
               ce->m_deviceDna,
               ce->m_efFuseValue);

      return;


   }


   void print_clusterIpmc ()
   {
      ClusterIpmc  *ci = &m_clusterIpmc;
      uint32_t address = ci->m_address;
      uint8_t     slot = (address >> 16) & 0xff;
      uint8_t      bay = (address >>  8) & 0xff;
      uint8_t  element = (address >>  0) & 0xff;

      
      fprintf (stderr, 
               "  Serial Number = %16.16" PRIx64 "\n"
               "  Slot.Bay.Elem = %2.2x/%2.2x/%2.2x\n"
               "  Group    Name = %s\n"
               "  Rtm      Type = %8.8"   PRIx32 "\n",
               ci->m_serialNumber,
               slot, bay, element,
               ci->m_groupName,
               ci->m_extInterconnect);

      return;
   }

   void print ()
   {
      fprintf (stderr, 
               "\n"
               "RCE CONFIGURATION INFORMATION\n"
               "Firmware:\n");
      print_firmware       ();

      fprintf (stderr, "\nCluster Element:\n");
      print_clusterElement ();

      fprintf (stderr, "\nCluster IPMC:\n");
      print_clusterIpmc    ();

      fprintf (stderr, "\n");

      return;
   }
   /* ------------------------------------------------------------------- */


private:
   /* ------------------------------------------------------------------- */
   /* These memory accessors are only in use during the filling           */
   /* After filling, they are closed                                      */
   /* ------------------------------------------------------------------- */
   MappedMemory         *m_mem0;   /*!< Memory access for segment 0       */
   MappedMemory         *m_mem1;   /*!< Memory access for segment 1       */
   /* ------------------------------------------------------------------- */

public:
   struct Firmware
   {
      uint32_t       m_fpgaVersion; /*!< The FPGA firmware version number */
      uint32_t        m_rceVersion; /*!< The RCE build version            */
      uint32_t           m_ethMode; /*!< The ethernet mode, hangeable     */
      char      m_buildString[256]; /*!< The build data, by whom, etc     */
   };


   struct ClusterElement
   {
      uint32_t        m_bsiVersion; /*!< The cluster's BSI version        */
      uint32_t    m_networkPhyType; /*!< The network type                 */
      uint64_t        m_macAddress; /*!< The MAC address                  */
      uint32_t      m_interconnect; /*!< ???                              */
      char      m_uBootVersion[32]; /*!< The UBoot version                */
      char          m_rptSwTag[32]; /*!< The RPT's software tag           */
      uint64_t         m_deviceDna; /*!< The ZYNQ's unique identifier     */
      uint32_t       m_efFuseValue; /*!< The value burned into ZYNQ fuse  */
   };


   struct ClusterIpmc
   {
      uint64_t     m_serialNumber; /*!< The serial number of the DPM      */
      uint32_t          m_address; /*!< The version/slot/bay/element      */
      char        m_groupName[32]; /*!< The cluster group name            */
      uint32_t  m_extInterconnect; /*!< The external interconnect         */
   };

   Firmware             m_firmware;
   ClusterElement m_clusterElement;
   ClusterIpmc       m_clusterIpmc;
      
};
/* ---------------------------------------------------------------------- */



/*---------------------------------------------------------------------- *//*!
 *
 * \class  DaqBuffer
 * \brief  The control structure for taking data
 *
\* ---------------------------------------------------------------------- */ 
class DaqBuffer {

public:
   /* ------------------------------------------------------------------ *//*!
    *
    * \struct Config
    * \brief  DAQ Configuration control parameters
    *
    * \par
    *  These parameters control the
    *    -# RunMode
    *    -# Whether to pitch the incoming DMA into the bit bucker
    *    -# Whether to transmit or pitch the accepted data to the host
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
      uint32_t     _enableRssi;  /*!< Select FW RSSI or SW TCP           */      
      int32_t      _pretrigger;  /*!< Pretrigger time (usecs)            */
      int32_t     _posttrigger;  /*!< Postrigger time (usecs)            */
      uint32_t         _period;  /*!< Software trigger period            */ 
   };
   /* ------------------------------------------------------------------ */

private:

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
      
      uint32_t _disTrgCnt;
      uint32_t _dropSeqCnt;
      uint32_t _trgMsgCnt;
   };


   private:
      // Software Device Configurations
      static const uint32_t SoftwareVersion = 0x01020000;
      static const uint32_t TxFrameCount    = 100;
      static const uint32_t RxFrameCount    = 10000;
      static const uint32_t WaitTime        = 1000;

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
public:
      Config volatile _config;

private:
      uint32_t _rxSize;
      uint32_t _txSize;
      uint32_t _txSeqId;

      // Status Counters
      Counters volatile      _counters;
      Counters volatile _last_counters;

      uint32_t _rxSequence;

      struct timeval _lastTime;

      DaqDmaDevice    _dataDma;
      DaqDmaDevice  _timingDma;

      
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
            uint32_t enableRssi,            
            uint32_t pretrigger,
            uint32_t duration,
            uint32_t period);

      void vetDmaBuffers();

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


