//-----------------------------------------------------------------------------
// File          : DataCompressionSystem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
// DataCompressionSystem Top Device
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
#ifndef __DATA_COMPRESSION_SYSTEM__H__
#define __DATA_COMPRESSION_SYSTEM__H__

#include <System.h>
#include <stdint.h>
using namespace std;

class CommLink;
class MultDest;

class DataCompressionSystem : public System {

      uint32_t idx_;
      MultDest *dest_;

   public:

      //! Constructor
      DataCompressionSystem (string defaults, uint32_t idx );

      //! Deconstructor
      ~DataCompressionSystem ( );

      //! Method to process a command
      void command ( string name, string arg );
};
#endif
