//-----------------------------------------------------------------------------
// File          : AxiDac7654.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 09/05/2013
// Project       : Generic 
//-----------------------------------------------------------------------------
// Description :
// DAQ Device Driver for the DAC7654 IC
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
// 09/04/2013: created
//-----------------------------------------------------------------------------

#ifndef __AXI_DAC7654_H__
#define __AXI_DAC7654_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain SPI Flash
class AxiDac7654 : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      AxiDac7654 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1 );

      //! Deconstructor
      ~AxiDac7654 ( );

      //! Method to process a command
      /*!
       * Returns status string if locally processed. Otherwise
       * an empty string is returned.
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );      

      //! Method to read status registers and update variables
      /*! 
       * Throws string on error.
      */
      void readStatus ( );
      
      //! Method to read configuration registers and update variables
      /*! 
       * Throws string on error.
      */
      void readConfig ( );      

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      void writeConfig ( bool force );
           
};

#endif
