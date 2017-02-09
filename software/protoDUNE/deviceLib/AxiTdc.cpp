//-----------------------------------------------------------------------------
// File          : AxiTdc.cpp
// Author        : Kurtis Nishimura <kurtisn@slac.stanford.edu>
// Created       : 11/03/2014
// Project       : General DAQ
//-----------------------------------------------------------------------------
// Description :
// Device container for AxiTdc.vhd
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
// 11/03/2014: created
//-----------------------------------------------------------------------------
#include <AxiTdc.h>
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
AxiTdc::AxiTdc ( uint linkConfig, uint baseAddress, uint index, Device *parent, uint addrSize, uint nChannels) : 
                        Device(linkConfig,baseAddress,"AxiTdc",index,parent) {

   RegisterLink *rl;
   Command      *c;
   Variable     *v;
   stringstream tmp;

   // Description
   desc_ = "FPGA TDC object.";

   // Create Registers: name, address
   addRegister(new Register("TdcCalTrigger",   baseAddress_ + 0x00*addrSize));

   addRegisterLink(rl = new RegisterLink("CalDelay", baseAddress_ + 0x01*addrSize, Variable::Configuration));
   rl->getVariable()->setDescription("Calibration Delay");
   rl->getVariable()->setComp(0,1,0,"");

   for (unsigned int i = 0; i < nChannels; ++i) {
      tmp.str(""); 
      tmp << "tdcChannel_"  << dec << setw(2) << setfill('0') << i;
      addRegisterLink(rl = new RegisterLink(tmp.str(), baseAddress_ + (0x02+i)*addrSize, Variable::Status));
      rl->getVariable()->setDescription(tmp.str());
   }

   addCommand(c = new Command("TdcCalTrigger"));
   c->setDescription("Trigger Calibration Start/Stop Signals to TDC");

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

}

// Deconstructor
AxiTdc::~AxiTdc ( ) { }

// Process Commands
void AxiTdc::command(string name, string arg) {
   Register *r;
   if (name == "TdcCalTrigger") {
      REGISTER_LOCK
      r = getRegister("TdcCalTrigger");
      r->set(0x0);
      writeRegister(r, true, false);
      REGISTER_UNLOCK
   }
}

