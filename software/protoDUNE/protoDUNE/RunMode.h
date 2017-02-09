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
//-----------------------------------------------------------------------------
// Modification history :
// 09/18/2014: created
//-----------------------------------------------------------------------------
#ifndef __RUN_MODE_H__
#define __RUN_MODE_H__

enum RunMode { 
   IDLE  = 0, 
   SCOPE = 1, 
   BURST = 2, 
   TRIG  = 3 };

#endif


