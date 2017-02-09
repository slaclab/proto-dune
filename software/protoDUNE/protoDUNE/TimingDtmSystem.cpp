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

#include <TimingDtmSystem.h>
#include <TimingDtm.h>

#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <CommLink.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include <MultLink.h>
#include <MultDestMapped.h>
#include <MultDestAxis.h>
#include <MultDestPgp.h>

using namespace std;

// Constructor
TimingDtmSystem::TimingDtmSystem (string defaults, uint32_t idx) : System("TimingDtmSystem") {
   stringstream tmp;
   MultLink * link;

   dest_[0] = new MultDestMapped (3, 0x80000000,0x2000,   // Core registers
                                     0x84000000,0x1000,   // BSI registers
                                     0xA0000000,0x1000);  // DTM registers

   dest_[1] = NULL;  // Spare

   link = new MultLink();
   //link->setDebug(true);
   link->setMaxRxTx(0x800000);
   link->open(2,dest_[0],dest_[1]);

   tmp.str("");
   tmp << "TimingDtm_" << std::dec << std::setw(3) << setfill('0') << idx;

   // Description
   desc_      = tmp.str();
   defaults_  = defaults;
   idx_       = idx;
   commLink_  = link;

   // Set run states
   vector<string> states;
   states.resize(2);
   states[0] = "Stopped";
   states[1] = "Enable";
   getVariable("RunState")->setEnums(states);

   set("HideRunControl","True");
   set("HideDataControl","True");
   getVariable("PollPeriod")->setInt(1);

   addDevice(new TimingDtm(0x00000000,idx,this));
}

// Deconstructor
TimingDtmSystem::~TimingDtmSystem ( ) { 
   delete commLink_;
   delete dest_[0];
   delete dest_[1];
}

//! Method to perform soft reset
void TimingDtmSystem::softReset ( ) { 
   System::softReset();   
}

//! Method to perform hard reset
void TimingDtmSystem::hardReset ( ) { 
   System::hardReset();
}

//! Method to set run state
void TimingDtmSystem::setRunState ( string state ) {
   System::setRunState(state);
}

//! Method to process a command
void TimingDtmSystem::command ( string name, string arg ) {
   System::command(name, arg);
}
