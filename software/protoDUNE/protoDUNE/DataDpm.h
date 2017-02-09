//-----------------------------------------------------------------------------
// File          : DataDpm.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    LBNE Data DPM Registers
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
// 06/19/2014: created
//-----------------------------------------------------------------------------
#ifndef __DATA_DPM_H__
#define __DATA_DPM_H__

#include <Device.h>
#include <stdint.h>

using namespace std;

//! Class to contain DataDpm
class DataDpm : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param index       Device index
       * \param parent      Parent device
      */
      DataDpm ( uint32_t linkConfig, uint32_t index, Device *parent );

      //! Deconstructor
      ~DataDpm ( );

};

#endif
