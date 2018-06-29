// -*-Mode: C;-*-

#ifndef BITSTREAM_H
#define BITSTREAM_H

/* ---------------------------------------------------------------------- *//*!

   \file  BitStream64.h
   \brief A bit stuffer
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


#include <stdint.h>
#include <ap_int.h>

class OStream : public hls::stream<uint64_t>
{
public:
   OStream (const char *name = "test") :
      hls::stream<uint64_t> (name)
   {
      return;
   }

#ifndef __SYNTHESIS__
public:
   const std::string &name () { return _name; }
#endif

   typedef uint32_t Idx_t;

   uint64_t               m_ccur; /*!< Output staging buffer                   */
   OStream::Idx_t         m_cidx; /*!< The current bit index                   */
};

class BitStream64
{
public:
   BitStream64 ();

   typedef uint32_t Idx_t;

public:
   void               set   (Idx_t idx);
   void               reset ();
   BitStream64::Idx_t flush (OStream &ostream);
   int                bidx  () const;
   int                widx  () const;

   //void               insert (APC_cv_t bit);
   void               insert (OStream &ostream, int bits, int nbits);
   void             transfer (OStream & ostream);

   void              dfready ();

public:
   uint64_t                  m_cur; /*!< Output staging buffer                   */
   BitStream64::Idx_t        m_idx; /*!< The current bit index                   */
   uint64_t                 m_ccur; /*!< Output staging buffer                   */
   BitStream64::Idx_t       m_cidx; /*!< The current bit index                   */
};


__inline BitStream64::BitStream64 () :
      m_idx (0),
      m_cur (0)
{
   #pragma HLS INLINE
   return;
}


__inline void BitStream64::set (BitStream64::Idx_t idx)
{
   #pragma HLS INLINE
   m_idx = idx;
}

__inline void BitStream64::transfer (OStream & ostream)
{
   ostream.m_ccur = m_cur;
   ostream.m_cidx = m_idx;
}

__inline BitStream64::Idx_t BitStream64::flush (OStream &ostream)
{
   #pragma HLS INLINE
    int cur_idx = bidx ();

   // If anything in the current buffer, write it out
   if (cur_idx != 0)
   {
      m_cur <<= 0x40 - cur_idx;
      ostream.write (m_cur);
   }

   ostream.m_cidx = m_idx;
   ostream.m_ccur = 0;

   return m_idx;
}

__inline int BitStream64::bidx () const
{
   #pragma HLS INLINE
   return m_idx & 0x3f;
}

__inline int BitStream64::widx () const
{
   #pragma HLS INLINE
   return m_idx >> 6;
}

//__inline void BitStream64::insert (unsigned int bit)
//{
//  #pragma HLS INLINE
//   insert (bit, 1);
//}

__inline void BitStream64::dfready ()
{
   #pragma HLS INLINE

   m_ccur = m_cur;
   m_cidx = m_idx;
}

__inline void BitStream64::insert (OStream &ostream, int bits, int nbits)
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

   int shift  =  overrun > 0 ? left : nbits;
   m_cur    <<= shift;
   m_cur     |=  overrun > 0 ? (bits >> overrun) : bits;

   // Check if there is enough room in the current word
   if (overrun >= 0)
   {
      // There is no need to clear any set bits beyond
      // the overrun bits, they will eventually by
      // shifted out the top bit.
      ostream.write (m_cur);


      #ifndef __SYNTHESIS__
         nbidx += left;

         /*  !!! STRIP - remove output 2018-06-28
         std::cout << "BS64:" << ostream.name() << ':'
                   << std::setfill ('0') << std::hex << std::setw (16) << m_cur
                   << ':'       <<  std::hex << std::setw(8) << std::setfill (' ') << bits
                   << " len = " <<  std::hex << std::setw(4) << std::setfill (' ') << nbidx << std::endl;
         */
      #endif

      m_cur   = bits;
   }

   m_idx += nbits;

   #ifndef __SYNTHESIS__
   ///   std::cout << "BS64:" << ostream.name () << "len = " << m_idx << std::endl;
   #endif
   return;
}


#endif
