//-----------------------------------------------------------------------------
// File          : AxiDac7654.cpp
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 09/05/2013
// Project       : Generic 
//-----------------------------------------------------------------------------
// Description :
// DAQ Device Driver for the DAC7654 IC
//
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
// 09/04/2013: created
//-----------------------------------------------------------------------------

#include <AxiDac7654.h>

#include <Register.h>
#include <Variable.h>
#include <Command.h>

#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

// Constructor
AxiDac7654::AxiDac7654 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"AxiDac7654",index,parent) {

   stringstream tmp;    
   uint32_t i;   
                        
   // Description
   desc_ = "AxiDac7654 object.";
   
   for (i=0;i<4;i++)
   {
      tmp.str("");
      tmp << "DAC" << dec << setw(1) << setfill('0') << i;// DAC[3:0]
      
      addRegister(new Register(tmp.str(), (baseAddress_+(0x80+i)*addrSize)));
      addVariable(new Variable(tmp.str(), Variable::Configuration));
      getVariable(tmp.str())->setRange(0,0xFFFF);
      getVariable(tmp.str())->setPerInstance(true);      
      getVariable(tmp.str())->setDescription("");
   } 

   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
AxiDac7654::~AxiDac7654 ( ) { }

// Method to process a command
void AxiDac7654::command ( string name, string arg ) {
   asm("nop");//no operation
}

// Method to read status registers and update variables
void AxiDac7654::readStatus ( ) {
   asm("nop");//no operation
}

// Method to read configuration registers and update variables
void AxiDac7654::readConfig ( ) {
   stringstream tmp;    
   uint32_t i; 
   
   REGISTER_LOCK

   for (i=0;i<4;i++) 
   {
      tmp.str("");
      tmp << "DAC" << dec << setw(1) << setfill('0') << i;// DAC[3:0]
      
      readRegister(getRegister(tmp.str()));
      getVariable(tmp.str())->setInt(getRegister(tmp.str())->get(0,0xFFFF));  
   } 

   REGISTER_UNLOCK
}

// Method to write configuration registers
void AxiDac7654::writeConfig ( bool force ) {
   stringstream tmp;    
   uint32_t i;  

   REGISTER_LOCK

   // Set registers
   for (i=0;i<4;i++) 
   {
      tmp.str("");
      tmp << "DAC" << dec << setw(1) << setfill('0') << i;// DAC[3:0]
      
      getRegister(tmp.str())->set(getVariable(tmp.str())->getInt(),0,0xFFFF);
      writeRegister(getRegister(tmp.str()),force,true);
   }
  
   REGISTER_UNLOCK
}
