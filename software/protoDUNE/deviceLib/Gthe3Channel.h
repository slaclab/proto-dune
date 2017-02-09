//-----------------------------------------------------------------------------
// File          : Gthe3Channel.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 02/16/2016
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device driver for Gthe3Channel
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 02/16/2016: created
//-----------------------------------------------------------------------------
#ifndef __GTHE3_CHANNEL_H__
#define __GTHE3_CHANNEL_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain Gthe3Channel
class Gthe3Channel : public Device {

   public:
      Gthe3Channel ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1 );
      ~Gthe3Channel ( );
    
};

#endif
