//-----------------------------------------------------------------------------
// File          : Adc16Dx370.cpp
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 27/04/2015
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device container for Adc16Dx370
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
// 27/04/2015: created
//-----------------------------------------------------------------------------
#include <Adc16Dx370.h>
#include <Register.h>
#include <RegisterLink.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <unistd.h>

using namespace std;

// Constructor
Adc16Dx370::Adc16Dx370 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"Adc16Dx370",index,parent) {
   uint32_t i;
   RegisterLink *rl;
   stringstream tmp;
   Command      *c;
   
   // Description
   desc_ = "ADC data acquisition control";
   
   addRegisterLink(new RegisterLink("ID_DEVICE_TYPE",baseAddress_ + 0x003*addrSize, Variable::Status));
   addRegisterLink(new RegisterLink("ID_PROD_UPPER", baseAddress_ + 0x004*addrSize, Variable::Status));
   addRegisterLink(new RegisterLink("ID_PROD_LOWER", baseAddress_ + 0x005*addrSize, Variable::Status));
   addRegisterLink(new RegisterLink("ID_MASKREV",    baseAddress_ + 0x006*addrSize, Variable::Status));
   addRegisterLink(new RegisterLink("ID_VNDR_UPPER", baseAddress_ + 0x00C*addrSize, Variable::Status));
   addRegisterLink(new RegisterLink("ID_VNDR_LOWER", baseAddress_ + 0x00D*addrSize, Variable::Status));   
   
   // Create Registers: name, address
   for (i=ADC_START_ADDR;i<=ADC_END_ADDR;i++) {
      if      (i==0x01) {
         // Skip reserved
      }
      else if (i>=0x3 && i<=0x0F) {
         // Skip reserved
      }
      else if (i==0x011) {
         // Skip reserved
      }      
      else if (i>=0x16 && i<=0x18) {
         // Skip reserved
      }
      else if (i>=0x1A && i<=0x3A) {
         // Skip reserved
      }
      else if (i>=0x3E && i<=0x46) {
         // Skip reserved
      }
      else if (i>=0x48 && i<=0x5F) {
         // Skip reserved
      }
      else if (i>=0x64 && i<=0x6B) {
         // Skip reserved
      }
      else if (i==0x6C) {
         // Status register
         addRegisterLink(rl = new RegisterLink("ADC_Status",    baseAddress_ + (0x6C*addrSize), 1, 7,
                                "Clock_ready",           Variable::Status, 0, 0x1,
                                "Calibration_done",      Variable::Status, 1, 0x1, 
                                "PLL_lock",              Variable::Status, 2, 0x1,
                                "Aligned_to_sysref",     Variable::Status, 3, 0x1,                                 
                                "Realigned_to_sysref",   Variable::Status, 4, 0x1,
                                "Sync_form_FPGA",        Variable::Status, 5, 0x1,                                 
                                "Link_active",           Variable::Status, 6, 0x1));                                
      }
      else if (i>=0x6D && i<=0x6F) {
         // Skip reserved
      }
      else if (i==0x71) {
         // Skip reserved
      }
      else {
         tmp.str("");
         tmp << "AdcReg0x" << hex << setw(4) << setfill('0') << hex << i;
         addRegisterLink(rl = new RegisterLink(tmp.str(), (baseAddress_+ (i*addrSize)), Variable::Configuration));
         rl->getVariable()->setPerInstance(true);
      }
   }  

   

   // Variables
   getVariable("Enabled")->setHidden(true);
   
   
   //Commands
   addCommand(c = new Command("CalibrateADC"));
   c->setDescription("ADC calibration (Powers down and then powers up the ADC)");

}

// Process Commands
void Adc16Dx370::command(string name, string arg) {
   if (name == "CalibrateADC") Calibrate();
   else Device::command(name,arg);
}


// Deconstructor
Adc16Dx370::~Adc16Dx370 ( ) { }

// Process Commands
//! ADC calibration after changing clock inputs
 void Adc16Dx370::Calibrate () {
   PowerDown();  
   usleep(1000000);//1000 ms sleep
   PowerUp();

}

void Adc16Dx370::PowerDown () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("AdcReg0x0002");
   r->set(0x3,0,0xff);
   writeRegister(r, true);  
   REGISTER_UNLOCK
}

void Adc16Dx370::PowerUp () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("AdcReg0x0002");
   r->set(0x0,0,0xff);
   writeRegister(r, true);  
   REGISTER_UNLOCK
}

