//-----------------------------------------------------------------------------
// File          : AxiSy56040.cpp
// Author        : Matt Weaver  <weaver@slac.stanford.edu>
// Created       : 10/27/2015
//-----------------------------------------------------------------------------
// Description :
// LegacyControl Top Device
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
//
// DATE        WHO  WHAT
// ----------  ---  -----------------------------------------------------------
// 2016.09.10  jjr  Removed unused buff[32] in AxiSy6540 constructor
// 10/27/2015: created
//-----------------------------------------------------------------------------

#include <AxiSy56040.h>
#include <System.h>
#include <CommLink.h>
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
AxiSy56040::AxiSy56040 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"AxiSy56040",index,parent) {
   stringstream tmp;                             

   // Description
   desc_ = "AxiSy56040 object.";

   // -----------------
   // 2016.09.10 -- jjr
   // -----------------
   // Removed as unused
   // char buff[32];
   // -----------------

   //
   //  Configuration fields
   //
   Variable::VariableType v = Variable::Configuration;

   addRegisterLink(new RegisterLink("Out0Map" , baseAddress_ + 0*addrSize, 1, v));
   addRegisterLink(new RegisterLink("Out1Map" , baseAddress_ + 1*addrSize, 1, v));
   addRegisterLink(new RegisterLink("Out2Map" , baseAddress_ + 2*addrSize, 1, v));
   addRegisterLink(new RegisterLink("Out3Map" , baseAddress_ + 3*addrSize, 1, v));

   // Variables
   Variable* va;
   va = getVariable("Enabled");
   va->set("True");
   va->setHidden(true);

   //Commands
   Command *c;

   c = new Command("FPGATiming");
   c->setDescription("Route Timing from FPGA");
   addCommand(c);

   c = new Command("BPTiming");
   c->setDescription("Route Timing from Backplane");
   addCommand(c);

   c = new Command("RTM0Timing");
   c->setDescription("Route Timing from RTM0");
   addCommand(c);

   c = new Command("RTM1Timing");
   c->setDescription("Route Timing from RTM1");
   addCommand(c);
}

// Deconstructor
AxiSy56040::~AxiSy56040 ( ) { }

// Process Commands
void AxiSy56040::command(string name, string arg) {
   unsigned v;
   if (name == "FPGATiming")
     v = 1;
   else if (name == "BPTiming")
     v = 2;
   else if (name == "RTM0Timing")
     v = 0;
   else if (name == "RTM1Timing")
     v = 3;
   else
     return;

   Register *r;
   REGISTER_LOCK
     r = getRegister("Out0Map");
   r->set(v);
   writeRegister(r, true, false);
   system_->commLink()->queueRegister(linkConfig_,r,true,true);
   r = getRegister("Out1Map");
   r->set(v);
   writeRegister(r, true, false);
   system_->commLink()->queueRegister(linkConfig_,r,true,true);
   r = getRegister("Out2Map");
   r->set(v);
   writeRegister(r, true, false);
   system_->commLink()->queueRegister(linkConfig_,r,true,true);
   r = getRegister("Out3Map");
   r->set(v);
   writeRegister(r, true, false);
   system_->commLink()->queueRegister(linkConfig_,r,true,true);
   REGISTER_UNLOCK
}
