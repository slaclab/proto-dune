// -*-Mode: C;-*-

#ifndef AP_DECODE_H
#define AP_DECODE_H


/* ---------------------------------------------------------------------- *//*!

   \file  dde/AP-Decode.h
   \brief Arithmetic Word Decoder interface file
   \author JJRussell - russell@slac.stanford.edu

   \verbatim  CVS $Id: APD.h,v 1.2 2006/01/24 00:20:23 russell Exp $ \endverbatim

   \par Overview
    Interface specification for routines to decode streams using
    an arithmetic probability encoding technique.  It is based on a
    32-bit table giving the probabilities of the encoded symbols.

   \par More Info
    For details on the implementation and design choices see the
    documentation in APD.c

   \par APD vs APD32 Interface
    The APD routines decode a byte stream of encoded symbols, while the
    APD32 routines decode a 32-bit stream of encoded symbols.

   \note
    The encoding side of these routines writes the encoded data as a big
    endian byte-stream, where the \e big endian refers to the fact that
    the bits within the bytes are serially accessed starting at the most
    significant bit to the least significant bit.

   \par APD32, Typical Usage
    One application of the APD32 routines is when the input stream that the
    encoded symbols are embedded in is treated as a 32-bit big endian
    stream. Typically what one will do is byte-swap this stream on little
    endian machines, so that the 32-bit words are in the correct order.
    Once this is done, one should use APD32_ routines to correctly access
    the input stream on either big or little-endian machines.

   \n
   \note
    Given that the APE encoding routines write a big-endian oriented
    byte-stream, the APD and APD32 routines are exactly the same on
    big-endian machines. As an implementation note, these routines
    truly are the same on big-endian  machines, with the equivalent names
    being aliased.

   \n
   \note
    This more than a convenience to the user. One might argue that the user
    could just undo the byte-swapping, but this is not  true in all cases.
    Specifically, if the user does not know the length of the encoded symbols
    (\e e.g. the encoded stream itself contains the end-of-stream information,
    he will be unable to determine how many words to byte-swap.
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


/*
 * This must match PACKET_B_NSAMPLES, but since that definition resides in
 * FPGA Vivado world, really don't wish to import it to the offline world
 */
#ifdef  APC_K_NBITS

  #if APC_K_NBITS != 12
  #error "APC_K_NBITS previously defined, but != 12, the only legitimate value"
  #endif

#else
  #define APC_K_NBITS   (10+2)
#endif

#include "AP-Common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// For the host side decoding
typedef uint16_t      APD_table_t;
typedef uint16_t      APD_cv_t;
typedef uint16_t      APD_range_t;
typedef uint32_t      APD_scaled_t;





/* ---------------------------------------------------------------------- *//*!

  \def     APD_iobuf
  \brief   The input/output buffer type
                                                                          */
/* ---------------------------------------------------------------------- */
#  define APD_iobuf_t   uint64_t
#  define APD_K_IOBUF_SHIFT    6
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \struct _APD_dtx
  \brief   Decoding context
                                                                          *//*!
  \typedef APD_dtx
  \brief   Typedef for struct \e _APD_dtx

   While this is defined in the public interface, this structure should
   be treated like a C++ private member. All manipulation of this
   structure should be through the APD or APD32 routines.
                                                                          */
/* ---------------------------------------------------------------------- */
typedef struct _APD_dtx
{
  APD_cv_t              lo;  /*!< Current lo limit                        */
  APD_cv_t              hi;  /*!< Current hi limit                        */
  APD_cv_t           value;  /*!< Current value                           */
  APD_iobuf_t       buffer;  /*!< Input staging buffer                    */
  unsigned int        togo;  /*!< Number of bits in the staging buffer    */
  uint8_t const       *beg;  /*!< Original input buffer address           */
  uint8_t const       *cur;  /*!< Current  input buffer address           */
  unsigned int        bbeg;  /*!< Beginnng bit offset                     */
}
APD_dtx;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

    \typedef APD_table_t
   \brief   The type for the decoding table
                                                                          */
/* ---------------------------------------------------------------------- */
typedef uint16_t APD_table_t;
/* ---------------------------------------------------------------------- */


static __inline APD_cv_t scale_hi (APD_cv_t        lo,
                                   APD_range_t  range,
                                   APD_table_t  value)
{

   APD_cv_t val;

   // -------------------------------------------------------
   //
   // The code has been arranged such that 2**N - 1 samples
   // are compressed.  This means that the cumulative
   // probability saturates at 2**N -1 so only N bits, not
   // N + 1 bits are needed in the APC_table_t.  This scaling
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
      APD_scaled_t xlo = lo - 1;
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



static __inline APD_cv_t scale_lo (APD_cv_t       lo,
                                   APD_range_t range,
                                   APD_table_t value)
{
   // ---------------------------------------------------
   // Need to scale lo up by APC_K_NORM_NBITS
   // This allows the calculation of the returned
   // value to be mapped to a DSP multiply and accumulate
   // ---------------------------------------------------
   APD_scaled_t xlo = lo;
   xlo <<= APC_K_NORM_NBITS;

   // -------------------------------------------------------
   // This is the straightforward calculation but cannot map
   // to a*b + c as does the following form.
   // ------------------------------------------------------
   //////APC_cv_t val = ((range * value) >> APC_K_NORM_NBITS) + lo;

   APD_cv_t val = ((range * value + xlo) >> APC_K_NORM_NBITS);


   //std::cout << "scale_lo lo:" << lo << " range:" << range << " value:" << value
   //          << " -> " << val  << std::endl;

   return val;
}


#if APD_DUMP
#define APD_dumpStatement(_statement) _statement
#else
#define APD_dumpStatement(_statement)
#endif


/* ---------------------------------------------------------------------- */

extern void           APD_start         (APD_dtx              *dtx,
                                         const void           *src,
                                         unsigned int         boff);

extern unsigned int   APD_decode        (APD_dtx              *dtx,
                                         APD_table_t const  *table);

extern int            APD_bdecompress   (uint8_t              *dst,
                                         int                   cnt,
                                         const void           *src,
                                         unsigned int         boff,
                                         APD_table_t const  *table);

extern int            APD_finish        (APD_dtx              *dtx);



extern void           APD32_start       (APD_dtx              *dtx,
                                         const void           *src,
                                         unsigned int         boff);

extern unsigned int   APD32_decode      (APD_dtx              *dtx,
                                         APD_table_t const  *table);

extern int            APD32_bdecompress (uint8_t              *dst,
                                         int                   cnt,
                                         const void           *src,
                                         unsigned int         boff,
                                         APD_table_t const  *table);

extern int            APD32_finish      (APD_dtx             *dtx);

/* ---------------------------------------------------------------------- */


#ifdef __cplusplus
}
#endif


#endif
