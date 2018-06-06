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

class XStream : public hls::stream<uint64_t>
{
public:
   XStream (const char *name = "test") : hls::stream<uint64_t>(name) {}

#ifndef __SYNTHESIS__
public:
   const std::string &name () { return _name; }

   void setName (const char *name)
   {
      _name = name;
   }
#else
   const char *name () { return "Dummy"; }
#endif
};

class BitStream64
{
public:
   BitStream64 ();

   typedef uint32_t Idx_t;

public:
   void               set   (Idx_t idx);
   void               reset ();
   BitStream64::Idx_t flush ();
   int                bidx  () const;
   int                widx  () const;

   //void               insert (APC_cv_t bit);
   void               insert (int bits, int nbits);

public:
   void setName (const char *name)
   {
      #ifndef __SYNTHESIS__
      m_out.setName (name);
      #endif
   }

public:
   uint64_t                  m_cur; /*!< Output staging buffer                   */
   BitStream64::Idx_t        m_idx; /*!< The current bit index                   */
   XStream                   m_out; /*!< Output array                            */
};


__inline BitStream64::BitStream64 () :
      m_cur (   0),
      m_idx (   0)
{
   #pragma HLS INLINE
   #pragma HLS STREAM variable=m_out depth=192 dim=1
   return;
}

__inline void BitStream64::set (BitStream64::Idx_t idx)
{
   #pragma HLS INLINE
   m_idx = idx;
}

__inline BitStream64::Idx_t BitStream64::flush ()
{
   #pragma HLS INLINE
    int cur_idx = bidx ();

   // If anything in the current buffer, write it out
   if (cur_idx != 0)
   {
      m_cur <<= 0x40 - cur_idx;
      m_out.write (m_cur);
   }
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


__inline void BitStream64::insert (int bits, int nbits)
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
      m_out.write (m_cur);

      #ifndef __SYNTHESIS__
         nbidx += left;

         std::cout << "BS64:" << m_out.name() << ':'
                   << std::setfill ('0') << std::hex << std::setw (16) << m_cur
                   << ':' << bits << std::setfill (' ') << " len = " <<  nbidx << std::endl;
      #endif

      m_cur   = bits;
   }

   m_idx += nbits;

   #ifndef __SYNTHESIS__
   ///   std::cout << "BS64:" << m_out.name () << "len = " << m_idx << std::endl;
   #endif
   return;
}


#endif
