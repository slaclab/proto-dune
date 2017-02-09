//-----------------------------------------------------------------------------
// File          : DataDpmEmuCh.cpp
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

#include <DataDpmEmuCh.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>
#include <stdio.h>

using namespace std;

DataDpmEmuCh::DataDpmEmuCh(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"DataDpmEmuCh",index,parent){
   // Description
   desc_ = "DataDpmEmuCh object";
   RegisterLink * rl;
   uint32_t i;  
   char ChCfg[50];
   char DlyCfg[50];
   char ChNoiseCfg[50];   
   
   // Register links
   for (i=0;i<128;i++) {
      sprintf(ChCfg,"ChCfg_%03d",i);
      sprintf(DlyCfg,"DlyCfg_%03d",i);
      sprintf(ChNoiseCfg,"ChNoiseCfg_%03d",i);
      addRegisterLink(rl = new RegisterLink(ChCfg, baseAddress_ + 0x800 + (4*i), 1, 2,
                                            DlyCfg,Variable::Configuration, 0, 0x1F,  // 0                              
                                            ChNoiseCfg,Variable::Configuration, 16, 0x7));// 1     
      rl->getVariable(0)->setDescription("Delay Configuration (units of 1/2MHz)");        
      rl->getVariable(1)->setDescription("Sets the number of ADC channel noise bits in the emulation adc data");       
   }

   hideAllVariables();
   
}

// Deconstructor
DataDpmEmuCh::~DataDpmEmuCh ( ) { }

// command
void DataDpmEmuCh::command(string name, string arg) {
   Device::command(name, arg);
}

// Hard Reset
void DataDpmEmuCh::hardReset () {
   Device::hardReset();
}

// Soft Reset
void DataDpmEmuCh::softReset () {
   Device::softReset();
}

// Count Reset
void DataDpmEmuCh::countReset () {
   Device::countReset();
}

// Method to read status registers and update variables
void DataDpmEmuCh::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void DataDpmEmuCh::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void DataDpmEmuCh::writeConfig ( bool force ) {  
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void DataDpmEmuCh::verifyConfig ( ) {
   Device::verifyConfig();
}
