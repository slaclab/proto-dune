//-----------------------------------------------------------------------------
// File          : PrbsRx.cpp
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/14/2013
// Project       : HPS SVT
//-----------------------------------------------------------------------------
// Description :
// Device container for AxiVersion.vhd
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
// 11/14/2013: created
//-----------------------------------------------------------------------------
#include <PrbsTx.h>
#include <Register.h>
#include <RegisterLink.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
PrbsTx::PrbsTx ( uint linkConfig, uint baseAddress, uint index, Device *parent, uint addrSize ) : 
                        Device(linkConfig,baseAddress,"PrbsTx",index,parent) {

   RegisterLink *l;

   // Description
   desc_ = "Firmware Version object.";

   // Create Registers: name, address
   addRegisterLink(new RegisterLink("ConfigStatus",   baseAddress_ + 0x00*addrSize, 1, 4,
                                    "AxiEn", Variable::Configuration, 0, 0x1,
                                    "TxEn", Variable::Configuration, 1, 0x1,
                                    "Busy", Variable::Status, 2, 0x1,
                                    "Overflow", Variable::Status, 3, 0x1));
   
   addRegisterLink(new RegisterLink("PacketLength",    baseAddress_ + 0x01*addrSize, Variable::Configuration));
   
   addRegisterLink(new RegisterLink("AxiStreamCfg", baseAddress_ + 0x02*addrSize, 1, 2,
                                    "tDest", Variable::Configuration, 0, 0xFF,
                                    "tId", Variable::Configuration, 8, 0xFF));
   
   addRegisterLink(l = new RegisterLink("DataCount",  baseAddress_ + 0x03*addrSize, Variable::Status));
   l->setPollEnable(true);
   
   addRegisterLink(l = new RegisterLink("EventCount",  baseAddress_ + 0x04*addrSize, Variable::Status));
   l->setPollEnable(true);
   
   addRegisterLink(new RegisterLink("RandomData",   baseAddress_ + 0x05*addrSize, Variable::Status));


   // Variables
   Variable* v;

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   getVariable("AxiEn")->setTrueFalse();
   //   getVariable("AxiEn")->setPerInstance(true);   
   getVariable("TxEn")->setTrueFalse();
   getVariable("TxEn")->setPerInstance(true);

}

// Deconstructor
PrbsTx::~PrbsTx ( ) { }


