//-----------------------------------------------------------------------------
// File          : AxiAds42lb69.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 04/18/2014
// Project       : 
//-----------------------------------------------------------------------------
// Description :
// DAQ Device Driver for the LTC2270 ADC IC
//
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
// 04/18/2014: created
//-----------------------------------------------------------------------------

#ifndef __AXI_ADS42LB69_H__
#define __AXI_ADS42LB69_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain AxiAds42lb69
class AxiAds42lb69 : public Device {

   public:

      AxiAds42lb69 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=4 );
      ~AxiAds42lb69 ( );

      void command ( string name, string arg ); 
      void hardReset ();
      void softReset ();
      void countReset ();       
      
      void readStatus ( );
      void readConfig ( );      
      void writeConfig ( bool force ); 
      void verifyConfig ( );  
};

#endif
