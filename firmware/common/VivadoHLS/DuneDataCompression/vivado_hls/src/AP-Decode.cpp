// -*-Mode: C;-*-

/* ---------------------------------------------------------------------- *//*!

   \file  AP-Decode.c
   \brief Arithmetic Decoder, implementation file
   \author JJRussell - russell@slac.stanford.edu

   \par Overview
    Implementation of the routines to decode bit streams using an
    arithmetic probability encoding technique. To successfully decode
    an input stream, the encoder and decoder must agree on the modeling
    table. This modeling table is generally constructed from a frequency
    distribution derived from the actual data to be encoded or from
    a distribution thought to be highly representative (in a statisical
    sense345) of the distribution to be encoded. This frequency distribution
    must then be transformed into the encoding/decoding modeling table.

   \n
   \par
    To achieve performance goals, particularly on the encoding side, these
    routines must be used with a model builder that forces a particular
    normalization of the cumulative probability. While such tables may
    be constructed in a variety of ways, APM_build is an example of such
    a routine and is anticipated to be the model table builder.

   \n
   \par Table Details
    The frequency table used in this is implemented with 32-bit resolution
   (technically, really 31, because the inclusive space [0,1] is mapped
    on to 0x00000000 - 0x80000000). This not only gives a high level of
    precision, but, more importantly, allows the frequency table to
    capture a larger dynamic range. From a simplistic viewpoint, the
    number of bits of dynamic range sets a floor on the maximum
    achievable compression.

   \n
   \note
    This file acts as a setup for the real code that is contained in the
    file acdtemplate.h. It merely defines some preprocessor symbols so
    that the template file can generate both the APD and APD32 routines
    on both big and little endian machines.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *\
 *
 * HISTORY
 * -------
 *
 * DATE       WHO WHAT
 * ---------- --- ---------------------------------------------------------
 * 2016.05.19 jjr Adapted for dune usage
 *
\* ---------------------------------------------------------------------- */




#include "AP-Decode.h"
#define ENDIANNESS_IS_LITTLE 1


#define APD_M_IOBUF_ALIGN     (sizeof (APD_iobuf_t) - 1)
#define APD_K_IOBUF_BITS      (1<<APD_K_IOBUF_SHIFT)
#define APD_M_IOBUF_MASK     ((1<<APD_K_IOBUF_SHIFT) - 1)




/* ---------------------------------------------------------------------- *//*!

  \fn   void APD_start (APD_dtx            *dtx,
                        void  const        *src,
                        unsigned int       boff)
  \brief Begins a decoding session

  \param  dtx  The decoding context to be initialized
  \param  src  The encoded source/input bit stream
  \param boff  The bit offset into the input bit stream

  \par
   This routine initializes a decoding context for byte-oriented encoded
   streams. See APD32_start for accessing 32-bit byte-swapped streams.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn     int APD_bdecompress (unsigned char       *dst,
                               int                  cnt,
                               void const          *src,
                               unsigned int        boff,
                               APD_table_t const *table)
  \brief  Convenience routine to decode a bit stream using the specified
          table into a byte array
  \return The number of bits that were decoded.

  \param   dst  The destination/output buffer
  \param   cnt  The number of bytes available in the output buffer
  \param   src  The encoded source/input buffer
  \param  boff  The bit offset to start at in the input buffer
  \param table  The decoding table

  \par
   This is a convenience routine, combining APD_start, APD_decode and
   APD_finish. This routine can only be used if the decoding table is
   the same for all symbols in the input stream.

  \par
   The input stream to be decoded must be big-endian byte. See
   APD32_bdecompress for decoding 32-bit byte-swapped streams.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn      unsigned int  APD_decode     (APD_dtx             *dtx,
                                         APD_table_t const *table)
  \brief   Decodes the next symbol
  \return  The decoded symbol

  \param   dtx   The decoding context
  \param table   The table to use in the decoding

  \par
   The input stream to be decoded must be big-endian byte. See
   APD32_bdecompress for decoding 32-bit byte-swapped streams.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn     int APD_finish (APD_dtx *dtx)
  \brief  Finishes the decoding, cleaning up any inprogress context
  \return The number of bits decoded

  \par
   This routine indicates the user is finish decoding a byte oriented
   stream. See APD32_finish for finishing a 32-bit byte-swapped stream.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
/* APD32, decoding 32-bit byte-swapped streams                            */
/* ---------------------------------------------------------------------- *//*!

  \fn   void APD32_start (APD_dtx      *dtx,
                          void const   *src,
                          unsigned int boff)
  \brief Begins a decoding session

  \param  dtx  The decoding context to be initialized
  \param  src  The encoded source/input bit stream
  \param boff  The bit offset into the input bit stream

  \par
   This routine initializes a decoding context for 32-bit byte-swapped
   streams. See APD32_start for accessing byte oriented streams.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn     int APD32_bdecompress (uint8_t              *dst,
                                 int                   cnt,
                                 void const           *src,
                                 unsigned int         boff,
                                 APDC_table_t const *table)
  \brief  Convenience routine to decode a bit stream using the specified
          table
  \return The number of bits that were decoded.

  \param   dst  The destination/output buffer
  \param   cnt  The number of bytes available in the output buffer
  \param   src  The encode source/input stream
  \param  boff  The bit offset to start at in the input buffer
  \param table  The decoding table

  \par
   This is a convenience routine, combining APD_start, APD_decode and
   APD_finish. This routine can only be used if the decoding table is
   the same for all symbols in the input stream.

  \par
   The input stream to be decoded must be 32-bit byte-swapped. See
   APD32_bdecompress for decoding byte oriented streams.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn      unsigned int  APD32_decode (APD_dtx            *dtx,
                                       APD_table_t const *table)
  \brief   Decodes the next symbol
  \return  The decoded symbol

  \param   dtx   The decoding context
  \param table   The table to use in the decoding

  \par
   The input stream to be decoded must be 32-bit byte-swapped. See
   APD32_bdecompress for decoding byte oriented streams.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn     int APD32_finish (APD_dtx *dtx)
  \brief  Finishes the decoding, cleaning up any inprogress context
  \return The number of bits decoded

  \par
   This routine indicates the user is finish decoding a 32-bit byte-swapped
   stream. See APD32_finish for finishing a byte stream.
                                                                          */
/* ---------------------------------------------------------------------- */









/* ---------------------------------------------------------------------- *//*!

  \static __inline unsigned int scale_m1 (uint32_t num,
                                          uint32_t den)
  \brief  Does a scaling equivalent to  ((\a num << 32) - 1) / \a den.
  \return The scaled value

  \param  num  The numerator/value to scale
  \param  den  The denominator/scaling factor

  \par
   The scaling is of equivalent to  ((\a num << 32) - 1) / \a den.
   If \a den is 0, this really a sentinal value indicating that
   it's value is really 1. Due to a finite number of bits, 1 must
   be represented as 0.
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline APD_cv_t scale_m1 (APD_cv_t num,
                                   APD_cv_t den)
{
  uint32_t q;

  //num64 = (num << 32) - 1;

  q  = ((num << APC_K_NORM_NBITS) - 1)/ den;

  // printf ("%16.16llx << 32 - 1 / %8.8x = %8.8x\n", n, den, a);

  return q;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn    static __inline int lookup (uint32_t             cum,
                                     APD_table_t const *table,
                                     unsigned int         cnt)
  \brief  Lookups the interval containing the specified cumulative
          probability using a binary search.
  \return The index of the interval

  \par      cum  The target cumulative probability
  \par    table  The table of cumulative probabilities
  \par      cnt  The number of intervals in the table

  \par
   This lookup uses a binary search to locate the interval. It has one
   twist for efficiency. Since the 0 symbol occurs with a high
   probability it is check for as a special case.
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline int lookup (APD_cv_t             cum,
                            APD_table_t const *table,
                            unsigned int         cnt)
{
    unsigned int      ilo;
    unsigned int      ihi;
    unsigned int        i;


    /*
     | Checking the 0 entry first, it is the most probable.
     | The additional check for 1 is if the table has only 1 entry.
     | In the case of only 1 entry table[1] is the top entry, which
     | technically corresponds to 1, but since 1 cannot be explicitly
     | represented in the table, it is replaced by the sentinal value of 0.
     | Thus the need for the cnt == 1 test.
     */
    if (cum < table[1] || cnt == 1) return 0;

    ilo = 1;
    ihi = cnt;


    /*
     | Notice in this loop, the top most entry of the table is never
     | referenced. Doing so would be wrong, since this is the funny 0 entry.
    */
    while (1)
    {
        i = (ihi + ilo) / 2;
        /*
          printf ("cum = %8.8x p[%3d:%3d:%3d] = %8.8x:%8.8x:%8.8x\n",
          cum,           ilo,  i,ihi,   table[ilo],table[i],table[ihi]);
        */
        if (cum >= table[i]) ilo = i;
        else                 ihi = i;
        if (ihi - ilo <= 1 ) break;
    }

    return ilo;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \fn    static __inline int lookup_top (APD_cv_t             cum,
                                         APD_table_t const *table,
                                         unsigned int         cnt)
  \brief  Lookups the interval containing the specified cumulative
          probability by doing a linear search from the top of the table
          to the bottom
  \return The index of the interval

  \par      cum  The target cumulative probability
  \par    table  The table of cumulative probabilities
  \par      cnt  The number of intervals in the table

  \par
   The table is search from the top to bottom using a linear search. This
   is likely the least efficient search, since many tables have a tendency
   to arrange their symbols in order of occurance from the bottom to the
   top. It is kept  around because it is very simple and is used as a check
   against more sophisticated searches.
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline int lookup_top (APD_cv_t             cum,
                                APD_table_t const *table,
                                unsigned int         cnt)
{
    APD_table_t const  *p;
    int            symbol;

    /* Find the symbol */
    p     = table + cnt;
    while (1)
    {
        //printf (" %x <= %x %x > %x\n", p[-1], cum, p[-1]*range, cum*range);
        if (*--p <= cum) break;
    }

    symbol = p - table;
    //printf ("Symbol = %d\n", p - table);

    return symbol;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \fn    static __inline int lookup_bot (uint32_t          cum,
                                         uint32_t const *table,
                                         unsigned int      cnt)
  \brief  Lookups the interval containing the specified cumulative
          probability by doing a linear search from the bottom of the table
          to the top
  \return The index of the interval

  \par      cum  The target cumulative probability
  \par    table  The table of cumulative probabilities
  \par      cnt  The number of intervals in the table

  \par
   The table is search from the bottom to top using a linear search. For
   small tables, this may be the most efficient way to search, avoiding
   the overhead of more sophisiticated methods
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline int lookup_bot (APD_cv_t             cum,
                                APD_table_t const *table,
                                unsigned int        cnt)
{
    int symbol;

    // Check if in the top bin
    if (table[cnt] <= cum)
    {
        symbol = cnt;
    }
    else
    {
        APD_table_t const *p = table + 1;
        while (1)
        {
            //printf (" %x >= %x\n", *p, cum);
            if (*p > cum) break;
            p++;
        }

        symbol = p - table - 1;
        //printf ("Symbol = %d\n", symbol);
    }

    return symbol;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
/* Define the non-swapped version                                         */
/* ---------------------------------------------------------------------- */

/* Set the stream access */

#if APD_K_IOBUF_SHIFT==6
#    define  load(_in)       *(uint64_t *)(_in)
#else
#    error APD_K_IOBUF is not 6 (64-bit access)
#endif



/* Define the routine interface names */
#define   apd_start       APD_start
#define   apd_decode      APD_decode
#define   apd_bdecompress APD_bdecompress
#define   apd_finish      APD_finish
#include "apdtemplate.h"

/* ---------------------------------------------------------------------- */


