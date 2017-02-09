//-----------------------------------------------------------------------------
// File          : DataDpmWibDbg.cpp
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

#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>

#include <DataDpmWibDbg.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

using namespace std;

DataDpmWibDbg::DataDpmWibDbg(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"DataDpmWibDbg",index,parent){
   // Description
   desc_ = "DataDpmWibDbg object";
   RegisterLink * rl; 
   stringstream tmp;
   uint32_t i;  

   // Register links
   addRegisterLink(rl = new RegisterLink("CapturingStatus", baseAddress_ + 0x000, Variable::Status,29,0x1));
   rl->getVariable()->setDescription("Logging status");

   vector<string> state;
   state.resize(2);
   state[0]   = "Completed.   Data[255:0] Valid";
   state[1]   = "In progress. Data[255:0] invalid";
   rl->getVariable()->setEnums(state); 
   
   for (i=1;i<256;i++) {
      tmp.str(""); 
      tmp << "DATA" << dec << setw(3) << setfill('0') << i;
      addRegisterLink(rl = new RegisterLink(tmp.str(), baseAddress_ + (4*i), Variable::Status));
      rl->getVariable()->setDescription(tmp.str());                                               
   }   
   
}

// Deconstructor
DataDpmWibDbg::~DataDpmWibDbg ( ) { }
