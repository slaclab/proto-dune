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
#include <DataCompressionSystem.h>
#include <DataCompressionDpm.h>
#include <DataCompression.h>

#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <CommLink.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <time.h>

#include <MultLink.h>
#include <MultDestMapped.h>
#include <MultDestAxis.h>

using namespace std;

// Constructor
DataCompressionSystem::DataCompressionSystem (string defaults, uint32_t idx ) : System("DataCompressionSystem") {
   stringstream tmp;
   MultLink * link;

   dest_ = new MultDestMapped (3, 0x80000000,0x2000,   // Core registers
                                  0x84000000,0x1000,   // BSI registers
                                  0xA0000000,0x10000); // DataCompression registers
   link = new MultLink();
   link->setMaxRxTx(0x800000);
   link->open(1,dest_);

   tmp.str("");
   tmp << "Data DPM " << dec << idx;

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

   addDevice(new DataCompressionDpm(0x00000000,idx,this));
}

// Deconstructor
DataCompressionSystem::~DataCompressionSystem ( ) { 
   delete commLink_;
   delete dest_;
}

//! Method to process a command
void DataCompressionSystem::command ( string name, string arg ) {
   
   // DataCompression *module;   
   // uint32_t            i;   
   
   if ( name == "ConfigDataCompression" ) {
/*
      // Set the pointer to the module
      module = (DataCompression*)(device("DataCompressionDpm",idx_)->device("DataCompression",0));
      // Load the configurations in DataCompression modules 
      // Place holder for future command or function
*/          
   } else System::command(name, arg);
}
