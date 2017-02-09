//-----------------------------------------------------------------------------
// File          : AxiAd5780.cpp
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 04/18/2014
// Project       : 
//-----------------------------------------------------------------------------
// Description :
// DAQ Device Driver for the AD5780 DAC IC
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
// 04/18/2014: created
//-----------------------------------------------------------------------------

#include <AxiAd5780.h>
#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

#define EN_MULTIPLE_INST   true

// Constructor
AxiAd5780::AxiAd5780 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) :
                        Device(linkConfig,baseAddress,"AxiAd5780",index,parent) {                        
   Variable * v;
   Command * c;   
   
   // Description
   desc_ = "AxiAd5780 Controller object.";
   
   // Create Registers: name, address   
   addRegister(new Register("dacRefreshRate", baseAddress_ + (0x10*addrSize)));
   addRegister(new Register("dacData",        baseAddress_ + (0x30*addrSize)));
   addRegister(new Register("debugMux",       baseAddress_ + (0x80*addrSize)));
   addRegister(new Register("debugData",      baseAddress_ + (0x90*addrSize)));
   addRegister(new Register("sdoDisable",     baseAddress_ + (0xA0*addrSize)));
   addRegister(new Register("binaryOffset",   baseAddress_ + (0xA1*addrSize)));
   addRegister(new Register("dacTriState",    baseAddress_ + (0xA2*addrSize)));
   addRegister(new Register("opGnd",          baseAddress_ + (0xA3*addrSize)));
   addRegister(new Register("rbuf",           baseAddress_ + (0xA4*addrSize))); 
   addRegister(new Register("halfSckPeriod",  baseAddress_ + (0xA5*addrSize))); 
   addRegister(new Register("dacRst",         baseAddress_ + (0xFE*addrSize)));
   
   // Status Variables   
   v = new Variable("dacRefreshRate",Variable::Status); v->setDescription(" "); addVariable(v);   
   v = new Variable("dacData",     Variable::Status); v->setDescription(" "); addVariable(v);   
   
   // Configuration Variables   
   v = new Variable("debugMux", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);  

   v = new Variable("debugData", Variable::Configuration);
   v->setDescription(" ");
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v); 
   
   v = new Variable("sdoDisable", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);  
   
   v = new Variable("binaryOffset", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);     
   
   v = new Variable("dacTriState", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);  
   
   v = new Variable("opGnd", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);  
   
   v = new Variable("rbuf", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);  
   
   v = new Variable("halfSckPeriod", Variable::Configuration);
   v->setDescription(" ");
   if(EN_MULTIPLE_INST) v->setPerInstance(true);
   addVariable(v);    

   // Command Variables       
   c = new Command("dacRst");       c->setDescription(" "); addCommand(c);   
}

// Deconstructor
AxiAd5780::~AxiAd5780 ( ) { }

// Method to process a command
void AxiAd5780::command ( string name, string arg ) {
   
   // Command is local
   if  ( name == "dacRst" ) {   
      REGISTER_LOCK
      myRegWrite(name,true);
      REGISTER_UNLOCK 
   }
}

// Method to read status registers and update variables
void AxiAd5780::readStatus ( ) {
   
   REGISTER_LOCK
   
   // Read registers
   myRegReadIntDec("dacRefreshRate"); 
   myRegReadInt("dacData"); 
      
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void AxiAd5780::readConfig ( ) {
  
   REGISTER_LOCK

   // Read registers
   myRegReadInt("debugMux",0,0x1);
   myRegReadInt("debugData");
   myRegReadInt("sdoDisable",0,0x1);
   myRegReadInt("binaryOffset",0,0x1);
   myRegReadInt("dacTriState",0,0x1);
   myRegReadInt("opGnd",0,0x1);
   myRegReadInt("rbuf",0,0x1);
   myRegReadIntDec("halfSckPeriod");
   
   REGISTER_UNLOCK
}

// Method to write configuration registers
void AxiAd5780::writeConfig ( bool force ) {
  
   REGISTER_LOCK

   // Write registers
   myRegWrite("debugMux",0,0x1,force);
   myRegWrite("debugData",force);
   myRegWrite("sdoDisable",0,0x1,force);
   myRegWrite("binaryOffset",0,0x1,force);
   myRegWrite("dacTriState",0,0x1,force);
   myRegWrite("opGnd",0,0x1,force);
   myRegWrite("rbuf",0,0x1,force);
   myRegWrite("halfSckPeriod",force);
  
   REGISTER_UNLOCK   
}

void AxiAd5780::myRegReadInt ( string nameReg, string nameVar, uint32_t bit, uint32_t mask) {
   Register * r;   
   r = getRegister(nameReg);
   readRegister(r);
   getVariable(nameVar)->setInt(r->get(bit, mask)); 
}

void AxiAd5780::myRegReadInt ( string name, uint32_t bit, uint32_t mask ){
   myRegReadInt(name,name,bit,mask);
}

void AxiAd5780::myRegReadInt ( string name ){
   myRegReadInt(name,name,0,0xFFFFFFFF);
}

void AxiAd5780::myRegReadIntDec ( string nameReg, string nameVar, uint32_t bit, uint32_t mask) { 
   Register * r;
   r = getRegister(nameReg);
   readRegister(r);
   getVariable(nameVar)->setBase10();
   getVariable(nameVar)->setInt(r->get(bit, mask)); 
}

void AxiAd5780::myRegReadIntDec ( string name, uint32_t bit, uint32_t mask ){
   myRegReadInt(name,name,bit,mask);
}

void AxiAd5780::myRegReadIntDec ( string name ){
   myRegReadIntDec(name,name,0,0xFFFFFFFF);
}

void AxiAd5780::myRegWrite ( string nameReg, string nameVar, uint32_t bit, uint32_t mask, bool force) {
   Register * r;
   r = getRegister(nameReg);
   r->set(getInt(nameVar), bit, mask);
   writeRegister(r, force); 
} 

void AxiAd5780::myRegWrite ( string name, uint32_t bit, uint32_t mask, bool force ){
   myRegWrite(name,name,bit,mask,force);
}

void AxiAd5780::myRegWrite ( string name, bool force ){
   myRegWrite(name,name,0,0xFFFFFFFF,force);
}
