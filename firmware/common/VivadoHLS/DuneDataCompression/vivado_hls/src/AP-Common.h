// -*-Mode: C++;-*-

#ifndef AP_COMMON_H
#define AP_COMMON_H


/* ---------------------------------------------------------------------- *//*!

   \file  APC.h
   \brief Arithmetic Probability Encoder/Decoder, common interface file
   \author JJRussell - russell@slac.stanford.edu

\verbatim
    CVS $Id: APC.h,v 1.3 2006/09/13 17:58:41 russell Exp $
\endverbatim
                                                                          */
/* ---------------------------------------------------------------------- */


#include <stdint.h>

#include "DuneDataCompressionTypes.h"



/*
 | Code value parameters
*/
#define APC_M_CV_ALL     ((unsigned int)((1LL << (APC_K_NBITS  )) - 1))
#define APC_M_CV_M1      ((unsigned int)( 1   << (APC_K_NBITS-1)) - 1)
#define APC_M_CV_TOP_BOT ((unsigned int)( 1   << (APC_K_NBITS-1) | 1))
#define APC_M_CV_TOP     ((unsigned int)( 1   << (APC_K_NBITS-1)))


#define APC_K_HI    (unsigned int)APC_M_CV_ALL /* Largest code value      */
#define APC_K_Q1    (unsigned int)(1 << (APC_K_NBITS - 2))
                                             /* Point after first quarter */
#define APC_K_HALF  (unsigned int)(2  *  APC_K_Q1)
                                             /* Point after first half    */
#define APC_K_Q3    (unsigned int)(3  *  APC_K_Q1)
                                             /* Point after third quarter */

/*
 | This sets the normalization. It cannot be greater than quarter the range
 | of the code value word. If it is more than half the range, there is a
 | danger that, due to rounding, the HI limit of the range will be less
 | than the LO limit.
 */
#define APC_K_NORM_NBITS (APC_K_NBITS - 2)
#define APC_K_NORM       (1 << (APC_K_NORM_NBITS))


#endif



