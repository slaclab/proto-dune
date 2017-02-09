//-----------------------------------------------------------------------------
// File          : TimingDtm.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    Timing DTM Registers
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

#ifndef __TIMING_DTM_H__
#define __TIMING_DTM_H__

#include <Device.h>
#include <stdint.h>

using namespace std;

//! Class to contain TimingDtm
class TimingDtm : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param index       Device index
       * \param parent      Parent device
      */
      TimingDtm ( uint32_t linkConfig, uint32_t index, Device *parent );

      //! Deconstructor
      ~TimingDtm ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      //! Hard Reset
      void hardReset ();

      //! Soft Reset
      void softReset ();

      //! Count Reset
      void countReset ();

      //! Method to read status registers and update variables
      void readStatus ( );

      //! Method to read configuration registers and update variables
      /*!
       * Throws string error.
       */
      void readConfig ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      void writeConfig ( bool force );

      //! Verify hardware state of configuration
      void verifyConfig ( );
};

#endif
