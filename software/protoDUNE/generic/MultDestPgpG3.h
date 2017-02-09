//-----------------------------------------------------------------------------
// File          : MultDestPgpG3.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Destination container for MultLink class.
//
// Destination Field usage:
// bits 7:0 = Index (ignored)
// bits 11:8 =  PGP VC for register transactions
// bits 15:12 = PGP Lane for register transactions
// bits 19:16 = PGP VC for commands
// bits 23:20 = PGP Lane for commands
// bits 27:24 = PGP VC for data
// bits 31:28 = PGP Lane for data
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
// 06/18/2014: created
//-----------------------------------------------------------------------------
#ifndef __MULT_DEST_PGP_G3_H__
#define __MULT_DEST_PGP_G3_H__
#include <MultDest.h>
#include <string>
#include <stdint.h>
using namespace std;

//! Class to contain PGP communications link
class MultDestPgpG3 : public MultDest {
      // Register Map
      struct PgpCardReg *reg_;

      string path_;

   public:

      //! Constructor
      MultDestPgpG3 (string path);

      //! Deconstructor
      ~MultDestPgpG3 ( );

      // The remaining functions are called only from the MultLink class.

      //! Open link
      void open ( uint32_t idx, uint32_t maxRxTx );

      //! Transmit data
      int32_t transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config );

      // Receive data
      int32_t receive ( MultType *type, void **ptr, uint32_t *context);
      // Evr Link Status
      bool getEvrStatus();

      // Evr Link Errors
      uint32_t getEvrErrors();

      // Evr Link Count
      uint32_t getEvrCount(uint32_t idx);
      uint32_t getEvrStatRaw();

      // set/Get EVR Enble
      bool getEvrEnable();
      void setEvrEnable(bool enable);

      // set/Get EVR Enble mask
      uint32_t getEvrEnableLane();
      void setEvrEnableLane(uint32_t mask);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneRunOpCode(uint32_t lane);
      void setEvrLaneRunOpCode(uint32_t lane, uint32_t code);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneAcceptOpCode(uint32_t lane);
      void setEvrLaneAcceptOpCode(uint32_t lane, uint32_t code);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneRunDelay(uint32_t lane);
      void setEvrLaneRunDelay(uint32_t lane, uint32_t delay);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneAcceptDelay(uint32_t lane);
      void setEvrLaneAcceptDelay(uint32_t lane, uint32_t delay);
      
      //! Transmit Op-Code
      int32_t sendOpCode ( uint32_t opCode );      

};
#endif
