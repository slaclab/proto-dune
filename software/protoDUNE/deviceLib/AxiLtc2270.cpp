//-----------------------------------------------------------------------------
// File          : AxiLtc2270.cpp
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

#include <AxiLtc2270.h>
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

// Constructor
AxiLtc2270::AxiLtc2270 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) :
                        Device(linkConfig,baseAddress,"AxiLtc2270",index,parent) {                        
   Variable * v;
   Command * c;
   stringstream tmp;
   uint32_t i,j;
   
   // Description
   desc_ = "AxiLtc2270 Controller object.";
   
   // Create Registers: name, address   
   addRegister(new Register("Reg00", baseAddress_ + (0x00*addrSize)));
   addRegister(new Register("Reg01", baseAddress_ + (0x01*addrSize)));
   addRegister(new Register("Reg02", baseAddress_ + (0x02*addrSize)));
   addRegister(new Register("Reg03", baseAddress_ + (0x03*addrSize)));
   addRegister(new Register("Reg04", baseAddress_ + (0x04*addrSize)));
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "adcData_"  << dec << i << "_" << j; 
         addRegister(new Register(tmp.str(),baseAddress_ + (0x60 + (8*i)+j)*addrSize));
      }
   }      
   addRegister(new Register("delayOut.rdy",baseAddress_ + (0x7F*addrSize)));
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "delayOut.data_"  << dec << i << "_" << j; 
         addRegister(new Register(tmp.str(),baseAddress_ + (0x80 + (8*i)+j)*addrSize));
      }
   }   
   addRegister(new Register("dmode",baseAddress_ + (0x90*addrSize)));
   addRegister(new Register("debug",baseAddress_ + (0xA0*addrSize)));
   
   // Status Variables   
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "adcData_"  << dec << i << "_" << j;
         v = new Variable(tmp.str(), Variable::Status);
         tmp.str("");
         tmp << dec << "The firmware automatically collects 8 consecutive samples after each 1 second timeout. This status variable is for ADC channel#" << i << ", sample#"<< j;
         v->setDescription(tmp.str());
         v->setRange(0,0xFFFF); 
         addVariable(v); 
      }
   }         
   v = new Variable("delayOut.rdy", Variable::Status); v->setDescription(" "); v->setTrueFalse();     addVariable(v);  
   
   // Configuration Variables
   v = new Variable("SoftRest", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("RESET REGISTER, Address = 0x00, data[7]");
   v->setTrueFalse();
   addVariable(v); 
   
   v = new Variable("PwrDwn", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("POWER-DOWN REGISTER, Address = 0x01, data[1:0]");
   vector<string> PwrDwn;
   PwrDwn.resize(4);
   PwrDwn[0]   = "Normal Operation";
   PwrDwn[1]   = "Channel 1 in Normal Operation, Channel 2 in Nap Mode";
   PwrDwn[2]   = "Channel 1 and Channel 2 in Nap Mode";
   PwrDwn[3]   = "Sleep Mode";
   v->setEnums(PwrDwn); 
   addVariable(v);

   v = new Variable("ClkInv", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("TIMING REGISTER, Address = 0x02, data[3]");
   vector<string> ClkInv;
   ClkInv.resize(2);
   ClkInv[0]   = "Normal CLKOUT Polarity";
   ClkInv[1]   = "Inverted CLKOUT Polarity";
   v->setEnums(ClkInv); 
   addVariable(v);

   v = new Variable("ClkPhase", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("TIMING REGISTER, Address = 0x02, data[2:1]");
   vector<string> ClkPhase;
   ClkPhase.resize(4);
   ClkPhase[0]   = "No CLKOUT Delay";
   ClkPhase[1]   = "Delayed by 45 degrees";
   ClkPhase[2]   = "Delayed by 90 degrees";
   ClkPhase[3]   = "Delayed by 135 degrees";
   v->setEnums(ClkPhase); 
   addVariable(v); 

   v = new Variable("Dcs", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("TIMING REGISTER, Address = 0x02, data[0]");
   vector<string> Dcs;
   Dcs.resize(2);
   Dcs[0]   = "Clock Duty Cycle Stabilizer Off";
   Dcs[1]   = "Clock Duty Cycle Stabilizer On";
   v->setEnums(Dcs); 
   addVariable(v);  

   v = new Variable("ILvds", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("OUTPUT MODE REGISTER, Address = 0x03, data[6:4]");
   vector<string> ILvds;
   ILvds.resize(8);
   ILvds[0]   = "3.5mA LVDS Output Driver Current";
   ILvds[1]   = "4.0mA LVDS Output Driver Current";
   ILvds[2]   = "4.5mA LVDS Output Driver Current";
   ILvds[3]   = "Not Used";
   ILvds[4]   = "3.0mA LVDS Output Driver Current";
   ILvds[5]   = "2.5mA LVDS Output Driver Current";
   ILvds[6]   = "2.1mA LVDS Output Driver Current";
   ILvds[7]   = "1.75mA LVDS Output Driver Current";
   v->setEnums(ILvds); 
   addVariable(v);  

   v = new Variable("TermOn", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("OUTPUT MODE REGISTER, Address = 0x03, data[3]");
   vector<string> TermOn;
   TermOn.resize(2);
   TermOn[0]   = "Internal Termination Off";
   TermOn[1]   = "Internal Termination On";
   v->setEnums(TermOn); 
   addVariable(v);     
   
   v = new Variable("OutOff", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("OUTPUT MODE REGISTER, Address = 0x03, data[2]");
   vector<string> OutOff;
   OutOff.resize(2);
   OutOff[0]   = "Digital Outputs Are Enabled";
   OutOff[1]   = "Digital Outputs Are Disabled and Have High Output Impedance";
   v->setEnums(OutOff); 
   addVariable(v);     

   v = new Variable("OutMode", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("OUTPUT MODE REGISTER, Address = 0x03, data[2]");
   vector<string> OutMode;
   OutMode.resize(4);
   OutMode[0]   = "Full-Rate CMOS Output Mode";
   OutMode[1]   = "Double Data Rate LVDS Output Mode";
   OutMode[2]   = "Double Data Rate CMOS Output Mode";
   OutMode[3]   = "Not Used";
   v->setEnums(OutMode); 
   addVariable(v);  

   v = new Variable("OutTest", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("DATA FORMAT REGISTER, Address = 0x04, data[5:3]");
   vector<string> OutTest;
   OutTest.resize(8);
   OutTest[0]   = "Digital Output Test Patterns Off";
   OutTest[1]   = "All Digital Outputs = 0";
   OutTest[2]   = "Not Used";
   OutTest[3]   = "All Digital Outputs = 1";
   OutTest[4]   = "Not Used";
   OutTest[5]   = "Checkerboard Output Pattern";
   OutTest[6]   = "Not Used";
   OutTest[7]   = "Alternating Output Pattern";
   v->setEnums(OutTest); 
   addVariable(v); 

   v = new Variable("Abp", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("DATA FORMAT REGISTER, Address = 0x04, data[5:3]");
   vector<string> Abp;
   Abp.resize(2);
   Abp[0]   = "Alternate Bit Polarity Mode Off";
   Abp[1]   = "Alternate Bit Polarity Mode On";
   v->setEnums(Abp); 
   addVariable(v); 

   v = new Variable("Rand", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("DATA FORMAT REGISTER, Address = 0x04, data[1]");
   vector<string> Rand;
   Rand.resize(2);
   Rand[0]   = "Data Output Randomizer Mode Off";
   Rand[1]   = "Data Output Randomizer Mode On";
   v->setEnums(Rand); 
   addVariable(v);   

   v = new Variable("TwoComp", Variable::Configuration);
   v->setPerInstance(true);
   v->setDescription("DATA FORMAT REGISTER, Address = 0x04, data[0]");
   vector<string> TwoComp;
   TwoComp.resize(2);
   TwoComp[0]   = "Offset Binary Data Format";
   TwoComp[1]   = "Two’s Complement Data Format";
   v->setEnums(TwoComp); 
   addVariable(v);     
   
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "delayOut.data_"  << dec << i << "_" << j; 
         v = new Variable(tmp.str(), Variable::Configuration);
         v->setDescription(" ");
         v->setRange(0,0x1F);
         v->setPerInstance(true);
         addVariable(v);          
      }
   }    
   
   v = new Variable("dmode", Variable::Configuration);
   v->setDescription(" ");
   v->setRange(0,0x3);
   v->setPerInstance(true);
   addVariable(v); 
   
   v = new Variable("debug", Variable::Configuration);
   v->setDescription(" ");
   v->setTrueFalse();
   v->setPerInstance(true);
   addVariable(v);  
   
   // Command Variables       
   c = new Command("cntRst");       c->setDescription(" "); addCommand(c);   
}

// Deconstructor
AxiLtc2270::~AxiLtc2270 ( ) { }

// Method to process a command
void AxiLtc2270::command ( string name, string arg ) {
   asm("nop");//no operation
}

// Method to read status registers and update variables
void AxiLtc2270::readStatus ( ) {
   stringstream tmp;
   uint32_t i,j;
   
   REGISTER_LOCK
   
   // Read registers
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "adcData_"  << dec << i << "_" << j;
         regReadInt(tmp.str(),0, 0xFFFF); 
      }
   }        
   regReadInt("delayOut.rdy",0, 0x01);  
   
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void AxiLtc2270::readConfig ( ) {
   stringstream tmp;
   uint32_t i,j;
   
   REGISTER_LOCK

   // Read registers
   regReadInt("Reg00","SoftRest",7,0x1); 
   
   regReadInt("Reg01","PwrDwn",0,0x3); 
   
   regReadInt("Reg02","ClkInv",3,0x1); 
   regReadInt("Reg02","ClkPhase",1,0x3); 
   regReadInt("Reg02","Dcs",0,0x1); 
   
   regReadInt("Reg03","ILvds",4,0x7); 
   regReadInt("Reg03","TermOn",3,0x1); 
   regReadInt("Reg03","OutOff",2,0x1); 
   regReadInt("Reg03","OutMode",0,0x3); 
 
   regReadInt("Reg04","OutTest",3,0x7); 
   regReadInt("Reg04","Abp",2,0x1); 
   regReadInt("Reg04","Rand",1,0x1); 
   regReadInt("Reg04","TwoComp",0,0x1);       
   
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "delayOut.data_"  << dec << i << "_" << j; 
         regReadInt(tmp.str(),0,0x1F);        
      }
   }
   regReadInt("dmode",0,0x3);
   regReadInt("debug",0,0x1);      
   
   REGISTER_UNLOCK
}

// Method to write configuration registers
void AxiLtc2270::writeConfig ( bool force ) {
   stringstream tmp;
   uint32_t i,j;  
  
   REGISTER_LOCK

   // Write registers   
   regWrite("Reg00","SoftRest",force,7,0x1);
   
   regWrite("Reg01","PwrDwn",force,0,0x3);
   
   regWrite("Reg02","ClkInv",force,3,0x1);
   regWrite("Reg02","ClkPhase",force,1,0x3);
   regWrite("Reg02","Dcs",force,0,0x1);
   
   regWrite("Reg03","ILvds",force,4,0x7);
   regWrite("Reg03","TermOn",force,3,0x1);
   regWrite("Reg03","OutOff",force,2,0x1); 
   regWrite("Reg03","OutMode",force,0,0x3);
 
   regWrite("Reg04","OutTest",force,3,0x7);
   regWrite("Reg04","Abp",force,2,0x1);
   regWrite("Reg04","Rand",force,1,0x1);
   regWrite("Reg04","TwoComp",force,0,0x1);       
   
   for (i=0;i<2;i++) {
      for (j=0;j<8;j++) {
         tmp.str(""); 
         tmp << "delayOut.data_"  << dec << i << "_" << j; 
         regWrite(tmp.str(),force,0,0x1F);        
      }
   }
   regWrite("dmode",force,0,0x3);
   regWrite("debug",force,0,0x1);    
  
   REGISTER_UNLOCK   
}
