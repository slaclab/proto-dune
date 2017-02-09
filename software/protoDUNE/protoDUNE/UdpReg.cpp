//-----------------------------------------------------------------------------
// File          : UdpReg.cpp
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

#include <UdpReg.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

using namespace std;

UdpReg::UdpReg(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"UdpReg",index,parent){
   // Description
   desc_ = "UdpReg object";
 
   // Register links
   addRegisterLink(new RegisterLink("RemotePort",     baseAddress_ + 0x800, Variable::Status,0,0xFFFF)); 
   addRegisterLink(new RegisterLink("RemoteIpAddr",   baseAddress_ + 0x804, Variable::Status,0,0xFFFFFFFF)); 
   addRegisterLink(new RegisterLink("LocalIpAddr",    baseAddress_ + 0xFF4, Variable::Status,0,0xFFFFFFFF)); 
   addRegisterLink(new RegisterLink("LocalMacAddrLo", baseAddress_ + 0xFF8, Variable::Status,0,0xFFFFFFFF)); 
   addRegisterLink(new RegisterLink("LocalMacAddrHi", baseAddress_ + 0xFFC, Variable::Status,0,0xFFFF));   
   
}

// Deconstructor
UdpReg::~UdpReg ( ) { }

// command
void UdpReg::command(string name, string arg) {
   Device::command(name, arg);
}

// Hard Reset
void UdpReg::hardReset () {
   Device::hardReset();
}

// Soft Reset
void UdpReg::softReset () {
   Device::softReset();
}

// Count Reset
void UdpReg::countReset () {
   Device::countReset();
}

// Method to read status registers and update variables
void UdpReg::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void UdpReg::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void UdpReg::writeConfig ( bool force ) {  
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void UdpReg::verifyConfig ( ) {
   Device::verifyConfig();
}
