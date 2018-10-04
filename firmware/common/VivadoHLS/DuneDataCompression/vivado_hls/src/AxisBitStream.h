// -*-Mode: C;-*-

#ifndef AXISBITSTREAM_H
#define AXISBITSTREAM_H

/* ---------------------------------------------------------------------- *//*!

   \file  AxisBitStream.h
   \brief A bit stuffer with the output going to an AxisStream
   \author JJRussell - russell@slac.stanford.edu

   Bits are stuffed into a 64 bit word such that bit 0 is in the MSB
                                                                          */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\
 *
 * HISTORY
 * -------
 *
 * DATE       WHO WHAT
 * ----------- --- ---------------------------------------------------------
 * 2016.12.08 jjr Created, extracted from AP-Encode.h
 *
 *
\* ---------------------------------------------------------------------- */

#include "Axis.h"
#include <stdint.h>
#include <ap_int.h>


class AxisBitStream
{
public:
   AxisBitStream ();
   AxisBitStream (int bdx);

   // -----------------------------------------------------
   // !!! IMPROVEMENT OPPORTUNITY !!!
   // -------------------------------
   // 32-bits is way overkill. The maximum number of output
   // words is around 30K. It really can't be any more
   // than somewhat less the 64K, otherwise there would
   // not be enough time to write them all out.
   // However, given that there is only 1 copy of this
   // per HLS stream, making it 'large' is not a big deal.
   // -----------------------------------------------------
   typedef uint32_t Idx_t;

public:
   void                 set   (Idx_t idx);
   void                 reset ();
   AxisBitStream::Idx_t flush (AxisOut &mAxis);
   int                  bidx  () const;
   int                  widx  () const;

   void               insert (AxisOut &mAxis, uint64_t bits, int nbits);
   void             insert64 (AxisOut &mAxis, uint64_t bits, ap_uint<6> lshift, ap_uint<6> rshift);
   void                write (AxisOut &mAxis, uint64_t  w64);

public:
   uint64_t                  m_cur; /*!< Output staging buffer                   */
   AxisBitStream::Idx_t      m_idx; /*!< The current bit index                   */
};


__inline AxisBitStream::AxisBitStream () :
      m_cur  (   0),
      m_idx  (   0)
{
   return;
}

__inline AxisBitStream::AxisBitStream (int bidx) :
      m_cur  (   0),
      m_idx  (bidx)
{
   return;
}

__inline void AxisBitStream::set (AxisBitStream::Idx_t idx)
{
   #pragma HLS INLINE
   m_idx = idx;
}

__inline AxisBitStream::Idx_t AxisBitStream::flush (AxisOut &mAxis)
{
   #pragma HLS INLINE
    int cur_idx = bidx ();

   // If anything in the current buffer, write it out
   if (cur_idx != 0)
   {  
      m_cur <<= 0x40 - cur_idx;
      this->write (mAxis, m_cur);
   }
   return m_idx;
}

__inline int AxisBitStream::bidx () const
{
   #pragma HLS INLINE
   return m_idx & 0x3f;
}

__inline int AxisBitStream::widx () const
{
   #pragma HLS INLINE
   return m_idx >> 6;
}


__inline void AxisBitStream::insert (AxisOut &mAxis, uint64_t bits, int nbits)
{
   #pragma HLS PIPELINE
   #pragma HLS INLINE


   ///std::cout << "Adding # bits = " << nbits << std::endl;

   int cur_bidx = bidx ();
   int left     = 64 - cur_bidx;
   int overrun  = nbits - left;

   #ifndef __SYNTHESIS__
      int nbidx = m_idx;
   #endif

   uint64_t obits;
   int shift  =  overrun > 0 ? left : nbits;
   m_idx     += nbits;

#if 1
   if (nbits != 0x40) bits &= (1LL << nbits) - 1;
   if (shift == 0x40)
   {
      m_cur  = bits;
      obits  = bits;
   }
   else
   {
      uint64_t tmp = m_cur << shift;
      if (overrun >= 0)
      {
         obits = tmp | (bits >> overrun);
         m_cur = bits;
      }
      else
      {
         m_cur = tmp | bits;
         return;
      }
   }

#else

   if (nbits != 0x40) bits &= (1LL << nbits) - 1;
   if (shift == 0x40)
   {
      obits = bits;
      m_cur = bits;
   }
   
  
   if (overrun > 0)
   {
      obits  = (m_cur << left) | (bits >> overrun);
      m_cur  = bits;
   }
   else
   {
      m_cur <<= nbits;
      m_cur  |= bits;
   }
#endif



   // Check if there is enough room in the current word
   if (overrun >= 0)
   {
      // There is no need to clear any set bits beyond
      // the overrun bits, they will eventually by
      // shifted out the top bit.
      this->write (mAxis, obits);

      #ifndef __SYNTHESIS__
         nbidx += left;

         /* STRIP remove output 2018-06-28
         std::cout << "AxisBs: "
                   << std::setfill ('0') << std::hex << std::setw (16) << m_cur
                   << ':'       <<  std::hex << std::setw(16) << std::setfill ('0') << bits
                   << " len = " <<  std::hex << std::setw( 4) << std::setfill (' ') << nbidx << std::endl;
         */
      #endif
   }


   #ifndef __SYNTHESIS__
   ///   std::cout << "BS64:" << m_out.name () << "len = " << m_idx << std::endl;
   #endif
   return;
}

__inline void AxisBitStream::insert64 (AxisOut &mAxis, uint64_t bits, ap_uint<6> lshift, ap_uint<6> rshift)
{
   #pragma HLS PIPELINE
   #pragma HLS INLINE


   ///std::cout << "Adding # bits = " << nbits << std::endl;



   #ifndef __SYNTHESIS__
      int nbidx = m_idx;
   #endif

   uint64_t toWrite;

   if (lshift == 0)
   {
      toWrite = bits;
   }
   else
   {
      toWrite =  (m_cur << lshift) | (bits >> rshift);
   }

   this->write (mAxis, toWrite);


   // There is no need to clear any set bits beyond
   // the overrun bits, they will eventually by
   // shifted out the top bit.

   #ifndef __SYNTHESIS__
      nbidx += lshift;

      /* STRIP remove output 2018-06-28
      std::cout << "AxisBs: "
                << std::setfill ('0') << std::hex << std::setw (16) << m_cur
                << ':'       <<  std::hex << std::setw(16) << std::setfill ('0') << bits
                << " len = " <<  std::hex << std::setw( 4) << std::setfill (' ') << nbidx << std::endl;
      */
   #endif

   m_cur  = bits;


   #ifndef __SYNTHESIS__
   ///   std::cout << "BS64:" << m_out.name () << "len = " << m_idx << std::endl;
   #endif
   return;
}


__inline void AxisBitStream::write (AxisOut &mAxis, uint64_t w64)
{
   #pragma HLS INLINE
   int odx = widx ();
   commit (mAxis, odx, true, w64, 0, 0);
   return;
}

#endif
