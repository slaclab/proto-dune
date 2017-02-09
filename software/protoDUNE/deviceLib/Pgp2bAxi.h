//-----------------------------------------------------------------------------
// File          : Pgp2bAxi.h
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/12/2013
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    DAQ Device Driver for the Pgp2bAxi.vhd
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
// 11/12/2013: created
//-----------------------------------------------------------------------------
#ifndef __PGP2B_REG_H__
#define __PGP2B_REG_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain Pgp2bAxi
class Pgp2bAxi : public Device {

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      Pgp2bAxi ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize = 1 );

      //! Deconstructor
      ~Pgp2bAxi ( );

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

      //! Dump debug
      void dumpDebug ();
};

#endif
