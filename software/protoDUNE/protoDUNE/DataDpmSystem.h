//-----------------------------------------------------------------------------
// File          : DataDpmSystem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
// DataDpmSystem Top Device
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
#ifndef __DATA_DPM_SYSTEM_H__
#define __DATA_DPM_SYSTEM_H__

#include <System.h>
#include <stdint.h>
using namespace std;

class CommLink;
class MultDest;
class DataBuffer;

class DataDpmSystem : public System {

      uint32_t idx_;
      MultDest *dest_[3];

      uint32_t lastErrors_;
      uint32_t buffState_; // 0 = ok, 1 = warn, 2 = err

   public:

      //! Constructor
      DataDpmSystem (string defaults, uint32_t idx );

      //! Deconstructor
      ~DataDpmSystem ( );

      //! Method to perform soft reset
      void softReset ( );

      //! Method to perform hard reset
      void hardReset ( );

      //! Method to set run state
      void setRunState ( string state );

      //! Method to process a command
      void command ( string name, string arg );

      //! Return local state, specific to each implementation
      string localState();

private:
      DataBuffer *dataBuffer ();
};
#endif
