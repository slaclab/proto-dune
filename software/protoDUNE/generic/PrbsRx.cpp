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
#include <PrbsRx.h>
#include <Register.h>
#include <RegisterLink.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <stdint.h>
using namespace std;

// Constructor
PrbsRx::PrbsRx ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"PrbsRx",index,parent) {

   RegisterLink *l;
   Variable     *v;
   Command      *c;

   // Description
   desc_ = "Firmware Version object.";

   cout << "PrbsRx: baseAddress: " << hex << baseAddress << endl;

   // Create Registers: name, address
   addRegisterLink(l = new RegisterLink("MissedPacketCount", baseAddress_ + 0x00*addrSize, Variable::Status));
   l->getVariable()->setDescription("Number of missed packets");
   l->setPollEnable(true);

   addRegisterLink(l = new RegisterLink("LengthErrorCount", baseAddress_ + 0x01*addrSize, Variable::Status));
   l->getVariable()->setDescription("Number of packets that were the wrong length");
   l->setPollEnable(true);

   addRegisterLink(l = new RegisterLink("EofeErrorCount", baseAddress_ + 0x02*addrSize, Variable::Status));
   l->getVariable()->setDescription("Number of EOFE errors");
   l->setPollEnable(true);

   addRegisterLink(l = new RegisterLink("DataBusErrorCount", baseAddress_ + 0x03*addrSize, Variable::Status));
   l->getVariable()->setDescription("Number of data bus errors");
   l->setPollEnable(true);

   addRegisterLink(l = new RegisterLink("WordStrbErrorCount", baseAddress_ + 0x04*addrSize, Variable::Status));
   l->getVariable()->setDescription("Number of word errors");
   l->setPollEnable(true);

   addRegisterLink(l = new RegisterLink("BitStrbErrorCount", baseAddress_ + 0x05*addrSize, Variable::Status));
   l->getVariable()->setDescription("Number of bit errors");
   l->setPollEnable(true);

   addRegisterLink(l = new RegisterLink("RxFifoOverflowCount", baseAddress_ + 0x06*addrSize, Variable::Status));
   
   addRegisterLink(l = new RegisterLink("RxFifoPauseCount", baseAddress_ + 0x07*addrSize, Variable::Status));
   
   addRegisterLink(l = new RegisterLink("TxFifoOverflowCount", baseAddress_ + 0x08*addrSize, Variable::Status));
   
   addRegisterLink(l = new RegisterLink("TxFifoPauseCount", baseAddress_ + 0x09*addrSize, Variable::Status));

   addRegisterLink(l = new RegisterLink("Dummy", baseAddress_ + 0x0a*addrSize, Variable::Configuration));
   l->getVariable()->setPerInstance(true);

   cout << "PrbsRx:Status:address: " << hex << baseAddress_ + 0x70*addrSize << endl;
   addRegisterLink(l = new RegisterLink("Status", baseAddress_ + 0x70*addrSize, Variable::Status));

   addRegisterLink(l = new RegisterLink("PacketLength", baseAddress_ + 0x71*addrSize, Variable::Status));
   l->setPollEnable(true);
   
   addRegisterLink(l = new RegisterLink("PacketRate", baseAddress_ + 0x72*addrSize, Variable::Status));
   l->setPollEnable(true);
   
   addRegisterLink(l = new RegisterLink("BitErrorCount", baseAddress_ + 0x73*addrSize, Variable::Status));
   l->setPollEnable(true);
   
   addRegisterLink(l = new RegisterLink("WordErrorCount", baseAddress_ + 0x74*addrSize, Variable::Status));   
   l->setPollEnable(true);
   
   addRegisterLink(l = new RegisterLink("RolloverEnable", baseAddress_ + 0xF0*addrSize, Variable::Configuration));
   l->getVariable()->setPerInstance(true);
  
   addRegister(new Register("CountReset", baseAddress_ + 0xFF*addrSize));

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   c = new Command("ResetCounters");
   c->setDescription("Reset all the error and rate counters");
   addCommand(c);

}

// Deconstructor
PrbsRx::~PrbsRx ( ) { }

// Process Commands
void PrbsRx::command(string name, string arg) {
   Register *r;
   if (name == "ResetCounters") {
      REGISTER_LOCK
      r = getRegister("CountReset");
      r->set(0x1);
      r->setStale();
      writeRegister(r, true, false);
      REGISTER_UNLOCK
   }
}

void PrbsRx::countReset() {
   command("ResetCounters","");
   Device::countReset();
}

