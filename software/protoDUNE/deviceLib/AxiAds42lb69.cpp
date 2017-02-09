//-----------------------------------------------------------------------------
// File          : AxiAds42lb69.cpp
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 04/18/2014
// Project       : 
//-----------------------------------------------------------------------------
// Description :
// DAQ Device Driver for the LTC2270 ADC IC
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

#include <AxiAds42lb69.h>

#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>

#include <sstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

// Constructor
AxiAds42lb69::AxiAds42lb69 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) :
                        Device(linkConfig,baseAddress,"AxiAds42lb69",index,parent) {                        
   Variable     * v;
   RegisterLink * rl;
   
   // Description
   desc_ = "AxiAds42lb69 Controller object.";
   
   // Create Registers: name, address
   addRegister(new Register("Reg06", baseAddress_ + (0x06*addrSize)));
   addRegister(new Register("Reg07", baseAddress_ + (0x07*addrSize)));
   addRegister(new Register("Reg08", baseAddress_ + (0x08*addrSize)));
   addRegister(new Register("Reg0B", baseAddress_ + (0x0B*addrSize)));
   addRegister(new Register("Reg0C", baseAddress_ + (0x0C*addrSize)));
   addRegister(new Register("Reg0D", baseAddress_ + (0x0D*addrSize)));
   addRegister(new Register("Reg0F", baseAddress_ + (0x0F*addrSize)));
   addRegister(new Register("Reg10", baseAddress_ + (0x10*addrSize)));
   addRegister(new Register("Reg11", baseAddress_ + (0x11*addrSize)));
   addRegister(new Register("Reg12", baseAddress_ + (0x12*addrSize)));
   addRegister(new Register("Reg13", baseAddress_ + (0x13*addrSize)));
   addRegister(new Register("Reg14", baseAddress_ + (0x14*addrSize)));
   addRegister(new Register("Reg15", baseAddress_ + (0x15*addrSize)));
   addRegister(new Register("Reg16", baseAddress_ + (0x16*addrSize)));
   addRegister(new Register("Reg17", baseAddress_ + (0x17*addrSize)));
   addRegister(new Register("Reg18", baseAddress_ + (0x18*addrSize)));
   addRegister(new Register("Reg1F", baseAddress_ + (0x1F*addrSize)));
   addRegister(new Register("Reg20", baseAddress_ + (0x20*addrSize)));

   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_0", baseAddress_ + (0x60*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_1", baseAddress_ + (0x61*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_2", baseAddress_ + (0x62*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_3", baseAddress_ + (0x63*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_4", baseAddress_ + (0x64*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_5", baseAddress_ + (0x65*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_6", baseAddress_ + (0x66*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_0_7", baseAddress_ + (0x67*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_0", baseAddress_ + (0x68*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_1", baseAddress_ + (0x69*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_2", baseAddress_ + (0x6A*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_3", baseAddress_ + (0x6B*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_4", baseAddress_ + (0x6C*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_5", baseAddress_ + (0x6D*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_6", baseAddress_ + (0x6E*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   addRegisterLink(rl = new RegisterLink("AdcSmpl_1_7", baseAddress_ + (0x6F*addrSize), Variable::Status)); rl->getVariable()->setRange(0,0xFFFF);    
   
   addRegisterLink(rl = new RegisterLink("DelayRdy", baseAddress_ + (0x7F*addrSize), Variable::Status)); rl->getVariable()->setTrueFalse();    
   
   addRegisterLink(rl = new RegisterLink("DelayData_0_0", baseAddress_ + (0x80*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_1", baseAddress_ + (0x81*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_2", baseAddress_ + (0x82*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_3", baseAddress_ + (0x83*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_4", baseAddress_ + (0x84*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_5", baseAddress_ + (0x85*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_6", baseAddress_ + (0x86*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_0_7", baseAddress_ + (0x87*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_1_0", baseAddress_ + (0x88*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true);     
   addRegisterLink(rl = new RegisterLink("DelayData_1_1", baseAddress_ + (0x89*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_1_2", baseAddress_ + (0x8A*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_1_3", baseAddress_ + (0x8B*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_1_4", baseAddress_ + (0x8C*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_1_5", baseAddress_ + (0x8D*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true); 
   addRegisterLink(rl = new RegisterLink("DelayData_1_6", baseAddress_ + (0x8E*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true);    
   addRegisterLink(rl = new RegisterLink("DelayData_1_7", baseAddress_ + (0x8F*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x1F); rl->getVariable()->setPerInstance(true);             
            
   addRegisterLink(rl = new RegisterLink("DelayMode", baseAddress_ + (0x90*addrSize), Variable::Configuration)); rl->getVariable()->setRange(0,0x3);; rl->getVariable()->setPerInstance(true);      

   // Configuration Variables
   v = new Variable("ClkDiv", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Internal clock divider for input sample clock");
   vector<string> ClkDiv;
   ClkDiv.resize(4);
   ClkDiv[0]   = "Bypassed";
   ClkDiv[1]   = "Divide-by-2";
   ClkDiv[2]   = "Divide-by-1";
   ClkDiv[3]   = "Divide-by-4";
   v->setEnums(ClkDiv); 
   addVariable(v);
   
   v = new Variable("SyncInDelay", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Controls the delay of the SYNCIN input with respect to the input clock");
   vector<string> SyncInDelay;
   SyncInDelay.resize(8);
   SyncInDelay[0]   = "0-ps delay";
   SyncInDelay[1]   = "60-ps delay";
   SyncInDelay[2]   = "120-ps delay";
   SyncInDelay[3]   = "180-ps delay";
   SyncInDelay[4]   = "240-ps delay";
   SyncInDelay[5]   = "300-ps delay";
   SyncInDelay[6]   = "360-ps delay";
   SyncInDelay[7]   = "420-ps delay";
   v->setEnums(SyncInDelay); 
   addVariable(v);   
   
   v = new Variable("PwnDwnChA", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Power Down Channel A");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("PwnDwnChB", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Power Down Channel B");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("StandBy", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Dual ADC is placed into standby mode");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("DataFormat", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Digital output data format");
   vector<string> DataFormat;
   DataFormat.resize(2);
   DataFormat[0] = "Twos complement";
   DataFormat[1] = "Offset binary";
   v->setEnums(DataFormat); 
   addVariable(v);    

   v = new Variable("DisableCtrlPins", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Disables power-down control from the CTRL1, CTRL2 pins");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("TestPatternAlign", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Aligns test patterns of two channels");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("Reset", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Software reset: This bit resets all internal registers to the default values and selfclears to ‘0’");
   v->setTrueFalse();
   addVariable(v);    
   
   v = new Variable("ChAGain", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Digital gain for channel A");
   vector<string> ChAGain;
   ChAGain.resize(20);
   ChAGain[0]  = "2.0 Vpp";
   ChAGain[1]  = "Do not use";
   ChAGain[2]  = "Do not use";
   ChAGain[3]  = "2.5 Vpp";
   ChAGain[4]  = "2.4 Vpp";
   ChAGain[5]  = "2.2 Vpp";
   ChAGain[6]  = "2.1 Vpp";
   ChAGain[7]  = "2.0 Vpp";
   ChAGain[8]  = "1.9 Vpp";
   ChAGain[9]  = "1.8 Vpp";
   ChAGain[10] = "1.7 Vpp";
   ChAGain[11] = "1.6 Vpp";
   ChAGain[12] = "1.5 Vpp";
   ChAGain[13] = "1.4 Vpp";
   ChAGain[14] = "1.3 Vpp";
   ChAGain[15] = "1.25 Vpp";
   ChAGain[16] = "1.2 Vpp";
   ChAGain[17] = "1.1 Vpp";
   ChAGain[18] = "1.05 Vpp";
   ChAGain[19] = "1.0 Vpp";
   v->setEnums(ChAGain); 
   addVariable(v);    
   
   v = new Variable("ChAGainEn", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Digital gain enable bit for channel A");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("FlipData", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Flips bit order on the LVDS output bus (LSB versus MSB)");
   v->setTrueFalse();
   addVariable(v);    
   
   v = new Variable("ChBGain", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Digital gain for channel B");
   vector<string> ChBGain;
   ChBGain.resize(20);
   ChBGain[0]  = "2.0 Vpp";
   ChBGain[1]  = "Do not use";
   ChBGain[2]  = "Do not use";
   ChBGain[3]  = "2.5 Vpp";
   ChBGain[4]  = "2.4 Vpp";
   ChBGain[5]  = "2.2 Vpp";
   ChBGain[6]  = "2.1 Vpp";
   ChBGain[7]  = "2.0 Vpp";
   ChBGain[8]  = "1.9 Vpp";
   ChBGain[9]  = "1.8 Vpp";
   ChBGain[10] = "1.7 Vpp";
   ChBGain[11] = "1.6 Vpp";
   ChBGain[12] = "1.5 Vpp";
   ChBGain[13] = "1.4 Vpp";
   ChBGain[14] = "1.3 Vpp";
   ChBGain[15] = "1.25 Vpp";
   ChBGain[16] = "1.2 Vpp";
   ChBGain[17] = "1.1 Vpp";
   ChBGain[18] = "1.05 Vpp";
   ChBGain[19] = "1.0 Vpp";
   v->setEnums(ChBGain); 
   addVariable(v);    
   
   v = new Variable("ChBGainEn", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Digital gain enable bit for channel A");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("OvrOnLsb", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Functions only with the DDR interface option. Replaces the LSB pair of 16-bit data (D1, D0) with OVR information");
   vector<string> OvrOnLsb;
   OvrOnLsb.resize(4);
   OvrOnLsb[0]  = "D1 and D0 are output in the ADS42LB69, NC for the ADS42LB49";
   OvrOnLsb[1]  = "Fast OVR in LVDS logic level";
   OvrOnLsb[2]  = "Normal OVR in LVDS logic level";
   OvrOnLsb[3]  = "D1 and D0 are output in the ADS42LB69, NC for the ADS42LB49";
   v->setEnums(OvrOnLsb); 
   addVariable(v);     
   
   v = new Variable("FastOvrOnPin", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Digital gain enable bit for channel A");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("ChATestPatterns", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Functions only with the DDR interface option. Replaces the LSB pair of 16-bit data (D1, D0) with OVR information");
   vector<string> ChATestPatterns;
   ChATestPatterns.resize(12);
   ChATestPatterns[0]   = "Normal operation";
   ChATestPatterns[1]   = "Outputs all 0s";
   ChATestPatterns[2]   = "Outputs all 1s";
   ChATestPatterns[3]   = "Outputs toggle pattern";
   ChATestPatterns[4]   = "Output digital ramp";
   ChATestPatterns[5]   = "Increment pattern: Do not use";
   ChATestPatterns[6]   = "Single pattern";
   ChATestPatterns[7]   = "Double pattern";
   ChATestPatterns[8]   = "Deskew pattern";
   ChATestPatterns[9]   = "Do not use";
   ChATestPatterns[10]  = "PRBS pattern";
   ChATestPatterns[11]  = "8-point sine wave";
   v->setEnums(ChATestPatterns); 
   addVariable(v);  

   v = new Variable("ChBTestPatterns", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Functions only with the DDR interface option. Replaces the LSB pair of 16-bit data (D1, D0) with OVR information");
   vector<string> ChBTestPatterns;
   ChBTestPatterns.resize(12);
   ChBTestPatterns[0]   = "Normal operation";
   ChBTestPatterns[1]   = "Outputs all 0s";
   ChBTestPatterns[2]   = "Outputs all 1s";
   ChBTestPatterns[3]   = "Outputs toggle pattern";
   ChBTestPatterns[4]   = "Output digital ramp";
   ChBTestPatterns[5]   = "Increment pattern: Do not use";
   ChBTestPatterns[6]   = "Single pattern";
   ChBTestPatterns[7]   = "Double pattern";
   ChBTestPatterns[8]   = "Deskew pattern";
   ChBTestPatterns[9]   = "Do not use";
   ChBTestPatterns[10]  = "PRBS pattern";
   ChBTestPatterns[11]  = "8-point sine wave";
   v->setEnums(ChBTestPatterns); 
   addVariable(v);     
   
   v = new Variable("CustomPattern1", Variable::Configuration);
   v->setPerInstance(true);
   v->setRange(0,0xFFFF);
   addVariable(v);  

   v = new Variable("CustomPattern2", Variable::Configuration);
   v->setPerInstance(true);
   v->setRange(0,0xFFFF);
   addVariable(v);     
   
   v = new Variable("LvdsClkStrength", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Increases the LVDS drive strength of the CLKOUT+/-");
   v->setTrueFalse();
   addVariable(v); 

   v = new Variable("LvdsDataStrength", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Increases the LVDS drive strength of the DATAOUT+/-");
   v->setTrueFalse();
   addVariable(v);   

   v = new Variable("DisableOutputChA", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Disables LVDS output buffers of channel A");
   v->setTrueFalse();
   addVariable(v);  

   v = new Variable("DisableOutputChB", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Disables LVDS output buffers of channel B");
   v->setTrueFalse();
   addVariable(v);     

   v = new Variable("LvdsDataMode", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Selects output interface between DDR and QDR LVDS mode");
   vector<string> LvdsDataMode;
   LvdsDataMode.resize(2);
   LvdsDataMode[0] = "QDR LVDS mode";
   LvdsDataMode[1] = "DDR LVDS mode";
   v->setEnums(LvdsDataMode); 
   addVariable(v);     
   
   v = new Variable("DdrOutputTiming", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("DDR Output Timing: Effective only when the DIS CTRL PINS bit is set to '1'.");
   v->setRange(0,0x1F);
   addVariable(v);        
   
   v = new Variable("LvdsClkStrengthEn", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Enables LvdsClkStrength");
   v->setTrueFalse();
   addVariable(v);

   v = new Variable("QdrOutputTimingChA", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Adjusts position of output data clock on channel A with respect to output data.");
   v->setRange(0,0x1F);
   addVariable(v);   
   
   v = new Variable("InvClkOutChA", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Inverts polarity of the output clock for channel A (QDR mode only)");
   v->setTrueFalse();
   addVariable(v);  

   v = new Variable("QdrOutputTimingChB", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Adjusts position of output data clock on channel B with respect to output data.");
   v->setRange(0,0x1F);
   addVariable(v);   
   
   v = new Variable("InvClkOutChB", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Inverts polarity of the output clock for channel B (QDR mode only)");
   v->setTrueFalse();
   addVariable(v);     
     
   v = new Variable("FastOvrThreshold", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Adjusts position of output data clock on channel B with respect to output data.");
   v->setRange(0,0x3F);
   addVariable(v);    
   
   v = new Variable("PdnOvrForCtrlPins", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("Determines if the CTRL1, CTRL2 pins are power-down control or OVR outputs");
   v->setTrueFalse();
   addVariable(v);       

}

// Deconstructor
AxiAds42lb69::~AxiAds42lb69 ( ) { }

// Method to process a command
void AxiAds42lb69::command ( string name, string arg ) {
   Device::command(name, arg);
}

// Hard Reset
void AxiAds42lb69::hardReset () {
   Device::hardReset();
}

// Soft Reset
void AxiAds42lb69::softReset () {
   Device::softReset();
}

// Count Reset
void AxiAds42lb69::countReset () {
   Device::countReset();
}

// Method to read status registers and update variables
void AxiAds42lb69::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void AxiAds42lb69::readConfig ( ) {
   char       buffer[100];
   Register *r1, *r2;
   
   REGISTER_LOCK

   regReadInt("Reg06","ClkDiv",0,0x3); 
   
   regReadInt("Reg07","SyncInDelay",0,0x7); 
   
   regReadInt("Reg08","PwnDwnChA",       7,0x1); 
   regReadInt("Reg08","PwnDwnChB",       6,0x1); 
   regReadInt("Reg08","StandBy",         5,0x1); 
   regReadInt("Reg08","DataFormat",      4,0x1); 
   regReadInt("Reg08","DisableCtrlPins", 3,0x1); 
   regReadInt("Reg08","TestPatternAlign",2,0x1); 
   regReadInt("Reg08","Reset",           0,0x1); 
   
   regReadInt("Reg0B","ChAGain",  3,0x1F); 
   regReadInt("Reg0B","ChAGainEn",2,0x1); 
   regReadInt("Reg0B","FlipData", 0,0x1); 
   
   regReadInt("Reg0C","ChBGain",  3,0x1F); 
   regReadInt("Reg0C","ChBGainEn",2,0x1); 
   regReadInt("Reg0C","OvrOnLsb", 0,0x3);    
   
   regReadInt("Reg0D","FastOvrOnPin",0,0x1);    
   
   regReadInt("Reg0F","ChATestPatterns",4,0xF); 
   regReadInt("Reg0F","ChBTestPatterns",0,0xF);   

   readRegister(r2 = getRegister("Reg10"));
   readRegister(r1 = getRegister("Reg11"));
   sprintf(buffer,"0x%.2x%.2x",r2->get(),r1->get());
   set("CustomPattern1",buffer); 

   readRegister(r2 = getRegister("Reg12"));
   readRegister(r1 = getRegister("Reg13"));
   sprintf(buffer,"0x%.2x%.2x",r2->get(),r1->get());
   set("CustomPattern2",buffer);    
   
   regReadInt("Reg14","LvdsClkStrength", 3,0x1);    
   regReadInt("Reg14","LvdsDataStrength",2,0x1);    
   regReadInt("Reg14","DisableOutputChA",1,0x1);    
   regReadInt("Reg14","DisableOutputChB",0,0x1);    
   
   regReadInt("Reg15","LvdsDataMode",0,0x1);    
   
   regReadInt("Reg16","DdrOutputTiming",1,0x1F);    
   
   regReadInt("Reg17","LvdsClkStrengthEn", 7,0x1);    
   regReadInt("Reg17","QdrOutputTimingChA",1,0x1F);    
   regReadInt("Reg17","InvClkOutChA",      0,0x1);    
   
   regReadInt("Reg18","QdrOutputTimingChB",1,0x1F);    
   regReadInt("Reg18","InvClkOutChB",      0,0x1);  
   
   regReadInt("Reg1F","FastOvrThreshold",0,0x3F);       
   
   regReadInt("Reg20","PdnOvrForCtrlPins",0,0x1);  
      
   REGISTER_UNLOCK   
   
   Device::readConfig();
}

// Method to write configuration registers
void AxiAds42lb69::writeConfig ( bool force ) {
   Register *r1, *r2;
   
   REGISTER_LOCK

   getRegister("Reg06")->set(getVariable("ClkDiv")->getInt(),0,0x3);   
   getRegister("Reg06")->set(0x1,7,0x1);//Reg06 expects BIT7 = '1'
   writeRegister(getRegister("Reg06"),force,true);
   
   regWrite("Reg07","SyncInDelay",force,0,0x7); 
   
   regWrite("Reg08","PwnDwnChA",force,       7,0x1); 
   regWrite("Reg08","PwnDwnChB",force,       6,0x1); 
   regWrite("Reg08","StandBy",force,         5,0x1); 
   regWrite("Reg08","DataFormat",force,      4,0x1); 
   regWrite("Reg08","DisableCtrlPins",force, 3,0x1); 
   regWrite("Reg08","TestPatternAlign",force,2,0x1); 
   regWrite("Reg08","Reset",force,           0,0x1); 
   
   regWrite("Reg0B","ChAGain",force,  3,0x1F); 
   regWrite("Reg0B","ChAGainEn",force,2,0x1); 
   regWrite("Reg0B","FlipData",force, 0,0x1); 
   
   regWrite("Reg0C","ChBGain",force,  3,0x1F); 
   regWrite("Reg0C","ChBGainEn",force,2,0x1); 
   regWrite("Reg0C","OvrOnLsb",force, 0,0x3);    
   
   getRegister("Reg0D")->set(getVariable("FastOvrOnPin")->getInt(),0,0x1);   
   getRegister("Reg0D")->set(0x1,2,0x1);//Reg0D expects BIT2 = '1'
   getRegister("Reg0D")->set(0x1,3,0x1);//Reg0D expects BIT3 = '1'
   getRegister("Reg0D")->set(0x1,5,0x1);//Reg0D expects BIT5 = '1'
   getRegister("Reg0D")->set(0x1,6,0x1);//Reg0D expects BIT6 = '1'
   writeRegister(getRegister("Reg0D"),force,true);   
   
   regWrite("Reg0F","ChATestPatterns",force,4,0xF); 
   regWrite("Reg0F","ChBTestPatterns",force,0,0xF);   

   r2 = getRegister("Reg10");
   r1 = getRegister("Reg11");
   sscanf(get("CustomPattern1").c_str(),"0x%2x%2x",(uint32_t *)r2->data(),(uint *)r1->data());
   writeRegister(r1,force);
   writeRegister(r2,force);  

   r2 = getRegister("Reg12");
   r1 = getRegister("Reg13");
   sscanf(get("CustomPattern2").c_str(),"0x%2x%2x",(uint32_t *)r2->data(),(uint *)r1->data());
   writeRegister(r1,force);
   writeRegister(r2,force);     
   
   regWrite("Reg14","LvdsClkStrength",force, 3,0x1);    
   regWrite("Reg14","LvdsDataStrength",force,2,0x1);    
   regWrite("Reg14","DisableOutputChA",force,1,0x1);    
   regWrite("Reg14","DisableOutputChB",force,0,0x1);    
   
   regWrite("Reg15","LvdsDataMode",force,0,0x1);    
   
   regWrite("Reg16","DdrOutputTiming",force,1,0x1F);    
   
   regWrite("Reg17","LvdsClkStrengthEn",force, 7,0x1);    
   regWrite("Reg17","QdrOutputTimingChA",force,1,0x1F);    
   regWrite("Reg17","InvClkOutChA",force,      0,0x1);    
   
   regWrite("Reg18","QdrOutputTimingChB",force,1,0x1F);    
   regWrite("Reg18","InvClkOutChB",force,      0,0x1);  
   
   regWrite("Reg1F","FastOvrThreshold",force,0,0x3F);       
   
   regWrite("Reg20","PdnOvrForCtrlPins",force,0,0x1);  
      
   REGISTER_UNLOCK     

   Device::writeConfig(force);
}

// Verify hardware state of configuration
void AxiAds42lb69::verifyConfig ( ) {
   Device::verifyConfig();
}