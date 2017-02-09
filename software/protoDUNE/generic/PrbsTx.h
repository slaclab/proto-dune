//-----------------------------------------------------------------------------
// File          : PrbsTx.h
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/12/2013
// Project       : Heavy Photon Search SVT DAQ
//-----------------------------------------------------------------------------
// Description :
// ADS1115 ADC
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
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __PRBS_TX_H__
#define __PRBS_TX_H__

#include <Device.h>
using namespace std;

//! Class to contain AD9252
class PrbsTx : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
   PrbsTx ( uint linkConfig, uint baseAddress, uint index, Device *parent, uint addrSize );

   //! Deconstructor
   ~PrbsTx ( );


};

#endif
