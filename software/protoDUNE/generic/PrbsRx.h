//-----------------------------------------------------------------------------
// File          : PrbsRx.h
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
#ifndef __PRBS_RX_H__
#define __PRBS_RX_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain AD9252
class PrbsRx : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      PrbsRx ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize );

      //! Deconstructor
      ~PrbsRx ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      // Count Reset
      void countReset();

};

#endif
