//-----------------------------------------------------------------------------
// File          : SimLinkByte.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 09/07/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Communications link For Simulation
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
// 09/07/2012: created
//-----------------------------------------------------------------------------
#ifndef __SIM_LINK_BYTE_H__
#define __SIM_LINK_BYTE_H__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
#include <stdint.h>
using namespace std;

// Constant
#define SIM_LINK_BUFF_SIZE 1000000
#define SHM_BASE           "axi_stream"

// Shared memory structure
typedef struct {

   // Upstream
   uint32_t        usReqCount;
   uint32_t        usAckCount;
   uint32_t        usData[SIM_LINK_BUFF_SIZE];
   uint32_t        usSize;
   uint32_t        usVc;
   uint32_t        usEofe;
   
   // Downtream
   uint32_t        dsReqCount;
   uint32_t        dsAckCount;
   uint32_t        dsData[SIM_LINK_BUFF_SIZE];
   uint32_t        dsSize;
   uint32_t        dsVc;

   // Shard path
   char path[200];

} SimLinkMemory;


//! Class to contain PGP communications link
class SimLinkByte : public CommLink {

   protected:

      // Shared memory
      SimLinkMemory *smem_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      SimLinkByte ( );

      //! Deconstructor
      ~SimLinkByte ( );

      //! Open link and start threads
      void open ( string system, uint32_t id );

      //! Stop threads and close link
      void close ();

};
#endif
