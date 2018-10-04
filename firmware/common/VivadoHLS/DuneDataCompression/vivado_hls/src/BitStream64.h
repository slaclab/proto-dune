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

\* ---------------------------------------------------------------------- */


#include <stdint.h>
#include <ap_int.h>

class OStream;


/* ---------------------------------------------------------------------- *//*!
 *
 *   \class BitStream64
 *   \brief Holds the current context, except the actual buffer for filling
 *          a 64-bit bit stream
 *
\* ---------------------------------------------------------------------- */
class BitStream64
{
public:
   BitStream64 ();

   /// !!! KLUDGE !!!
   /// THIS IS WRONG WIdx_t must be 8 bits and Idx_t must be 14 bits, but
   /// it fails the build.  It succeeded at 7 and 13, try resorting
   /// 10/01/2018  Restore WIdx_t to 8 bits
   /// 10/01/2018  m_bcnt is really not used so try sitting its width to 1
   //              before eliminating it altogether



   // Not the only 128 bits are reserved
   // Only the Compressed bit stream needs this mask and with a maximum of
   // 80 64-bit words (rounded out to 128) only 7 bits are needed.
   //
   // But the word index, which indexes the number of 64-bit words needs to
   // be large enough to handle the larger bit stream associated with the
   // histogram and symbol overflows.  This is a maximum of around 215
   // 64-bit words, rounded up to 256, so 8 bits are needed.
   //
   // The total bit count index must be the some of these two (8+6)
   typedef ap_uint<128>  Msk_t;   /*!< Bit mask for normal/overflow words */
   typedef ap_uint<7>   MIdx_t;   /*!< Bit mask index                     */
   typedef ap_uint<8>   WIdx_t;   /*!< Word index                         */
   typedef ap_uint<6>   BIdx_t;   /*!< Bit index (within a 64-bit word)   */
   typedef ap_uint<1>    Idx_t;   /*!< Total bit index                    */


   enum class OvrLo
   {
      Bits     =  0,
      NShard   = 12,
      NPending = 18,
      NBits    = 32
   };

   enum class OvrHi
   {
      Bits     = 11,
      NShard   = 17,
      NPending = 31,
      NBits    = 34
   };

   enum class OvrSize
   {
      Bits     = 12,
      NShard   =  6,
      NPending = 14,
      NBits    =  4

   };

public:
   typedef ap_uint<(int)OvrSize::   NBits>   OvrNBits_t;    /*!< Number of 'same'   bits       */
   typedef ap_uint<(int)OvrSize::NPending>   OvrNPending_t; /*!< Number of pending  bits       */
   typedef ap_uint<(int)OvrSize::  NShard>   OvrNShard_t;   /*!< Number of leftover bits       */
   typedef ap_uint<(int)OvrSize::    Bits>   OvrBits_t;     /*!< Bit pattten to insert         */

   static uint64_t compose (OvrNBits_t nbits, OvrNPending_t npending, OvrNShard_t nshard,
                            OvrBits_t   bits)
   {
      uint64_t over = (nbits, npending, nshard, bits);
      return   over;
   }

   static int    extract (ap_uint<64> ovr, OvrHi hi, OvrLo lo)
   {
      int    val = ovr.range (static_cast<int>(hi), static_cast<int>(lo));
      return val;
   }

   static void    extract (uint64_t       ovr64,
                           OvrNBits_t    &nbits,
                           OvrNPending_t &npending,
                           OvrNShard_t   &nshard,
                           OvrBits_t     &bits)
   {
      ap_uint<64> ovr = ovr64;
      nbits    = extract (ovr, OvrHi::NBits,     OvrLo::NBits);
      npending = extract (ovr, OvrHi::NPending,  OvrLo::NPending);
      nshard   = extract (ovr, OvrHi::NShard,    OvrLo::NShard);
      bits     = extract (ovr, OvrHi::Bits,      OvrLo::Bits);
      return;
   }

public:
   void               reset ();
  int                 flush (OStream &ostream);
   int                bidx  () const;
   int                widx  () const;
   void           inc_widx  ();

   void               write (OStream &ostream, uint64_t wrd);
   void              insert (OStream &ostream, uint64_t bits, ap_uint<7> nbits);
   void            transfer (OStream &ostream);

public:
   BitStream64::Msk_t       m_omsk; /*!< Which wwords are overflow words  */
   BitStream64::BIdx_t      m_bidx; /*!< Bit index into m_cur             */
   BitStream64::WIdx_t      m_widx; /*!< Current word index               */
   uint64_t                  m_cur; /*!< Output staging buffer            */
};
/* ---------------------------------------------------------------------- */


#define USE_FIFO 0
#if USE_FIFO
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

#else

/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief The output stream context.
 *
 *   The values in here are all written once on the producer's side and
 *   read once on the consumer side.  Those values that are really
 *   read/write are contained in BitStream64 and transfer to this class
 *   when the stream is ready to be written.
 *
\* ---------------------------------------------------------------------- */
class OStream
{
public:

#ifndef __SYNTHESIS__

   OStream (const char *name) : m_name (  name) { return; }
   OStream ()                 : m_name ("Test") { return; }
   char const    *name ()                { return m_name; }
   char const  *m_name;

#else

   OStream () { return; }

   #endif

public:

   void write (uint64_t wrd, int idx)
   {
      m_buf[idx] = wrd;
   }

   uint64_t read (int idx)
   {
      uint64_t data = m_buf[idx];
      return data;
   }

public:
    BitStream64::Msk_t   m_omsk; /*!< Which wwords are overflow words     */
    BitStream64::BIdx_t  m_bidx; /*!< Number of bits in the shard         */
    BitStream64::WIdx_t  m_widx; /*!< Number of words in the output buffer*/
    uint64_t            m_shard; /*!< Last shard                          */
    uint64_t         m_buf[256]; /*!< The buffer                          */
};
/* ---------------------------------------------------------------------- */
#endif




__inline BitStream64::BitStream64 () :
      m_omsk   (0),
      m_bidx   (0),
      m_widx   (0),
      m_cur    (0)
{
   #pragma HLS INLINE
   return;
}


__inline void BitStream64::transfer (OStream & ostream)
{
   ostream.m_omsk  = m_omsk;
   ostream.m_bidx  = m_bidx;
   ostream.m_widx  = m_widx;
   ostream.m_shard = m_cur;
}

__inline int BitStream64::flush (OStream &ostream)
{
   #pragma HLS INLINE
   int cur_widx = widx ();
   int cur_bidx = bidx ();

   // If anything in the current buffer, write it out
   if (cur_bidx != 0)
   {
      m_cur <<= 0x40 - cur_bidx;

      #if USE_FIFO
         ostream.write (m_cur);
      #else
         ostream.write (m_cur, cur_widx);
         ostream.m_widx = cur_widx + 1;
      #endif
   }

   ostream.m_bidx  = cur_bidx;
   ostream.m_shard = 0;

   int bcnt = cur_widx;
   bcnt = (bcnt << 6) | cur_bidx;
   return bcnt;
}

__inline int BitStream64::bidx () const
{
   #pragma HLS INLINE
   return m_bidx;
}

__inline int BitStream64::widx () const
{
   #pragma HLS INLINE
   return m_widx;
}

__inline void BitStream64::inc_widx ()
{
   m_widx += 1;
   return;
}


__inline void BitStream64::write (OStream &ostream, uint64_t wrd)
{
   int widx = m_widx;
   ostream.write (wrd, m_widx++);
}



__inline void BitStream64::insert (OStream &ostream, uint64_t bits, ap_uint<7> nbits)
{
   #pragma HLS PIPELINE
   #pragma HLS INLINE

   ///std::cout << "Adding # bits = " << nbits << std::endl;

   int        cur_bidx = bidx ();
   ap_uint<7>     left = 64 - cur_bidx;
   int         overrun = nbits - left;

   #ifndef __SYNTHESIS__
      int nbidx = (m_widx << 64) | m_bidx;
   #endif

   ap_uint<7> shift  =  overrun > 0 ? left : nbits;
   m_cur    <<= shift;
   m_cur     |=  overrun > 0 ? (bits >> overrun) : bits;

   // Check if there is enough room in the current word
   if (overrun >= 0)
   {
      // There is no need to clear any set bits beyond
      // the overrun bits, they will eventually by
      // shifted out the top bit.
      #if USE_FIFO
          ostream.write (m_cur);
      #else
         int wrdIdx = widx ();
         ostream.write (m_cur, wrdIdx);
         inc_widx ();
      #endif

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

   m_bidx += nbits;

   #ifndef __SYNTHESIS__
   ///   std::cout << "BS64:" << ostream.name () << "len = " << m_idx << std::endl;
   #endif
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class OverElement
{
public:
   typedef ap_uint<4>   OvrNBits_t;    /*!< Number of 'same'   bits       */
   typedef ap_uint<14>  OvrNPending_t; /*!< Number of pending  bits       */
   typedef ap_uint<6>   OvrNShard_t;   /*!< Number of leftover bits       */
   typedef ap_uint<12>  OvrBits_t;     /*!< Bit pattten to insert         */
   typedef ap_uint<64>  OvrShard_t;    /*!< Leftover bits                 */

public:
   OverElement () { return; }
   OverElement (OvrNBits_t nbits, OvrNPending_t npending, OvrNShard_t nshard,
                OvrBits_t   bits,                         OvrShard_t   shard) :
            m_nbits    (nbits),
            m_npending (npending),
            m_nshard   (nshard),
            m_bits     (bits),
            m_shard    (shard)
{
    return;
}

public:
   OvrNBits_t        m_nbits;
   OvrNPending_t  m_npending;
   OvrNShard_t      m_nshard;
   OvrBits_t          m_bits;
   OvrShard_t        m_shard;
};
/* ---------------------------------------------------------------------- */
#endif
