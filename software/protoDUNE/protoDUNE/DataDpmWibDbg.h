//-----------------------------------------------------------------------------
// File          : DataDpmWibDbg.h
// Author        : Matt Graham  <mgraham@slac.stanford.edu>
// Created       : 11/05/2015
// Project       : 
//-----------------------------------------------------------------------------
// Description :  Container for Waveform Extraction constants
//
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
// 11/05/2015: created
//-----------------------------------------------------------------------------
#ifndef __DATA_DPM_WIB_DBG_H__
#define __DATA_DPM_WIB_DBG_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

class DataDpmWibDbg:public Device {        
 public:
    DataDpmWibDbg ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1 );
    ~DataDpmWibDbg( ); 
};

#endif
