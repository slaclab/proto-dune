//-----------------------------------------------------------------------------
// File          : DaqMuxV2.h
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 7/19/2016
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device Driver for DAQ Multiplexer Version 2
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
//-----------------------------------------------------------------------------
#ifndef __DAQ_MUX_COMMON_H__
#define __DAQ_MUX_COMMON_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain DaqMuxV2
class DaqMuxV2 : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      DaqMuxV2 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1 );

      //! Deconstructor
      ~DaqMuxV2 ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      //! Command functions
      void trigDaq ();
      void trigArm ();
      void freeze  ();    
};

#endif
