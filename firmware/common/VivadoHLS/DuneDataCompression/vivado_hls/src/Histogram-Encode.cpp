#include "DuneDataCompressionHistogram.h"
#include "BitStream64.h"


// -----------------------------------------------------------------------
//
// If not already defined and not doing SYNTHESIS,
// set the defaults for
//    HIST_ENCODE_DEBUG - Provides debugging information of the encoding
//    HIST_ENCODE_BTE   - Enables binary tree encoding of 32-words of
//                        the histogram bins that are only 0 or 1 count.
//                        At this point the binary tree encoding is too
//                        resource intensive to be used.  A rewrite that
//                        is FPGA friendly needs to be done.
//   HIST_ENCODE_CHECK    Checks the encoding of the histogram
//
// -----------------------------------------------------------------------
#if !defined(HIST_ENCODE_DEBUG) || defined(__SYNTHESIS__)
#define HIST_ENCODE_DEBUG 0
#endif

#if !defined(HIST_ENCODE_CHECK) || defined(__SYNTHESIS__)
#define HIST_ENCODE_CHECK 1
#endif

#ifndef HIST_ENCODE_BTE
#define HIST_ENCODE_BTE 0
#endif

#if      HIST_ENCODE_CHECK
#undef   HIST_DECODE_CHECK
#define  HIST_DECODE_CHECK 1
#endif
// -----------------------------------------------------------------------



//----------------------------------------------------
// Now of these can be used during the SYNTHESIS stage
// ---------------------------------------------------
#if     defined(__SYNTHESIS__)
#undef  HIST_ENCODE_DEBUG
#undef  HIST_DECODE_DEBUG
#undef  HIST_ENCODE_CHECK
#endif
// ---------------------------------------------------


/* ---------------------------------------------------------------------- */
#ifdef HIST_ENCODE_DEBUG
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *//*!
 *
 *  \enum  HistEncodeScheme
 *  \brief Enumerates the scheme used to encode a particular histogram bin
 *
\* ---------------------------------------------------------------------- */
enum HistEncodeScheme
{
   HES_K_DIFF = 0,   /*!< Differences                                     */
   HES_K_FX2  = 1,   /*!< Fixed 2 bit encoding                            */
   HES_K_FX1  = 2,   /*!< Fixed 1 bit encoding                            */
   HES_K_BT   = 3,   /*!< Binary tree encoding                            */
};
/* ---------------------------------------------------------------------- */


static void         hist_encode_print (bool              debug,
                                       HistEncodeScheme scheme,
                                       int                ibin,
                                       int                diff,
                                       int               nbits,
                                       uint32_t           bits,
                                       uint64_t            cur,
                                       int32_t             idx,
                                       int                left);

static void hist_encode_summary_print (bool              debug,
                                       int               nbits,
                                       int                left);


#define HistEncodeSchemeDeclare(_name)       HistEncodeScheme _name
#define HistEncodeSchemeSet(_name, _scheme) _name = _scheme

#else

#define hist_encode_print(_debug, _scheme, _ibin, _diff,       \
                          _nbits, _bits,    _cur,  _idx,  _left)

#define hist_encode_summary_print(_debug, _nbits, _left)

#define HistEncodeSchemeDeclare(_name)
#define HistEncodeSchemeSet(_name, _scheme)

/* ---------------------------------------------------------------------- */
#endif  /* HIST_DEBUG                                                     */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#ifdef  HIST_ENCODE_CHECK
#define HISTOGORAM_DECODE_CHECK
#include "Histogram-Decode.cpp"
#define HIST_ENCODE_CHECK_statement(_statement) _statement
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define hist_check(_hist, _bs64, _ebits, _m_cbins)
#define HIST_ENCODE_CHECK_statement(_statement) _statement
/* ---------------------------------------------------------------------- */
#endif   /* HIST_ENCODE_CHECK                                             */
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#if HIST_ENCODE_BTE
/* ---------------------------------------------------------------------- */
static void hist_encode_bte (BitStream64 &bs, Histogram &hist, int r0);
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define     hist_encode_bte(_bs, _hist, _r0)
/* ---------------------------------------------------------------------- */
#endif /* HIST_ENCODE_BTE                                                 */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Encodes a channels histogram
 *
 *  \param[in:out] bs64  The encoding bit stream
 *  \param[in    ] hist  The histogram to encode
 *
 *  \par
 *   Because any formal encoding incurs too much  overhead for such a
 *   small number of values, this is somewhat of an ad hoc scheme.
 *
 *   The strategy is to take advantage of the fact that the histogram
 *   falls off exponentially. Thus the number of significant bits,
 *   essentially to the log base 2, of a bin entries is somewhat linear.
 *
 *   The contents of a bin is deconstructed into two pieces
 *      -# The number of significant bits
 *      -# The significant bits
 *
 *   Because the number of significant bits decays linearly, the difference
 *   between the the number of significant bits of adjacent bits is encoded.
 *   If the fall off was perfectly linear, the difference would all be 0, but
 *   noise and statistics being what they are, they are variations.
 *
 *   The scheme chosen to encode this is with 3 bit masks for each group of 32
 *   entries.
 *         -# Mask 0 is a bit mask of all non-zero entries
 *         -# Mask 1 is a bit mask of all entries that decrease by 1 bit
 *         -# Mask 2 is a bit mask of all entries that increase by 1 bit
 *
 *   One now takes advantage of the fact these are some the number of bits
 *   need in subsequent masks decreases; only those entries that are non-zero
 *   can contribute to the Mask 1 and only those entries that are non-zero
 *   and not +1 can contribute to Mask 2.
 *
 *   Bits that are not 0, +1 or -1 are represented by the 0s in Mask 2.  These
 *   entries are encoded as is using however many bits needed to encode the
 *   remaining entries.
 *
 *   EXAMPLE
 *   Consider a histogram with 8 entries with a total of 255 entries
 *
 *   ENTRY            0     1     2     3     4     5     6     7
 *   HISTOGRAM :   0x70, 0x41, 0x40, 0x02,  0x5,   02,  0x5,  0x0
 *   CUMULATIVE:         0x70, 0xb1, 0xf1, 0xf3, 0xf8, 0xfa, 0xff
 *   LEFT          0xff, 0x8f, 0x4e,  0xe,  0xc,  0x7,  0x5, 0x0
 *   NBITS            7,    6,    5,    2,    3,    1,    1,    0
 *   DIFF             -    +1    +1    +3    -1    +2,   +1    +1
 *
 *   Mask non 0s      1     1     1     1     1     1     1     0 -> 0x7e (8 bits)
 *   Mask of  1s      0     1     1     0     0     0     1     - -> 0x31 (7 bits)
 *   Mask of -1s      0     -     -     0     1     0     -     - -> 0x2  (4 bits)
 *   Mask of rest     1                 1           1
 *
 *   Note that the Mask of the rest is just to 0s of the -1s, so is implied.
 *
 *   One can see that subsequent bit masks are limited to only those entries that
 *   have not been previously accounted for.  So this suggests that the masks be
 *   arranged in order of the most to least populated.  This strategy decimates
 *   the mask as quickly as possible.
 *
 *   For real distributions Mask 0 of the first 32-bits is fairly randomly populated,
 *   so no compressed version is generally possible. However as you get to the tail
 *   of the distributions, there are many 0 entries and the mask beccomes sparsely
 *   populated, suggesting a binary tree encoding.
 *
 *   The encoding of the significant bits is more straight-forward. Taking advantage
 *   that the first set bit is implied, for
 *      -#  the entries that differ by only +1/-1 just encode the last n-1 significant bits
 *      -#  the entries that differ by more are encoded using a number of bits set
 *          by the remaining entries, on the fact the number of entries can not be greater
 *          than this number.
 *
 *          In the above example, entries 0, 3 and 4 fall into this category.
 *          At entry 0, there are 0xff entries left, so need 8 bits to encode 0x70.
 *          At entry 3, there are  0xe entries left, so need 4 bits to encode 0x2
 *          At entry 4, there are  0xc entries left, so need 4 bits to enocde 0x5
 */
void Histogram::encode (BitStream64 &bs64, Table table[NBins+1]) const
{

   #pragma HLS INLINE off

   static int Version (0);


   // DEBUGGING ONLY STREAM
   HIST_ENCODE_CHECK_statement (BitStream64 CheckStream; CheckStream.setName ("Hist"); )


   // Debug only
   bool debug = true;


   Histogram::Acc_t left = PACKET_K_NSAMPLES -1;
   HISTOGRAM_statement (if (debug) print ());

   // This is for debug purposes only
   HistEncodeSchemeDeclare (encoding_scheme);

   // -----------------------------------------------------------------------------
   // Format the header
   //
   // Note this could fail if the nbits > 32.
   // This seems unlikely.  That would require a histogram of 2**15 bins
   //
   // Format version = 0  (2 bits)
   //           last > 1  (Size in bits of lastgt1)
   //           last > 2  (No more than the actual bits in last > 1
   //
   // Note::
   // Because of the countLeadingMethod is not marked as const, the
   // values lastgt1 and lastgt2 must be copied to non-const variables.
   // -----------------------------------------------------------------------------
   Histogram::Idx_t lastgt1 = m_clastgt1;
   Histogram::Idx_t lastgt2 = m_clastgt2;
   int             nlastgt2 = lastgt1.length () - lastgt1.countLeadingZeros ();
   int                nbits = lastgt1.length () + nlastgt2;
   uint32_t            bits = (Version << nbits)
                            | ((uint32_t)lastgt1  << nlastgt2)
                            | ((uint32_t)lastgt2);
   nbits        += 2;
   int          diff;

   // ----------------------------------------------------------------
   // This insert of these bits into the output stream is deferred 
   // and done on entry to the HIST_ENCODE_BIN_LOOP.  This unnatural
   // construction saves dropping down another copy of the bs64.insert
   // which is a big savings in FPGA resources.
   // ----------------------------------------------------------------
   //bs64.insert (tmp, nbits);
   // ----------------------------------------------------------------

   
   // ----------------------------------------------------------------
   // DEPRECATED: This was part of the binary tree encoding of the
   // groups of 32-bins containing only 0's and 1's. The current
   // implementation of the encoding routines was too resource
   // expensive, so this gain in packing efficiency has been 
   // sacrificed until a better implementation is found.  
   //
   // Also note, that when the number of histogram bins is 64,
   // there are almost no benefit, i.e. there are no groups of
   // such bins.
   // ------------------------------------------------------------
   // Find last group of 32 bits that contains a count value > 1
   // All bins past this point are either 0 or 1.
   // The premise is that bins in past this are sparsely populated
   // and amiable to a binary tree compression on the bit mask.
   // ------------------------------------------------------------
   ///int r0 = lastgt1 | 0x1f;
   // ------------------------------------------------------------
   
   int      ibin;
   int prv_nbits;
   int total = 0;

   // ---------------------------------------------------------------------------------
   // This loop divides the histogram into the following regions
   // Region  Range                               Encoding Method
   // ------  ----------------------------------  ------------------------------------
   //      0  0 - last bin with a count gt 3;     Differences in the significant bits
   //      1  last bin > 3 + 1 - last bin < 2     Absolute, 2 bits (values of 0-3)
   //      2  last_bin > 2 + 1 - next 32 boundary Absolute, 1 bit  (values of 0,1)
   //      3  remaining                           Must be 0 or 1,masks encoded
   //                                             as binary tree
   // ---------------------------------------------------------------------------------
   HIST_ENCODE_BIN_LOOP:
   for (ibin = 0; ;ibin++)
   {
      #pragma HLS LOOP_TRIPCOUNT min=40 max=64 avg=60
      #pragma HLS PIPELINE II=2
      
      if (nbits)
      {
         bs64.insert (bits, nbits);
         HIST_ENCODE_CHECK_statement (CheckStream.insert (bits, nbits);)
      }

      if (ibin > m_clastgt0)
      {
         break;
      }

      Histogram::Entry_t cnts = m_comask.test (ibin)
                              ? m_cbins[ibin]
                              : Histogram::Entry_t (0);

      bits        = cnts;
      table[ibin] = total;
      total      += cnts;

      // For the bins up to and including the last bin of 2 significant bits
      // of more, encode using the differences
      if (ibin <= lastgt2)
      {
         HistEncodeSchemeSet (encoding_scheme, HES_K_DIFF);

         // Count the number of significant bits in this bin
         nbits = cnts.length() - cnts.countLeadingZeros ();

         // --------------------------------------------------------
         // Guess that bin[0] (overflow bin) has 2 significant bits
         //            bin[1] (0 count  bin) has 7 significant bits
         // --------------------------------------------------------
         if      (ibin == 0) prv_nbits = 2;
         else if (ibin == 1) prv_nbits = 7;

         // Count the difference in significant bits from the last entry
         int diff  = prv_nbits - nbits;
         prv_nbits = nbits;

         // ------------------------------------------------------------
         // The following is a poor man's Huffman type encoding.  This
         // is just a prefix code of n 0s.
         //
         //   Diff # 0s
         //      0    0. The leading 1 forms the stop bit
         //              and the first bit of the bin count
         //      1    1  There is one less significant bit than the
         //              previous count
         //     -1    2  There is one more significant bit than the
         //              previous count
         //   rest    3  The number of digits follows. This is no
         //              more than what is need for the remaining
         //              count.  For example, if there are less than
         //              127 values left, 6 digits are used.
         // ------------------------------------------------------------
         {
            if (diff == 0)
            {
               // Only need to handle special case of 0
               if (cnts == 0) { nbits = 1; bits = 1; }
            }
            else if (diff == 1)
            {
               if (cnts == 0) { nbits += 2;  bits = 1; }
               else           { nbits += 1;            }
            }
            else if (diff == -1)
            {
               nbits += 2;
            }
            else if (diff == 2)
            {
               if (cnts == 0) { nbits += 4; bits =1; }
               else           { nbits += 3;          }
            }
            else if (diff == -2)
            {
               nbits += 4;
            }
            else
            {
               // Handle 0 case specially
               // This is necessary to ensure there is a stop bit
               if (nbits == 0)
               {
                  nbits = 6;
                  bits  = 1;
               }
               else
               {
                  // Encode as an absolute value using the maximum number
                  // of bits that will contain it
                  nbits = 7 + left.length () - left.countLeadingZeros ();
               }
            }
         }
      }

      // All values are <= 3, just encode the absolute counts
      else if (ibin <= lastgt1)
      {
         HistEncodeSchemeSet (encoding_scheme, HES_K_FX2);
         nbits = 2;
      }

      else
      {
         HistEncodeSchemeSet (encoding_scheme, HES_K_FX1);
         nbits = 1;
      }

      #if 0
      // This is if doing a binary tree encode of those past r0
      // I've abandoned this idea for the present time.  It looks
      // easier and with only a small penalty to shorten the range
      // such that there aren't many stragglers.
      //  Shard to the 32-bit boundary, all values are 0 or 1
      else if (ibin <= r0)
      {
         HistEncodeSchemeSet (encoding_scheme, HES_K_FX1);
         nbits = 1;
      }

      // -----------------------------------------------------
      // Only 0s or 1s are left; these are taken care of with
      // the binary tree encoding.
      // ----------------------------------------------------
      else
      {
         nbits = 0;
      }
      #endif

      if (nbits)
      {
         hist_encode_print (debug, encoding_scheme,        ibin, diff,
                            nbits, bits, bs64.m_cur, bs64.m_idx, left);
      }

      left -= cnts;
   }

   // Last bin
   table[ibin] = total;

   // If defined, binary tree encode the remaining bins
   hist_encode_bte (bs64, this, r0);

   hist_encode_summary_print (debug, nbits, left);


   // DEBUG
   #if HIST_ENCODE_CHECK
   {
      int ebits = CheckStream.flush ();
      hist_check (m_id, CheckStream, ebits, m_bins);  ///!!!  m_cbins);
   }
   #endif

   return;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- */
#if HIST_ENCODE_BTE
/* ---------------------------------------------------------------------- */

static inline int bte_size (uint32_t w);

/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief Does a binary tree encoding, of the final groups of 32 bins
 *         that consist solely of 0's and 1's
 *         
 *  \param[in] bs    The output bit stream
 *  \param[in] hist  The histogram being encoded
 *  \param[in] r0    The last bin encode, so r0 + 1 = first bin to be
 *                   encoded.
 *                   
/* ---------------------------------------------------------------------- *//*!

static inline void hist_encode_bte (BitStream64 &bs, Histogram &hist, int r0)
 {
 // --------------------------------------------------------
 // Encode the bit masks of the region containing 0s and 1s
 // Stop at last bin (module 32) with > 0 cnts.
 // The encoding is via a binary tree, encoding the masks
 // 32-bits at a time.
 //
 // For each mask of 32 bits words, the output is
 //     Bit   0:  0:mask = 0, 1:mask!=0
 //     Bit 1-2:  Binary tree Encoding scheme
 //     Bits >2:  The serially encoded binary tree
 //
 // NOTE: Bit 0 = most significant bit
 // -------------------------------------------------------
 HIST_ENCODE_BTE_LOOP:
 for (ibin = r0+1; ibin <= hist.m_lastgt0; ibin += 32)
 {
    #pragma HLS PIPELINE
    #pragma HLS LOOP_TRIPCOUNT min=0 max=4 avg=2
    int       end = ibin + 31;
    uint32_t mask = hist.m_omask (end, ibin);
    uint32_t bits;

    if (mask != 0)
    {
       uint32_t pattern = BTE_wordPrepare (mask);
       //int schemeSize   = BTE_wordSize    (mask, pattern);
       ///int scheme       = BTE_scheme      (schemeSize);
       ///nbits            = BTE_size        (schemeSize);

       ///bs64.insert (pattern, nbits);

       nbits = 30;
       bits  = pattern;

       // !!! KLUDGE !!! -- BTE should be fixed
       // BTE returns left justified field, bs64 inserts right justified,
       // so must right justify
       ///bits             = BTE_wordEncode  (mask, pattern,schemeSize) >> (32 - nbits);
       ///bs64.insert ((1 << 2) | scheme, 3);
    }
    else
    {
       nbits = 1;
       bits  = 0;
    }

    /////bs64.insert (bits, nbits);
    hist_encode_print (debug, HES_K_BT, ibin, 0,
                       nbits, bits, bs64.m_cur, bs64.m_idx, left);
    return;
}
/* ---------------------------------------------------------------------- */

 
 
/* ---------------------------------------------------------------------- */ 
static inline int bte_size (uint32_t w)
{
   int size;
   if (w)
   {
      uint32_t pattern = BTE_wordPrepare (w);
      size = BTE_size (BTE_wordSize (w, pattern));
   }
   else
   {
      size = 0;
   }

   return size;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define bte_size(_w) 0
/* ---------------------------------------------------------------------- */
#endif  /* HIST_ENCODE_BTE                                                */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
#if HIST_ENCODE_BTE && HIST_ENCODE_DEBUG
/* ---------------------------------------------------------------------- *//*!
 * 
 *   \brief  Prints the occupancy masks
 *   
 *   \param[in]  title  A title caption
 *   \param[in]     m0  Mask 0
 *   \param[in]     m1  Mask 1
 *   \param[in]     m2  Mask 2
 *   \param[in]     m3  Mask 3
 *   
\* ---------------------------------------------------------------------- */
static void print_bte_masks (char const *title,
                             uint32_t       m0,
                              uint32_t      m1,
                              uint32_t      m2,
                              uint32_t      m3)
{
    int size0 = bte_size (m0);
    int size1 = bte_size (m1);
    int size2 = bte_size (m2);
    int size3 = bte_size (m3);

    std::cout << std::setw (8) << title
              <<  ' ' << std::setw(8) << std::setfill ('0') << m0
              << " (" << std::hex << std::setw (2) 
                      << std::setfill (' ') << size0 << ')'
              
              <<  ' ' << std::setw(8) << std::setfill ('0') << m1
              << " (" << std::hex << std::setw (2) 
                      << std::setfill (' ') << size1 << ')'
              
              <<  ' ' << std::setw(8) << std::setfill ('0') << m2 
              << " (" << std::hex << std::setw (2) 
                      << std::setfill (' ') << size2 << ')'
              
              <<  ' ' << std::setw(8) << std::setfill ('0') << m3 
              << " (" << std::hex << std::setw (2) 
                      << std::setfill (' ') << size3 << ')'
                      
              << std::endl;

    return;
}
/* ---------------------------------------------------------------------- */

#else
/* ---------------------------------------------------------------------- */
#define print_bte_masks(_title, _masks)
/* ---------------------------------------------------------------------- */
#endif  /* HIST_ENCODE_BTE && HIST_ENCODE_DEBUG                           */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#ifdef HIST_ENCODE_DEBUG
/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Prints the occupancy masks
 *
 *   \param[in]  title  A title caption
 *   \param[in]      m  The array of masks to print]
 *
\* ---------------------------------------------------------------------- */
static void print_masks (char const                       *title,
                         ap_uint<32> const m[Histogram::NGroups])
{
   std::cout << std::setw (8) << title;

   for (int group = 0; group < Histogram::NGroups; group++)
   {
      uint32_t mask = m[group];
      int size = bte_size (mask);
      std::cout << ' '  << std::hex << std::setw (8) << std::setfill ('0') << mask
                << " (" << std::hex << std::setw (2) << std::setfill (' ') << size << ')';
   }

   std::cout << std::endl;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief    For each histogram entry, print the compressed bits and the
 *             current contents of the compressed bit stream
 *
 *  \param[in]  debug  Print only if in debug mode
 *  \param[in] scheme  The scheme used to encode this bin
 *  \param[in]   ibin  Which bin of the histogram
 *  \param[in]   diff  The difference in the number of significant bits
 *                     in the bin contents (the count) from the previous
 *                     bin.
 *  \param[in]  nbits  The number of bits to stuff into the output bit
 *                     stream.
 *  \param[in]   bits  The bits to stuff into the output bit stream
 *  \param[in]    cur  The current value of the 64-bit buffer that will
 *                     eventually be committed to the output bit stream.
 *  \param[in]    idx  The current bit index
 *  \param[in]   left  The number of counts left to committed.  When this
 *                     number reaches 0, all non-zero histogram bins have
 *                     been committed.
 \* ---------------------------------------------------------------------- */
static void hist_encode_print (bool              debug,
                               HistEncodeScheme scheme,
                               int                ibin,
                               int                diff,
                               int               nbits,
                               uint32_t           bits,
                               uint64_t            cur,
                               int32_t             idx,
                               int                left)
{
   static char const Scheme[4][5] = { "DIFF", "FX2 ", "FX1 ", "BT  " };

   if (!debug) return;

   std::cout << std::setw (4) << Scheme[scheme] << ' '
             << std::setw ( 5) << std::setfill (' ') << std::hex <<  ibin << ' ';

   // Check if in the binary tree encoding
   if (scheme < HES_K_BT)
   {
      // No, regular encoding
      std::cout << std::setw ( 3) << std::setfill (' ') << std::dec <<  diff << ' ';
   }
   else
   {
      // Binary tree encoding, the diff is irrelevant
      std::cout << "     ";
   }

   std::cout << std::setw ( 3) << std::setfill (' ') << std::dec << nbits << ' '
             << std::setw ( 5) << std::setfill (' ') << std::hex <<  bits << ' '
             << std::setw (16) << std::setfill (' ') << std::hex <<   cur << ' '
             << std::setw ( 5) << std::setfill (' ') << std::dec <<   idx << ' '
             << std::setw ( 5) << std::setfill (' ') << std::dec <<  left
             << std::endl;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Prints the summary of the histogram encoding
 *
 *   \param[in  debug  Print only if in debug mode
 *   \param[in] nbits  The number of bits used to encode the histogram
 *   \param[in] left   The number of counts left. These are the counts
 *                     in the region of the binary tree encoding. (These
 *                     bins are not counted/deducted during the encoding
 *                     process. It is too expensive and serves no real
 *                     purpose.
 */
static void hist_encode_summary_print (bool debug, int nbits, int left)
{
   if (!debug) return;
   std::cout << "Encoded bit count:" << std::dec << nbits
            << " Left:" << left << std::endl;
   return;
}
/* ---------------------------------------------------------------------- */
#endif  /* HIST_ENCODE_DEBUG                                              */
/* ---------------------------------------------------------------------- */
