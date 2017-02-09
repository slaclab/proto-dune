//-----------------------------------------------------------------------------
// File          : TimingDtmSystem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
// TimingDtmSystem Top Device
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

#ifndef __TIMING_DTM_SYSTEM_H__
#define __TIMING_DTM_SYSTEM_H__

#include <System.h>
#include <stdint.h>
using namespace std;

class CommLink;
class MultDest;

class TimingDtmSystem : public System {

      uint32_t idx_;
      MultDest *dest_[2];

   public:

      //! Constructor
      TimingDtmSystem (string defaults, uint32_t idx);

      //! Deconstructor
      ~TimingDtmSystem ( );

      void softReset ( );
      void hardReset ( );
      void setRunState ( string state );
      void command ( string name, string arg );

};
#endif

