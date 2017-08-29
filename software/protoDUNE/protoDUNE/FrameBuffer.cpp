//-----------------------------------------------------------------------------
// File          : FrameBuffer.cpp
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 09/18/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    Class to contain and track a microslice buffer.
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
//
//       DATE WHO WHAT 
// ---------- --- ------------------------------------------------------------
// 2017.07.11 jjr Moved many methods to be inlines in the .h files
// 2016.11.05 jjr Added receive frame sequence number
//
// 09/18/2014: created
//-----------------------------------------------------------------------------
#include "FrameBuffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Constructor
FrameBuffer::FrameBuffer () {
   _index  = -1;
   _data   = NULL;
   _size   = 0;
}

// Destructor
FrameBuffer::~FrameBuffer () {
}

