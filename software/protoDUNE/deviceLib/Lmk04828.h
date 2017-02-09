//-----------------------------------------------------------------------------
// File          : Lmk04828.h
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 27/04/2015
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device Driver for Lmk04828
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
// 27/04/2015: created
//-----------------------------------------------------------------------------
#ifndef __LMK_H__
#define __LMK_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

//! Class to contain Lmk04828
class Lmk04828 : public Device {
   
      //! Device configuration address range constants
      #define START_ADDR 0x100     
      #define END_ADDR   0x17D
      
   public:

      
      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      Lmk04828 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1 );

      //! Deconstructor
      ~Lmk04828 ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
       void command ( string name, string arg );

      //! Synchronise internal counters
       void SyncClks ();

      // //! Powerdown the sysref lines.
      // void syarefOn ();

    
};

#endif
