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
// 2017.05.27 jjr Added setData to set fields separately.  
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
      uint64_t   _ts_range[2];
   public:
      FrameBuffer  ();
     ~FrameBuffer  ();

      void        setData (uint32_t        index, 
                           uint8_t         *data, 
                           uint32_t         size, 
                           uint32_t  rx_sequence);

      void        setIndex (uint32_t       index);
      void        setData  (uint8_t        *data);
      void        setSize  (uint32_t        size);
      void   setRxSequence (uint32_t rx_sequence);
      void    setTimeRange (uint64_t beg, uint64_t end)
      {
         _ts_range[0] = beg;
         _ts_range[1] = end;
      }

      uint16_t getCsf () const
      {
         // Need to swap slot and crate
         // Id = slot.3 | crate.5 | fiber.3
         uint64_t const *p64 = reinterpret_cast<decltype(p64)>(_data);
         uint16_t    id = (p64[1] >> 13) & 0xfff;
         uint16_t crate = (id     >>  3) &  0x1f;
         uint16_t  slot = (id     >>  8) &  0x07;
         uint16_t fiber = (id     >>  0) &  0x03;

         id = (crate << 6) | (slot << 3) | fiber;
         return id;
      }

      int32_t  index       () const;
      uint8_t *baseAddr    () const;
      uint32_t size        () const;
      uint32_t rx_sequence () const;
};

#endif

