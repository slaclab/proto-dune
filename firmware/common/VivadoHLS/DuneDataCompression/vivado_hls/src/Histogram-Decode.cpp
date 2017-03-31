#include "DuneDataCompressionHistogram.h"
#include "BitStream64.h"

#if !defined (HIST_DECODE_CHECK) && !defined (__SYNTHESIS__)
#define HIST_DECODE_CHECK 0
#endif



#if HIST_DECODE_CHECK
static int hist_compare (Histogram::Entry_t const hbins[Histogram::NBins],
                         uint16_t           const dbins[Histogram::NBins],
                         int                                        ebits,
                         int                                        dbits);

static int hist_decode  (uint16_t                  bins[Histogram::NBins],
                         BitStream64                                &bs64,
                         int                                        tbits);

static int get_nbits   (int                                 nsignificant,
                        int                                         nbits,
                        int                                         left);


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Checks that the encoded histogram matches the orginal
 *   \retval A flag indicating that the check was a failure or not
 *
 *   \param[in]  ihist Which histogram
 *   \param[in]   bs64 The encoded bit stream
 *   \param[in]  ebits The number of bits it took to encode the
 *                     histogram
 *   \parampin]  ebins The original histogram bins that were encoded
 *
\* ---------------------------------------------------------------------- */
static bool hist_check (int                                        ihist,
                        BitStream64                                &bs64,
                        int                                        ebits,
                        Histogram::Entry_t const ebins[Histogram::NBins])

 {
   // ------------------------------------
   // Check the integrity of the encoding
   // -----------------------------------
   uint16_t dbins[Histogram::NBins];
   int      dbits = hist_decode  (dbins, bs64, ebits);
   int      ecnt  = hist_compare (ebins, dbins, ebits, dbits);

   std::cout << "Histogram[" << std::hex << std::setw(4) << ihist
             << "] encode/decode " << (ecnt ? "failed" : "succeeded");

   if (ecnt == 0) std::cout << " encoded bits:" << std::hex << ebits
                            << ':' << std::dec << ebits;

   std::cout  << std::endl;

   return ecnt != 0;
 }
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Compares the encoded and decoded histogram.
 *  \return Count of the number of mismatches.
 *
 *  \par
 *   A mismatch occurs if
 *     -# The bin contents differ
 *     -# The number of decoded bits does not match the number of
 *        encoded bits
\* ---------------------------------------------------------------------- */
static int hist_compare (Histogram::Entry_t const hbins[Histogram::NBins],
                         uint16_t           const dbins[Histogram::NBins],
                         int                                       ebits,
                         int                                       dbits)
{
   int ecnt;

   // -----------------------------------------------------
   // Check that the encoded bit count == decoded bit count
   // -----------------------------------------------------
   if (ebits != dbits)
   {
      ecnt = 1;
      std::cout << "Encoded:Decoded bit count mismatch " << std::hex
                << ebits << "!=" << dbits << std::endl;
   }
   else
   {
      ecnt = 0;
   }

   int prv = hbins[0];
   // ----------------------------
   // Check each bin for equality
   // ---------------------------
   for (int idx = 0; idx < Histogram::NBins; idx++)
   {
      int expected = hbins[idx];
      int got      = dbins[idx];



      if (expected != got)
      {
         ecnt += 1;
         std::cout << std::hex << std::setw(3) << idx << ' '
                   << std::setw (4) << std::hex
                   << expected << "!=" << got << std::endl;
      }

      prv = hbins[idx + 1];

      // If have hit the maximum, that's it
      if (prv == 1023) break;
   }


   return ecnt;
}
/* ---------------------------------------------------------------------- */



static void print_hist (bool debug, int idx, int cnts, int left)
{
   if (!debug) return;
   if ((idx & 0x1f) == 0) printf ("Bin[%2.2x]", idx);
   printf (" %3.3x(%3d)", cnts, left);

   if ((idx & 0x1f) == 0x1f) putchar ('\n');
}


#include "BFU.h"
#include "BTD.h"
int hist_decode (uint16_t bins[Histogram::NBins], BitStream64 &bs64, int tbits)
{
   bool debug = true;

   int64_t  m01s;
   uint64_t buf[Histogram::NBins * 10/64 + 10];
   int idx = 0;
   int left = 1023;

   while (1)
   {
      uint64_t dat;
      bool okay = bs64.m_out.read_nb (buf[idx]);
      if (!okay) break;
      idx += 1;
   }

   BfuPosition_t position = 0;
   BFU bfu;

   _bfu_put (bfu, buf[0], 0);


   int nbits;
   int nsignificant = 2;
   int cnts;

   int version  = _bfu_extractR(bfu, buf, position, 2);
   int lastgt1  = _bfu_extractR(bfu, buf, position, 6);
   nbits        = 32 - __builtin_clz (lastgt1);
   int lastgt2  = _bfu_extractR(bfu, buf, position, nbits);

   bfu          =  BFU__ffs     (buf, _bfu_get_tmp (bfu), position);
   nbits        = _bfu_get_pos  (bfu) - position;
   nsignificant =  get_nbits    (2, nbits, left);
   position     = _bfu_get_pos  (bfu);

   // Specical coding when cnts == 0
   if (nsignificant == 0 || nbits == 5)
   {
      cnts  = 0;
      int tmp = _bfu_extractR (bfu, buf, position, 1);
      nsignificant = 0;
   }
   else
   {
      if (nbits == 6)
      {
         // The value is encoded as an absolute value.
         // This means that the number of significant bits must
         // be computed from the actual value.
         nsignificant = 32 -__builtin_clz (cnts);
      }
      cnts  = _bfu_extractR (bfu, buf, position, nsignificant);
   }

   print_hist (debug, 0, cnts, left);
   bins[0]      = cnts;
   left        -= cnts;

   nsignificant = 7;
   for (idx = 1; idx <= lastgt2;)
   {
      bfu          =  BFU__ffs     (buf, _bfu_get_tmp (bfu), position);
      nbits        = _bfu_get_pos  (bfu) - position;
      nsignificant =  get_nbits    (nsignificant, nbits, left);
      position     = _bfu_get_pos  (bfu);

      // Special coding when cnts == 0
      if (nsignificant == 0 || nbits == 5)
      {
         cnts         = 0;
         int tmp      = _bfu_extractR (bfu, buf, position, 1);
         nsignificant = 0;
      }
      else
      {
         if (nbits == 6)
         {
            // The value is encoded as an absolute value.
            // This means that the number of significant bits must
            // be computed from the actual value.
            nsignificant = 32 -__builtin_clz (cnts);
         }
         cnts  = _bfu_extractR (bfu, buf, position, nsignificant);
      }

      print_hist (debug, idx, cnts, left);
      bins[idx]    = cnts;
      left        -= cnts;
      idx         +=    1;
      if (left == 0)
      {
         goto CLEANUP;
      }
   }


   while (idx <= lastgt1)
   {
     cnts      = _bfu_extractR (bfu, buf, position, 2);
     print_hist (debug, idx, cnts, left);
     bins[idx] = cnts;
     left     -= cnts;
     idx      +=    1;
     if (left == 0)
     {
        goto CLEANUP;
     }
   }

   // The remaining entries are either 0 or 1
   // This loop could be done a lot better
   while (1)
   {
      cnts = _bfu_extractBR (bfu, buf, position);
      print_hist (debug, idx, cnts, left);
      bins[idx] = cnts;
      left     -= cnts;
      idx      += 1;
      if (left == 0)
      {
         goto CLEANUP;
      }
   }

#if 0
   // Get the number of entries between present and the next 32-bit boundary
   nbits = ~lastgt1 & 0x1f;
   m01s  = _bfu_extractL(bfu, buf, position, nbits);
   while (--nbits >= 0)
   {
      cnts = m01s < 0 ? 1 : 0;
      print_hist (debug, idx, cnts, left);
      bins[idx] = cnts;
      left     -= cnts;
      idx      += 1;
      if (left == 0)
      {
         if (nbits)
         {
            // Have consumed too many bits, put'em back
            position -= nbits;
            _bfu_put (bfu, buf[bfu_index (position)], position);
         }
         goto CLEANUP;
      }
      m01s <<= 1;
   }


   // Extract the bins that are all 0s or 1s */
   while (idx < Histogram::NBins - 1)
   {
      int        end = idx + 32;
      BfuWord_t flag = _bfu_extractBoolean (bfu, buf, position);
      if (flag)
      {
         int scheme = _bfu_extractR (bfu, buf, position, 2);
         int mbits;
         bfu = BFU__word (buf, bfu.cur, position);
         uint32_t w = BTD_wordDecode  (bfu.val,
                                       scheme,
                                       &nbits);
         position += nbits;
         _bfu_put (bfu, buf[bfu_index (position)], position);
         do
         {
            cnts = bins[idx] = w & 1;
            print_hist (debug, idx, cnts, left);
            idx  += 1;
            left -= cnts;
            if (left == 0)
            {
               goto CLEANUP;
            }
            w   >>= 1;
         }
         while (idx < end);
      }
      else
      {
         // All zeros
         do
         {
            bins[idx] = 0;
            print_hist (debug, idx, 0, left);
         }
         while (++idx < end);
      }
   }
#endif

   // Zero any remaining bins
   CLEANUP:
   while (idx < Histogram::NBins)
   {
      bins[idx] = 0;
      print_hist (debug, idx, 0, left);
      idx += 1;
   }

   EXIT:
   return position;
}

static int get_nbits (int nsignificant, int nbits, int left)
{
   if (nbits == 0)
   {
      // No change
   }
   else if (nbits == 1)
   {
      nsignificant -= 1;
   }
   else if (nbits == 2)
   {
      nsignificant += 1;
   }
   else if (nbits == 3)
   {
      nsignificant -= 2;
   }
   else if (nbits == 4)
   {
      nsignificant += 2;
   }
   else if (nbits == 5)
   {
      nsignificant = 1;
   }
   else
   {
      // Any bits past 7 belong with the value
      // Since the value is absolutely encoded, one cannot depend
      // on the leading 1 to have stopped the scan.
      nsignificant =  (32 - __builtin_clz (left)) - (nbits - 7);
   }

   return nsignificant;
}
#else
static inline int hist_compare (Histogram::Entry_t const hbins[Histogram::NBins],
                                uint16_t           const dbins[Histogram::NBins],
                                int                                        ebits,
                                int                                        dbits)
{
   return 0;
}
#endif
