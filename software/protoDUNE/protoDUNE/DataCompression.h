//-----------------------------------------------------------------------------
// File          : DataCompression.h
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
//
// DATE       WHO WHAT
// ---------- --- -------------------------------------------------------------
// 2016.11.19 jjr Added enableHLSmodule method
//
// 11/05/2015: created
//-----------------------------------------------------------------------------
#ifndef __DATA_COMPRESSION_DEVICE_H__
#define __DATA_COMPRESSION_DEVICE_H__

#include <Device.h>
#include <stdint.h>
using namespace std;

class DataCompression:public Device {        
 public:
    DataCompression ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1 );
    ~DataCompression( ); 


    void command ( string name, string arg );  
    void hardReset ();
    void softReset ();
    void countReset ();    
    void readStatus ( );
    void readConfig ( );      
    void writeConfig ( bool force );
    void verifyConfig ( );

    class StatusRegisters
    {
    public:
       StatusRegisters (Device       *device,
                        uint32_t baseAddress,
                        uint32_t    addrSize);

    public:
       enum Enum
       {
          Ca_Pattern    =  0,
          Cfg_Mode      =  1,
          Cfg_NCfgs     =  2,

          Rd_Status     =  3,
          Rd_NFrames    =  4,
          Rd_NNormal    =  5,
          Rd_NDisabled  =  6,
          Rd_NFlush     =  7,
          Rd_NDisFlush  =  8,
          Rd_NErrSofM   =  9,
          Rd_NErrSofU   = 10,
          Rd_NErrEofM   = 11,
          Rd_NErrEofU   = 12,
          Rd_NErrEofE   = 13,
          Rd_NErrK28_5  = 14,
          Rd_NErrSeq    = 15,

          Wr_NWrote     = 16,
          StatusCnt     = 17
       };

    private:
       RegisterLink *rl_[StatusCnt];
    };

private:
    void enableHLSmodule (uint32_t addrSize);
    StatusRegisters sr_;
};

#endif
