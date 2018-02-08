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
// 2018.02.06 jjr Added a status mask to accumulate error/status bits
// 2017.07.11 jjr Moved many methods from .cpp file to be inlined
//                Added method to get the frame address as a 64 bit number
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
      uint32_t   _status;

public:
      uint64_t   _ts_range[2];
   public:
      FrameBuffer  ();
     ~FrameBuffer  ();

     enum StatusMask
     {
        Missing = 1    /*!< Frame has missing time samples */
     };
        

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
         _status      =   0;
      }


      void  addStatus (StatusMask bit)
      {
         _status |= static_cast<uint32_t>(bit);
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

      uint32_t  status      () const;
      int32_t   index       () const;
      uint8_t  *baseAddr    () const;
      uint64_t *baseAddr64  () const;
      uint32_t  size        () const;
      uint32_t  rx_sequence () const;
};





/* ====================================================================== */
/* BEGIN: IMPLEMENTATION                                                  */
/* ---------------------------------------------------------------------- *//*!

  \brief Set the basic parameters of an incoming frame
  
  \param[in]       index  The DMA buffer index of the frame
  \param[in]        data  The virtual address  of the frame
  \param[in]        size  The size, in bytes,  of the frame
  \param[in] rx_sequence  The received sequence number of the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setData (uint32_t       index, 
                                  uint8_t        *data,
                                  uint32_t        size,
                                  uint32_t rx_sequence)
{
    _index       = static_cast<int32_t>(index);
    _data        = data;
    _size        = size;
    _rx_sequence = rx_sequence;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief     Sets the DMA frame buffer index
  \param[in] index  The DMA buffer index of the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setIndex (uint32_t index) { _index = index; }
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief     Sets the DMA frame buffer virtual address
  \param[in] data  The virtual address  of the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setData (uint8_t *data) { _data  =  data; }
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief     Sets the size, in bytes, of the frame buffer
  \param[in] size  The size, in bytes, of the frame buffer
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setSize (uint32_t size) { _size  =  size; } 
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setRxSequence (uint32_t rx_sequence) 
{
   _rx_sequence = rx_sequence; 
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief   Return the frame buffer's DMA index
  \return  The frame buffer's DMA index
                                                                          */
/* ---------------------------------------------------------------------- */
inline int32_t FrameBuffer::index() const { return(_index); }
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief   Return the base address of the frame buffer as an 8-bit pointer
  \return  The base address of the frame as a 8-bit pointer
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint8_t *FrameBuffer::baseAddr () const { return (_data); }
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief  Return the frame buffer size, in bytes
  \return The frame buffer size, in bytes
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::size () const { return(_size); }
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief  Return the frame buffer's received sequence number
  \return The frame buffer's received sequence number
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::rx_sequence () const { return _rx_sequence; }
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief   Return the base address of the frame as a 64-bit pointer
  \return  The base address of the frame as a 64-bit pointer
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t *FrameBuffer::baseAddr64 () const 
{
   return reinterpret_cast<uint64_t *>(_data);
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief The status of this frame expressed as a mask of error bits.
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::status () const
{
   return _status;
}
/* ---------------------------------------------------------------------- */
/* END: IMPLEMENTATION                                                    */
/* ====================================================================== */

#endif

