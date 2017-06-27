//-----------------------------------------------------------------------------
// File          : RunMode.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 09/18/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
//
// HISTORY
//
//      DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2017.06.19 jjr New definition of RunMode replaces the 35-ton version
// 2014.09.18     Created
//-----------------------------------------------------------------------
#ifndef __RUN_MODE_H__
#define __RUN_MODE_H__


/* ---------------------------------------------------------------------- *//*!

  \enum  class RunMode
  \brief Describes the current triggering 

  \par
   The term RunMode is historical and not quite an accurate description.
   It has conflated concepts, whether 
       -# the system is currently running
       -# the triggering mode
       -# the style of data being taken.

   Eventually this should be made to clearer
                                                                          */
/* ---------------------------------------------------------------------- */
enum class RunMode 
{ 
   IDLE     = 0,   /*!< System is not being triggered                     */
   EXTERNAL = 1,   /*!< System is using an external trigger               */
   SOFTWARE = 2,   /*!< System is using an internal software trigger      */
};
/* ---------------------------------------------------------------------- */


#endif


