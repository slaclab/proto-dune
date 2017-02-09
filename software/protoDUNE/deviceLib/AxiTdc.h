//-----------------------------------------------------------------------------
// File          : AxiTdc.h
// Author        : Kurtis Nishimura <kurtisn@slac.stanford.edu>
// Created       : 11/3/2014
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    DAQ Device Driver for AxiTdc.vhd
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
// 11/03/2014: created
//-----------------------------------------------------------------------------
#ifndef __AXI_TDC_H__
#define __AXI_TDC_H__

#include <Device.h>
using namespace std;

//! Class to contain AxiTdc
class AxiTdc : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      AxiTdc ( uint linkConfig, uint baseAddress, uint index, Device *parent, uint addrSize=1, uint nChannels = 1);

      //! Deconstructor
      ~AxiTdc ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      //! Method to read status registers and update variables
      //void readStatus ( );

      //! Method to read configuration registers and update variables
      /*!
       * Throws string error.
       */
      //void readConfig ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      //void writeConfig ( bool force );

    
};

#endif
