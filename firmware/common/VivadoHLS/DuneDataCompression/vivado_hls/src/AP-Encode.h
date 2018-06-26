// -*-Mode: C;-*-

/* ---------------------------------------------------------------------- *//*!

   \file  AP-Encode.c
   \brief Arithmetic Word Encoder, implementation file
   \author JJRussell - russell@slac.stanford.edu

    Implementation of the routines to encode symbols using an arithmetic
    probability encoding technique.
                                                                          */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\
 *
 * HISTORY
 * -------
 *
 * DATE     WHO WHAT
 * -------- --- ---------------------------------------------------------
 * 08.17.10 jjr Eliminated local copy of FFS.ih in favor of PBI version
 * 01.14.09 jjr In the encode and encode_list routines, rephrased the 
 *              auto-increment expressions on the cast values. The newer
 *              versions of  the gcc compiler do not allow casting of
 *              lvalues.
 * 09.13.06 jjr Merged changes from production branch. The changes are
 *              what is noted in the 05.04.06 fix.
 * 05.04.06 ohs Fixed bug in bits_plus_follow when _to_follow > 32
 *          jjr Note also that Owen corrected the cnt = FFS (xor) -1
 *              to the equivalent, but more formally correct
 *              cnt = FFS (xor) - (32 - APE_K_BITS)
 * 09.21.05 jjr Eliminated PBS/FFS.ih in favor of the local copy
 * 09.21.05 jjr Moved from ZLIB
 *
\* ---------------------------------------------------------------------- */

#define APC_K_NBITS (PACKET_B_NSAMPLES+2)  /* # of bits in code value     */

#include "AP-Common.h"
#include "BitStream64.h"
#include "Histogram-Encode.cpp"

#include <stdint.h>
#include <ap_int.h>
#include <assert.h>


/* ---------------------------------------------------------------------- *\
 * Compile time configuration constants
 * ------------------------------------
 *
 *  APE_DUMP    -- when non-zero, dumps encoding details
 *  APE_CHECKER_-- when non-zero, check the sophisicated reduction method
 *                 against a straight-forward implementation. This is
 *                 likely needed only when the reduction method is dorked
 *                 with.
\* ---------------------------------------------------------------------- */
#if    !defined(APE_DUMP)    && !defined(__SYNTHESIS__)
#define APE_DUMP 0
#endif

#if    !defined(APE_CHECKER) && !defined(__SYNTHESIS__)
#define APE_CHECKER 1
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
typedef ap_uint<10>          APE_table_t;
typedef ap_uint<10+2>        APE_cv_t;
typedef ap_uint<10+3>        APE_range_t;
typedef ap_uint<10 + (10+2)> APE_scaled_t;
typedef ap_int<10+3>         APE_xscv_t; /* Extended signed reverseo of cv_t */
typedef ap_uint<4>           APE_cvcnt_t;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct  APE_instruction
 *  \brief  The data/instructions needed to encode an symbol value
 *
\* ---------------------------------------------------------------------- */
struct APE_instruction
{
   ap_uint< 4>     m_nbits; /*!< The number of valid bits in m_bits       */
   ap_uint<12>      m_bits; /*!< The bit pattern to insert                */
   ap_uint< 8>  m_npending; /*!< The number of pending bits               */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class   APE_cv
  \brief   Defines the limits of the condition value
                                                                          */
/* ---------------------------------------------------------------------- */
class APE_cv
{
public:
   APE_cv () { return; }

public:
   void inline    scale  (APE_table_t lo, APE_table_t hi);
   void inline reduce_hi (APE_cvcnt_t nreduce, APE_cv_t hi_mask);
   void inline reduce_lo (APE_cvcnt_t nreduce);

   static inline APE_cv_t scale_hi (APE_cv_t        lo,
                                    APE_range_t  range,
                                    APE_table_t  value);

   static inline APE_cv_t scale_lo (APE_cv_t       lo,
                                    APE_range_t range,
                                    APE_table_t value);

public:
   APE_cv_t m_hi;  /*!< The high limit of the condition value             */
   APE_cv_t m_lo;  /*!< The low  limit of the condition value             */
};
/* ---------------------------------------------------------------------- */


class APE_etxOut
{
public:
   APE_etxOut ();

public:
   OStream               ha; /*!< Output bit array (histogram+ overflow)   */
   OStream               ba; /*!< Output bit array (encoded symbols        */
};

/* ---------------------------------------------------------------------- *//*!

  \class _APE_etx
  \brief   Encoding context

   While this is defined in the public interface, this structure should
   be treated like a C++ private member. All manipulation of this
   structure should be through the APE routines.
                                                                          */
/* ---------------------------------------------------------------------- */
class APE_etx
{
public:
  APE_cv                cv; /*!< Current lo/hi limits                     */
  BitStream64           ha; /*!< The encoded histogram/overflow data      */
  BitStream64           ba; /*!< The encoded symbols                      */
  unsigned int    tofollow; /*!< Used when lo,hi straddle the .5 boundary */
  int                nhist; /*!< Number of bits in the encoded histogram  */
};
/* ---------------------------------------------------------------------- */

APE_etxOut::APE_etxOut ()
{
   // Declare the encoding context and provide enough stream buffering
   // to handle the worse case
   //    ha - contains the histograms and the overflow data
   //    ba - contains the encoded bits
   //
   // Worse case to ha is
   //    nbins * PACKET_B_NSAMPLES + every bin overflowing
   // with nbins = 32 and NSAMPLES = 10 bits = ~320 bits and 13 bit symbols
   //     32 * 10 + 13 * 1024 = 13,362 -> 213 64 bit words
   //
   // Worse case for ba is a completely flat distribution
   // with nbins = 32 and NSAMPLES = 1024 bits this is
   //     5 bits * 1024 = 5,120 bits -> 80 64 bit words
   //
   // Round these to 256 and 128
   #pragma HLS STREAM          variable=ha depth=256 dim=1
   #pragma HLS STREAM          variable=ba depth=128 dim=1

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if APE_CHECKER  /* DIAGNOSTIC PURPOSES ONLY                              */
/* ---------------------------------------------------------------------- *//*!
 *
 *  \class  APE_checker
 *  \brief  Class used to check the reduction of the code values low
 *          and limits and to produce the resulting bit pattern.
 *
 *  \note
 *   This used entirely for debugging.  It checks the fairly sophisticated
 *   method used in the actual code against a very straight-forward
 *   implementation.
 */
class APE_checker
{
public:
      APE_checker (int npending ) : m_npending (npending) { return; }

public:
      void        reduce (APE_cv const &cv);
      bool        check  (APE_cv const &cv, int npending,  int nbits,  uint32_t bits);
      static void put_bit_plus_pending (uint32_t &bits, bool      bit, int &npending);

public:
   uint16_t    m_low;  /*!< The reduced low  limit of the code value      */
   uint16_t   m_high;  /*!< The reduced high limit of the code value      */
   int    m_npending;  /*!< The old and new number of pending bits        */
   uint32_t   m_bits;  /*!< The bit pattern resulting from the reduction  */
   int       m_nbits;  /*!< The number of bits in the bit pattern         */
};
/* ---------------------------------------------------------------------- */
#define APE_checkerStatement(_statement) _statement
/* ---------------------------------------------------------------------- */
bool encode_check (APE_etxOut                         &etx,
                   Histogram const                   &hist,
                   Symbol_t  const syms[PACKET_K_NSAMPLES]);
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define APE_checkerStatement(_statement)
#define encode_check(_etx, _hist,_syms) failure

/* ---------------------------------------------------------------------- */
#endif   /* APE_CHECKER                                                   */
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#if     APE_DUMP
/* ---------------------------------------------------------------------- */
#define APE_dumpStatement(_statement) _statement
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define APE_dumpStatement(_statement)
/* ---------------------------------------------------------------------- */
#endif  /* APE_CHECKER                                                    */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/* LOCAL PROTOTYPES                                                       */
/* ---------------------------------------------------------------------- */
static inline uint32_t compose (APE_cv_t bits, int nbits, int npending);
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn         int  APE_start (APE_etx       *etx)

  \brief      Initializes the encoding context.
  \retval 0,  Currently, always 0. This may expand to a status code if
              necessary

  \param etx  The encoding context to initialize

   Note that since the AWx routines are bit encoders/decoders, one needs to
   specify the offsets into the buffers as bit offsets.
                                                                          */
/* ---------------------------------------------------------------------- */
static int inline APE_start (APE_etx &etx)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   /* Set the encoding context */
   etx.cv.m_lo   = 0;
   etx.cv.m_hi   = APC_K_HI;
   etx.tofollow  = 0;


   return 0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn     unsigned int APE_finish (APE_etx *etx)
  \brief  Finishes the encoding by flushing out any currently buffered bits
  \retval If > 0, the number of encoded bits.
          If < 0, the number of bits needed to encode the final piece

  \param  etx  The encoding context

  \par
   At the end of the encoding, there is some context that needs flushing.
   This flushing may contribute more bits to the output stream.
                                                                          */
/* ---------------------------------------------------------------------- */
static int inline APE_finish (APE_etxOut &etxOut, APE_etx &etx)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   int             tofollow = etx.tofollow + 1;
   ap_uint<1>           bit = etx.cv.m_lo >> (APC_K_NBITS - 2);

   uint32_t buf = compose (bit, 1, tofollow);
   etx.ba.insert  (etxOut.ba, buf, 1 + tofollow);

   etx.ba.transfer (etxOut.ba);

   //   printf ("Total bits = %u\n", etx->ba.m_idx);
   // Return the number of bits
   return etx.ba.m_idx;
}
/* ---------------------------------------------------------------------- */




// --------------------------------------------------
// If either defined,
// These are diagnostic statements that are in common
// --------------------------------------------------
#if     APE_DUMP || APE_CHECKER
#define APE_sharedStatement(_statement) _statement
#else
#define APE_sharedStatement(_statement)
#endif
// --------------------------------------------------


/* ---------------------------------------------------------------------- *//*!

  \fn    int  APE_encode (APE_etx              *etx,
                          APE_table_t  const *table,
                          Symbol_t     const  *syms,
                          int                nsymss)

  \brief          Encodes an array of symbols
  \return         Number of symbols remaining to be encoded. If this is
                  0, all symbols where encoded.

  \param     etx  The encoding context
  \param   table  The encoding table
  \param    syms  The array  of symbols to encode
  \param   nsyms  The number of symbols
                                                                          */
/* ---------------------------------------------------------------------- */
static int APE_encode  (APE_etxOut        &etxOut,
                        Histogram    const  &hist,
                        Symbol_t     const  *syms,
                        int                 nsyms)
{
   #pragma HLS INLINE off
   APE_etx etx;

   Histogram::Table table[Histogram::NBins+1];
   APE_start (etx);


   //static APE_instruction   instructions[1024];
   //APE_instruction *instruction = instructions;

   // Setup the APE decoding context
   AdcIn_t prv = *syms++;
   hist.encode (etxOut.ha, etx.ha, table, prv);
   etx.nhist = etx.ha.m_idx;   /// DEBUG
   ////APE_dumpStatement (hist.print (0));


   int    npending   = etx.tofollow;
   APE_cv         cv = etx.cv;


   // DEBUGGING/DIAGNOSITC
   APE_checkerStatement (APE_checker chk (npending));
   APE_checkerStatement (Symbol_t const *syms_beg = syms - 1);


   APE_ENCODE_LOOP:
   while (--nsyms > 0)
   {
      //////#pragma HLS ALLOCATION instances=insert limit=1 function
      #pragma HLS PIPELINE II=1
      #pragma HLS UNROLL factor=1

       Histogram::Symbol_t ovr;
       AdcIn_t             cur = *syms++;
       Histogram::Symbol_t sym = Histogram::symbol (cur, prv);
       Histogram::Idx_t    idx = Histogram::idx    (sym, ovr);
       prv = cur;

       cv.scale (table[idx], table[idx+1]);

       if (idx == 0)
       {
          etx.ha.insert (etxOut.ha, ovr, hist.m_nobits);
       }

       // ----------------------------------------------------
       // This is a diagnostic debugging method used to check
       // the more sophisticated reduction method that follows
       // against this very straight-forward implementation.
       // ----------------------------------------------------
       APE_checkerStatement (chk.reduce (cv));
       APE_dumpStatement    (APE_cv cv_scaled = cv);


       // ---------------------------------------------
       // Keep the LO and HI in the middle of the range
       // ---------------------------------------------

       // ------------------------------------------
       // Count the number of bits that are the same
       // ------------------------------------------
       APE_cv_t     diff = (cv.m_lo ^ cv.m_hi);
       APE_cvcnt_t nsame = diff.countLeadingZeros ();


       // ----------------------------------------------------------------
       // Check for a pattern straddles the Q1,q2 boundary and extends
       // till the carry is resolved.
       //
       // This is tricky.  To get the maximum parallelism. want this
       // calculation to be as independent as possible of the calculation
       // of nsame.
       //
       // Because hi >= lo, the first observation is that the next lo and hi
       // bit after the same bits must be lo = 0 and hi = 1. The logic
       // is there are only 4 possibilites
       //
       //     lo  hi   Meaing
       //      0   0   Impossible, would be included in nsame
       //      1   0   Impossible, lo cannot be > hi
       //      0   1   Okay, the only possible pattern
       //      1   1   Impossible, would be included in nsame
       //
       // The unresolved carry bits (if any) start immediately after this and
       // continue as long as lo = 1 and hi = 0. This ceases to be the case
       // at the first occurence of either lo = 0 or hi = 1. So the expression
       //
       //        ~lo | hi = 1 selects this pattern
       //
       // Consider this pattern
       //     lo  hi  ~lo | hi      Result
       //      0   0    1 |  0 = 1  Terminated
       //      0   1    1 |  1 = 1  Terminated
       //      1   0    0 |  0 = 0  Continued
       //      1   1    0 |  1 = 1  Terminated
       //
       // Now consider this anded with the bit masks of the differences
       //
       //         <nsame>1<11111><0,1,0>
       //              diff = 00000001<11111><0,1,0>
       //              lo   = xxxxxxx1<11111><0,0,1>
       //              hi   = yyyyyyy1<00000><0,1,1>
       //          ~lo|hi   = zzzzzzz1<000000>1,1,1>
       //  (lo|hi)&diff     = 00000001<00000><0,1,1>
       //
       //  This is almost what is needed except the diff mask selects
       //  the leading bit and fails to correctly select the terminating
       //  bit. This is easily rectified by just shift ~lo|hi up 1 bit.
       //
       //  This isn't an accident. The number of differ bits in a row
       //  after the lo = 0, hi = 1 bit must at least the number of
       //  carry bits.  So these bits select the carry bits and the
       //  termination bit. (The | 1 ensures termination in the case
       //  of cv.m_lo = cv.m_hi.
       //
       //  The final detail is that the reduction of cv.m_hi depends
       //  on whether there are pending bits. The straight-forward
       //  method would be use the calculated number of pending bits.
       //  However this means that one must wait until that number is
       //  calculated, This introduces too much detail, so the out is
       //  to note all that is needed is not the number of pending bits
       //  but whether there are any at all. To do this, only the bits
       //  immediately after the hi = 1, lo = 0 bits that terminate
       //  the number of same bits is hi = 0 and lo = 1  The following
       //  calculation does the appropriate ~hi & lo to find all such
       //  bits which is then moved to the top bit (the << (nsame+1))
       //  and then isolated with the & APC_M_CV_TOP.  (Note that this
       //  also works in the degenerate cases of both name = 11 or 12.
       //  In both cases, m_lo == 0 so the and cannot be true.)
       // ---------------------------------------------------------------
       APE_cv_t hi_mask = ((cv.m_lo & ~cv.m_hi)) << (nsame);
       hi_mask <<= 1;
       hi_mask   = hi_mask & APC_M_CV_TOP;


       // ------------------------------------------------------------
       // Find the first bit that terminates the set of pending bits
       // After nreduce has been calculated, everything that is needed
       // to calculate the new reduced values of cv.m_lo,m_hi are now
       // known, so that calculation can proceed in parallel with the
       // bit stuffing.
       // ------------------------------------------------------------
       APE_cv_t    pending = (((~cv.m_lo | cv.m_hi) << 1) |  1) & diff;
       APE_cvcnt_t nreduce = pending.countLeadingZeros ();


       // ------------------------------------------------------------
       // nreduce  = number of bits to reduce lo and hi by
       //            this <# same bits> + <# of new pended bits>
       // mpending = number of newly pended bits
       // bits     = the bit pattern that is the same in lo and hi
       // ------------------------------------------------------------
       APE_cvcnt_t mpending  = nreduce - nsame;
       APE_cv_t    lo        = cv.m_lo;


       // ------------------------------------------------------
       // npending_save, ebits and nbits are for debugging only
       // They are checked against the pattern produced by the
       // chk::reduce method.
       // ---------------------------------------------
       APE_cv_t  bits;
       uint32_t ebits;
       int     nebits;
       APE_dumpStatement (int npending_save = npending);
       //bool   finish = (idx == 0) || (nsyms == 1);
       if (nsame) /// || finish)
       {
          bits  = lo >> (lo.length () - nsame);

          APE_ENCODE_STUFF:
#if 0
          instruction->m_nbits    = nsame;
          instruction->m_bits     = bits;
          instruction->m_npending = npending;
          instruction            += 1;
#else
          ebits  = compose (bits, nsame, npending);
          nebits = nsame + npending;
          etx.ba.insert  (etxOut.ba, ebits, nebits);
#endif
          // -----------------------------------------------
          // If finishing up for either because we are done
          // or are encoding an overflow symbol.
          // -----------------------------------------------
#if 0
          if (finish)
          {
             ap_uint<1>  bit = etx.cv.m_lo >> (APC_K_NBITS - 2);
             uint32_t buf = compose (bit, 1, mpending + 1);
             if (idx == 0)
             {
                buf <<= 12;
                buf  |= sym;
             }
             ba.insert (etxOut.ba, buf, 2 + mpending + 12);

             cv.m_lo   = 0;
             cv.m_hi   = APC_K_HI;
             npending  = 0;
             continue;

          else
          {
             //APE_sharedStatement (ebits =) ba.insert (etxOut.ba, bits, nsame, npending);
             npending = mpending;
          }
#else
          npending = mpending;
#endif
       }
       else
       {
          APE_dumpStatement   (  bits = 0);
          APE_sharedStatement ( ebits = 0);
          APE_sharedStatement (nebits = 0);
          npending += mpending;
       }


       // ---------------------------------------------------------
       // Update the CV values
       // For lo: The vacated bits are set to 0
       //     hi: The vacated bits are set to 1
       //
       // While this seems simple, one must overcome the fact that
       // C left shift operator (<<) always shifts in 0s. For the
       // m_lo, this is what you want, but for m_hi, you want 1s
       // To meet timing reduce_hi is a try convoluted.
       // ---------------------------------------------------------
       cv.reduce_lo (nreduce);
       cv.reduce_hi (nreduce, hi_mask);


       // ------------------------------
       // Diagnostic print out and check
       // ------------------------------
       APE_dumpStatement   (dump (1023 - nsyms, sym, cv_scaled, nsame, bits,
                                  npending_save, cv,    nebits, ebits));
       APE_checkerStatement (chk.check (cv, npending, nebits, ebits));
   }


 EXIT:
   etx.cv        = cv;
   etx.tofollow  = npending;

   etx.ha.transfer (etxOut.ha);
   APE_finish (etxOut, etx);

   return nsyms;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Composes the bit pattern of \a nbits of \a bits with
 *          \a npending
 *
 *   \param[in]  bits    The bit pattern to insert
 *   \param[in] nbits    The number of relevant bits
 *   \param[in] npending The number of pending bits. These are essentially
 *                       the number of previously unknown bits because of
 *                       an unresolved carry.
 *
 *  \par
 *  \a npending bits will be inserted immediately after MSB of bits. These
 *  bits will be 0 in the case of the MSB being 1 and 0 if the MSB is 1.
 *  The remaining \a nbits - 1 will then be inserted for a total of
 *  \a nbits + \a npending bits
 *
\* ---------------------------------------------------------------------- */
static inline uint32_t compose (APE_cv_t bits, int nbits, int npending)
{
   #pragma HLS PIPELINE
   #pragma HLS INLINE

   //   std::cout << " inserting nbits + npending = " << std::dec
   //             << nbits << '+' << to_follow << std::endl;

   // ----------------------------------------------------
   // !!! KLUDGE !!!
   // The total number of bits to insert cannot exceed 32.
   // This can be fixed, but at a later date
   // ----------------------------------------------------
   assert (npending + nbits <= 32);


   uint32_t buf = 0;
   int     nbuf = npending + nbits;


   // Check if any following bits
   // These are either 0 or 1 depending on the sign of most significant bit in bits
   if (npending)
   {
      nbits -= 1;
      uint32_t leading_bit = bits[nbits];
      buf |=  leading_bit << (nbuf-1);
      if (leading_bit == 0)
      {
         // Leading bit is 0, must insert 'to_follow' 1s
         buf |= ((1 << npending) - 1) << nbits;
      }
      else
      {
         // Leading bit is 1, must clear it
         bits.clear (nbits); // ^= 1 << nbits;
      }
   }

   buf |= bits;
   return buf;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/* IMPLEMENTATION: APE_cv                                                 */
/* ---------------------------------------------------------------------- *//*!

   \fn __inline  APE_cv::scale (APE_table_t const table[2])

   \brief  Scales the input lo and hi condition value/limits using the
           table values

   \param[in]     table  The table values
                                                                          */
/* ---------------------------------------------------------------------- */
inline void APE_cv::scale (APE_table_t lo, APE_table_t hi)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   APE_range_t range;

   range  = m_hi;
   range -= m_lo;
   range += 1;

   APE_cv_t t1 = hi; ///table[1];
   APE_cv_t t0 = lo; ///table[0];

   m_hi = scale_hi (m_lo, range, t1);
   m_lo = scale_lo (m_lo, range, t0);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Scales the hi limit of the code values
 *   \return The new lo limit
 *
 *   \param[in]    lo  The current low liimt to
 *   \param[in] range  The range of the probablity interval
 *   \param[in] value  The high edge of the probability interval
 *
\* ---------------------------------------------------------------------- */
inline APE_cv_t APE_cv::scale_hi (APE_cv_t        lo,
                                  APE_range_t  range,
                                  APE_table_t  value)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   APE_cv_t val;

   // -------------------------------------------------------
   //
   // The code has been arranged such that 2**N - 1 samples
   // are compressed.  This means that the cumulative
   // probability saturates at 2**N -1 so only N bits, not
   // N + 1 bits are needed in the APE_table_t.  This scaling
   // is still done is 2**N so it can be implemented with a
   // shift, not an expensive divide.
   //
   // So this whole business below trying to handle this
   // problem can be ignored. It turns out that this selection
   // pushed one over a timing limit. (That was the motivation
   // for encoding only 2**N - 1 and not 2**N samples.
   // --------------------------------------------------------

   ////
   /// When value = 0, this really represents value = MAX
   //// i.e. 1 << APC_K_NORM_NBITS
   //// So if value = 0, the multiple by this value cancels
   //// the scaling shift;
   ////if (value == 0)
   ///{
   ///   val = range + lo - 1;
   ///}
   ///else
   {
      // ---------------------------------------------------
      // Need to scale lo up by APC_K_NORM_NBITS
      // This allows the calculation of the returned
      // value to be mapped to a DSP multiply and accumulate
      // ---------------------------------------------------
      APE_scaled_t xlo = lo - 1;
      xlo <<= APC_K_NORM_NBITS;



      // -------------------------------------------------------
      // This is the straightforward calculation but cannot map
      // to a*b + c as does the following form.
      // ------------------------------------------------------
      ////val = ((range * value) >> APC_K_NORM_NBITS) + lo -1;

      val = ((range * value + xlo)>> APC_K_NORM_NBITS);
   }

   //////val = val - 1 + lo;

   //std::cout << "scale_hi lo:" << lo << " range:" << range << " value:" << value
   //          << " -> " << val  << std::endl;

   return val;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Scales the lo limit of the code values
 *   \return The new lo limit
 *
 *   \param[in]    lo  The current low liimt to
 *   \param[in] range  The range of the probablity interval
 *   \param[in] value  The low edge of the probability interval
 *
\* ---------------------------------------------------------------------- */
inline APE_cv_t APE_cv::scale_lo (APE_cv_t       lo,
                                  APE_range_t range,
                                  APE_table_t value)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   // ---------------------------------------------------
   // Need to scale lo up by APC_K_NORM_NBITS
   // This allows the calculation of the returned
   // value to be mapped to a DSP multiply and accumulate
   // ---------------------------------------------------
   APE_scaled_t xlo = lo;
   xlo <<= APC_K_NORM_NBITS;

   // -------------------------------------------------------
   // This is the straightforward calculation but cannot map
   // to a*b + c as does the following form.
   // ----------------------------------------------------
   APE_cv_t val = ((range * value + xlo) >> APC_K_NORM_NBITS);


   //std::cout << "scale_lo lo:" << lo << " range:" << range << " value:" << value
   //          << " -> " << val  << std::endl;

   return val;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Reduce the hi value getting it ready for the next round of
 *          encoding
 *
 *  \param[in] nreduce  The number of higher order bits to discord.
 *                      The vacate lower order bits are replaced with 1s
 *  \param[in] hi_mask  This is either the top bit of APE_cv_t or 0
 *                      depending on the whether there are pending bits
 *                      or not.
 *
\* ---------------------------------------------------------------------- */
inline void APE_cv::reduce_hi (APE_cvcnt_t nreduce, APE_cv_t hi_mask)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   // -------------------------------------------------------
   // This is an elaborate ruse to get nreduce 1s shifted
   // in from the right.  THe observation to make is that
   // a right shift on a signed value will drag the sign bit.
   // But we need a left shift, so the trick is to
   //  1. reverse the bits of m_hi
   //  2. or this value in a integer one bit wider than m_hi
   //  3. shift this value right by nreduce
   //  4. Put back in m_hi, this truncates the extra sign bit
   //  5. Reverse the bits back
   //  6. Or in the hi_mask
   //
   // Now the only reason for doing this is that Vivado HLS
   // is smart enough to replace all this with very straight
   // forward code the drags a  1s in from the least signficant
   // bits.  Effectively if one had an operation, x <<= n,
   // which instead of shifting in 0s, it shifted in 1s.
   //
   // This method is allows the timing to be met.
   // ---------------------------------------------------------
   APE_xscv_t r (1 << 12);    // Set the sign bit
   APE_xscv_t tmp = m_hi.reverse ();

   r    |= tmp;  ///m_hi.reverse ();   // Or in the reversed value
   r   >>= nreduce;           // Shift and drag sign bit

   m_hi  = r;                 // Discard the sign bit
   m_hi  = m_hi.reverse ();   // Reverse to nominal order

   m_hi |= hi_mask;           // Or in the hi_mask

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Reduce the lo value getting it ready for the next round of
 *          encoding
 *
 *  \param[in] nreduce  The number of higher order bits to discord.
 *                      The vacate lower order bits are replaced with 1s
 *
\* ---------------------------------------------------------------------- */
inline void APE_cv::reduce_lo (APE_cvcnt_t nreduce)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   // As opposed reducing the high, when shifting out the upper bits
   // the vacated lower bits are filled with 0s.  This is the definition
   // of the C shift (<<) operator, so the elaborate ruse needed to
   // trick Vivado HLS to occupy the vacated bits in m_hi is not needed.
   m_lo  = (m_lo << nreduce) & APC_M_CV_M1;

   return;
}
/* ---------------------------------------------------------------------- */
/* END: IMPLEMENTATION APE_cv                                             */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if APE_CHECKER
/* --------------------------------------------------------------------- *//*!
 *
 *  \brief Adds the bit plus any pending bits
 *
 *  \param[in:out]     bits  The bit pattern to populate
 *  \param[in]          bit  The bit to add
 *  \param[inLout] npending  The number of npending bits, returned as 0
 *
\* -------------------------------------------------------------------- */
inline void APE_checker::put_bit_plus_pending (uint32_t &bits,
                                               bool       bit,
                                               int  &npending)
{
   bits <<= 1;
   if (bit) bits |= 1;
   for ( int i = 0 ; i < npending ; i++ )
   {
      bits <<= 1;
      if (!bit) bits |= 1;
   }

   npending = 0;

  return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief Does the reduction of the low and high code value limits and
 *          computes the number and bit pattern to emit.
 *
 *   \param[in]  low  The  low code value limit
 *   \param[in] high  The high code value limit
 *
 *   \par
 *    This computation is done in the most straigh-forward way possible.
 *    It is used as a check for the more sophisticated method used in
 *    the acctual calculation.  In this sense, it is a debugging tool.
 *
 \* ---------------------------------------------------------------------- */
void APE_checker::reduce (APE_cv const &cv)
{
   uint32_t     low = cv.m_lo;
   uint32_t    high = cv.m_hi;
   uint32_t    bits = 0;
   int        nbits = 0;
   int     npending = m_npending;

   while (1)
   {
      // -------------------------------------------------
      // If both low and high are in the lower half
      // This means that their first bits match and are 0
      // -------------------------------------------------
      if ( high < APC_K_HALF )
      {
         int n  = npending;
         nbits += n + 1;
         put_bit_plus_pending (bits, 0, n);
         npending = 0;
      }

      // -------------------------------------------------
      // If both low and high are in the upper half
      // This means that their first bits match and are 1
      // -------------------------------------------------
      else if ( low >= APC_K_HALF )
      {
         int n  = npending;
         nbits += n + 1;
         put_bit_plus_pending (bits, 1, n);
         npending = 0;
      }

      // -------------------------------------------------
      // If both low and high straddle the Q2 boundary
      // This means that the next bit cannot be determined
      // until this condition is relieved
      // -------------------------------------------------
      else if ( low >= APC_K_Q1 && high < APC_K_Q3 )
      {
         npending += 1;
         low      -= APC_K_Q1;
         high     -= APC_K_Q1;
      }
      else
      {
         // All done
         break;
      }

      // The reduction of the low and high limits
      high <<= 1;
      high++;
      low <<= 1;
      high &= APC_M_CV_ALL;
      low  &= APC_M_CV_ALL;
   }

   // ------------------------
   // Capture the next context
   // ------------------------
   m_bits     = bits;
   m_nbits    = nbits;
   m_low      = low;
   m_high     = high;
   m_npending = npending;

   return;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Checks the reduction of the code value limits and the bit pattern
 *         + number of pending bits matches of the straight-forward and
 *         sophisticated methods match.
 *
 *  \param[in]       cv The code value low and high limits to check
 *  \param[in] npending The number of pending bits to check
 *  \param[in]    nbits the number of bits to check
 *  \param[in]      bit The bit pattern to check
 *
\* ----------------------------------------------------------------------- */
bool APE_checker::check (APE_cv const &cv,
                         int     npending,
                         int        nbits,
                         uint32_t    bits)
{
   if ( (m_npending != npending) ||
        (m_low      != cv.m_lo)   ||
        (m_high     != cv.m_hi)   ||
        (m_nbits    !=   nbits)   ||
        (m_bits     !=    bits))
   {
      std::cout << "ERROR" << std::endl;
      return false;
   }

   return true;
}
/* ---------------------------------------------------------------------- */
#endif  /* APE_CHECKER                                                    */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#if     APE_DUMP
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Debugging print out
 *
 *  \param[in]         isym  The index of the symbol that was encoded
 *  \param[in]          sym  The symbol being encoded.  This is a surrogate
 *                           for the ADC.
 *  \param[in]    cv_scaled  The code values after being scaled, but before
 *                           being reduced
 *  \param[in] npending_org  The number of pending bits that were forced out
 *  \param[in]   cv_reduced  The code values after being reduced
 *  \param[in]       nebits  The number of bits that this symbol commits to
 *                           tge output stream
 *  \param[in]        ebits  The bit pattern that this symbol commits to the
 *                           output stream
\* ---------------------------------------------------------------------- */
static void dump (int                 isym,
                  uint16_t             sym,
                  APE_cv const  &cv_scaled,
                  int                nsame,
                  uint32_t            bits,
                  int         npending_org,
                  APE_cv const &cv_reduced,
                  int               nebits,
                  uint32_t           ebits)
{
   std::cout
   << "Sym["   << std::setw (3) << std::hex     << isym << "] = "
   << std::setw (4) << sym
   << " Cv "   << std::setw (5) << cv_scaled.m_lo << ':'
   << std::setw (5) << cv_scaled.m_hi

   << " bits " << std::setw (4) << nsame << ':' << std::setw (6)
   << bits << " pending:" << npending_org

   << " Cv "   << std::setw (5) << cv_reduced.m_lo
   << ':' << std::setw (5) << cv_reduced.m_hi

   << " Bits " << std::setw (2) << nebits << ':' << std::setw (6) << ebits
   << std::endl;

   return;
}
/* ---------------------------------------------------------------------- */

#define APE_dumpStatement(_statement) _statement

/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */

#define APE_dumpStatement(_statement)

/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */

