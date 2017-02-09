//-----------------------------------------------------------------------------
// File          : MultDestMapped.h.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Mapped Memory Destination container for MultLink class.
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
// 06/18/2014: created
//-----------------------------------------------------------------------------
#ifndef __MULT_DEST_MAPPED_H__
#define __MULT_DEST_MAPPED_H__
#include <MultDest.h>
#include <string>
#include <stdint.h>
using namespace std;

class MappedMemory;

//! Class to contain PGP communications link
class MultDestMapped : public MultDest {

      MappedMemory * map_;

   public:

      //! Constructor
      // Pass count and uint32_t base, size_t size combinations
      MultDestMapped (uint32_t count, ... );

      //! Deconstructor
      ~MultDestMapped ( );

      // The remaining functions are called only from the MultLink class.

      //! Open link
      void open ( uint32_t idx, uint32_t maxRxTx );

      //! Closelink
      void close ();

      //! Transmit data.
      int32_t transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config );

};
#endif
