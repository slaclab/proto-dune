// -*-Mode: C++;-*-


#ifndef _DUNE_DATA_COMPRESSION_TYPES_H_
#define _DUNE_DATA_COMPRESSION_TYPES_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionTypes.h
 *  @brief    Interface file for the Dune data compression
 *  @verbatim
 *                               Copyright 2013
 *                                    by
 *
 *                       The Board of Trustees of the
 *                    Leland Stanford Junior University.
 *                           All rights reserved.
 *
 *  @endverbatim
 *
 *  @par Facility:
 *  DUNE
 *
 *  @author
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  2015/12/02>
 *
 *  @par Last commit:
 *  \$Date: $ by \$Author: $.
 *
 *  @par Revision number:
 *  \$Revision: $
 *
 *  @par Location in repository:
 *  \$HeadURL: $
 *
 * @par Credits:
 * SLAC
 *
\* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\

   HISTORY
   -------

   DATE       WHO WHAT
   ---------- --- ---------------------------------------------------------
   2018.04.18 jjr Diced this up into smaller, more modular pieces
                  - Basic #defines parameter definitions -> Parameters.h
                  - Axis definitions    -> Axis.h
                  - Config definitions  -> Config.h
                  - Monitor definitions -> Monitor.h

   2018.03.06 jjr Changed the symbolic definitions of the Axis control
                  bits from masks to bit numbers.
   2016.05.03 jjr Added output limiting register to the configuration
   2015.11.28 jjr Added error counters for Start of Frame, End of Frame on
                  other than the expected words, i.e. first word for SOF
                  and last for EOF

   2015.11.18 jjr Added error counters for no Start of Frame, End of Frame
                  and K28_5 character not seen to the read status block

                  Redefined the bits in user first and user last so that aren't
                  overloaded. Bits 0 & 2 are now only set in user last and
                  bits 1 & 3 are not only set in user first.  This will
                  allow us to resynchronize the frames if need be.

                  To accomodate this the user field had to be expanded to
                  4 bits

   2016.10.22 jjr Change Axi streams to hls::streams.  Change data types
                  to unsigned 64-bit data.
   2016.10.20 jjr Change ModuleConfig init and mode to 32-bit integers
                  just to make it easier to manipulation from the
                  processor.
   2016.10.12 jjr Added Common Status to the status block.  This will
                  count statistics common to all channels
   2016.01.05 jjr Added the status word to the raw frame. Previously this
                  was absent, meaning that this simulation did not match
                  the frames as delivered by the FPGA code.
   2015.12.02 jjr Made public

\* ---------------------------------------------------------------------- */


//////////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Data compression'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Data compression', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////


#include "Parameters.h"
#include "Axis.h"
#include "Config.h"
#include "Monitor.h"
#include "Frame.h"

#include "ap_int.h"
#include "WibFrame.h"
#include <stdint.h>



/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef ModuleIdx_t
 *  \brief   An integer sized to contain the module indices.
 *
 *  \par
 *   The incoming channels are processed in groups of channels called
 *   modules. The number of modules on an RCE is RCE_K_NMODULES, with
 *   RCE_B_NMODULES giving the number of bits in an integer needed
 *   to specify a module index.
 *
 *  \note
 *   Must guard against having only 1 module. In this case RCE_B_NMODULES
 *   will be 0, and, without the check, an ap_uint<0> will try to be
 *   instantiated; clearly a non-sensical idea.
 *
\* ---------------------------------------------------------------------- */
#if RCE_B_NMODULES > 0
typedef ap_uint<RCE_B_NMODULES> ModuleIdx_t;
#else
typedef ap_uint<1> ModuleIdx_t;
#endif
/* ---------------------------------------------------------------------- */


#define ADC_B_NBITS 12

/* ---------------------------------------------------------------------- *//*!
 *
 * \typedef Adcin_t
 * \brief   Typedef for an input ADC word
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint<ADC_B_NBITS>  AdcIn_t;
/* ---------------------------------------------------------------------- */



#if 0
/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef WibTimestamp_t
 *  \brief   The timestamp information in the WIB frame
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint<64> WibTimestamp_t;
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef WibCrate_t
 *  \brief   The WIB crate number
 *
 *  \typedef WibSlot_t
 *  \brief   The WIB slot number
 *
 *  \typedef WibFiber_t
 *  \brief   The WIB fiber number
 *
 *  \typedef WibId_t
 *  \brief   The WIB fiber slot and crate #
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint< 5> WibCrate_t;
typedef ap_uint< 3> WibSlot_t;
typedef ap_uint< 3> WibFiber_t;
typedef ap_uint<13> WibId_t;
/* ---------------------------------------------------------------------- */
#endif

#endif

