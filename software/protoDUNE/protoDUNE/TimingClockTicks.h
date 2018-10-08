#ifndef __TIMING_CLOCK_TICKS_H__
#define __TIMING_CLOCK_TICKS_H__


///////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the
// top-level directory of this distribution and at:
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
// No part of 'DUNE Development Software', including this file,
// may be copied, modified, propagated, or distributed except according to
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////



// ----------------------------------------------------------------------
//
// HISTORY
//
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2018.10.03 jjr Changed from_usecs to return a 64-bit value allowing
//                one to convert a period > 32 bits (i.e. ~4.2 seconds)
// 2018.07.24 jjr Separated from DaqBuffer.h
// ----------------------------------------------------------------------


#include <stdint.h>


/* ---------------------------------------------------------------------- *//*!

   \class  TimingClockTicks
   \brief  Captures the parameters of and methods for the timing system
                                                                          */
/* ---------------------------------------------------------------------- */
class TimingClockTicks
{
public:
   /* ------------------------------------------------------------------- *//*!

     \enum  Constants
     \brief The constants associated with the timing system
                                                                          */
   /* ------------------------------------------------------------------- */
   enum Constants
   {
      CLOCK_PERIOD    = 20, /*!< Number of nanoseconds per clock tick     */
      PER_SAMPLE      = 25, /* Number of clock ticks between ADC samples  */
      SAMPLE_PERIOD   = CLOCK_PERIOD * PER_SAMPLE,
                            /*!< Number of nanoseconds between ADC samples*/
      PER_FRAME       = PER_SAMPLE * 1024,
                            /*!< Elapsed time, in ticks, in a 1024 packet */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

      \brief  Convert the period in micro seconds to period in clock ticks.
      \return The period in clock ticks.

      \param[in] period  The period, in usecs, to convert
                                                                          */
   /* ------------------------------------------------------------------- */
   constexpr static inline uint64_t from_usecs (uint32_t period)
   {
      return (1000 * static_cast<uint64_t>(period) + CLOCK_PERIOD/2) 
            / CLOCK_PERIOD;
   }
   /* ------------------------------------------------------------------- */
};

#endif
