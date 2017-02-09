//-----------------------------------------------------------------------------
// File          : FrameBuffer.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 09/18/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    Class to contain and track a microslice buffer.
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
// Modification history :
//
//       DATE WHO WHAT 
// ---------- --- ------------------------------------------------------------
// 2016.11.05 jjr Added receive frame sequence number
//
//
// 09/18/2014: created
//-----------------------------------------------------------------------------

#ifndef __FRAME_BUFFER_H__
#define __FRAME_BUFFER_H__

#include <stdint.h>

class FrameBuffer {
   private:
      uint8_t  * _data;
      int32_t    _index;
      uint32_t   _size;
      uint32_t   _rx_sequence;
   public:
      FrameBuffer  ();
     ~FrameBuffer  ();

      void        setData (uint32_t        index, 
                           uint8_t         *data, 
                           uint32_t         size, 
                           uint32_t  rx_sequence); 

      int32_t  index       ();
      uint8_t *baseAddr    ();
      uint32_t size        ();
      uint32_t rx_sequence ();
};

#endif

