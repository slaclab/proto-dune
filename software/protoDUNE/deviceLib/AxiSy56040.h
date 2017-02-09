//-----------------------------------------------------------------------------
// File          : AxiMicronN25Q.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 03/19/2014
// Project       :  
//-----------------------------------------------------------------------------
// Description :
//    XBAR
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
// 03/19/2014: created
//-----------------------------------------------------------------------------

#ifndef __AXI_SY_56040_H__
#define __AXI_SY_56040_H__

#include <Device.h>

//! Class to contain generic register data.
class AxiSy56040 : public Device {
   public:

      //! Constructor
      AxiSy56040 (uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1);

      //! Deconstructor
      ~AxiSy56040 ( );

      void command(string name, string arg);
};
#endif
