//-----------------------------------------------------------------------------
// File          : AxiAd9467.cpp
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

#include <AxiAd9467.h>
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
AxiAd9467::AxiAd9467 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) :
                        Device(linkConfig,baseAddress,"AxiAd9467",index,parent) {                        
   Variable * v;
   stringstream tmp;
   uint32_t i;
   
   // Description
   desc_ = "AxiAd9467 Controller object.";
   
   ///////////////////////////////////////////////////////// 
   //                   SPI Registers                     //
   ///////////////////////////////////////////////////////// 
   addRegister(new Register("Reg000", (baseAddress_+(0x00*addrSize))));
   addRegister(new Register("Reg001", (baseAddress_+(0x01*addrSize))));
   addRegister(new Register("Reg002", (baseAddress_+(0x02*addrSize))));
   addRegister(new Register("Reg0FF", (baseAddress_+(0x03*addrSize))));
   addRegister(new Register("Reg008", (baseAddress_+(0x04*addrSize))));
   addRegister(new Register("Reg00D", (baseAddress_+(0x05*addrSize))));
   addRegister(new Register("Reg00F", (baseAddress_+(0x06*addrSize))));
   addRegister(new Register("Reg010", (baseAddress_+(0x07*addrSize))));
   addRegister(new Register("Reg014", (baseAddress_+(0x08*addrSize))));
   addRegister(new Register("Reg015", (baseAddress_+(0x09*addrSize))));
   addRegister(new Register("Reg016", (baseAddress_+(0x0A*addrSize))));
   addRegister(new Register("Reg018", (baseAddress_+(0x0B*addrSize))));
   addRegister(new Register("Reg02C", (baseAddress_+(0x0C*addrSize))));
   addRegister(new Register("Reg036", (baseAddress_+(0x0D*addrSize))));
   addRegister(new Register("Reg107", (baseAddress_+(0x0E*addrSize))));

   addVariable(new Variable("SoftRest", Variable::Configuration));
   getVariable("SoftRest")->setPerInstance(true);      
   getVariable("SoftRest")->setTrueFalse();  
   getVariable("SoftRest")->setDescription("Address = 0x000, data[5]");
   
   addVariable(new Variable("ChipId", Variable::Status));    
   getVariable("ChipId")->setDescription(
      "\tAddress = 0x001, data[7:0]\n"
      "Default is unique chip ID, different for each device. This is a read-only register.");  

   addVariable(new Variable("ChipGrade", Variable::Status));    
   getVariable("ChipGrade")->setDescription(
      "\tAddress = 0x002, data[6:4]\n"
      "Child ID used to differentiate graded devices.");     
      
   addVariable(new Variable("PowerDown", Variable::Configuration));   
   getVariable("PowerDown")->setPerInstance(true);  
   getVariable("PowerDown")->setTrueFalse();    
   getVariable("PowerDown")->setDescription(
      "Address = 0x008, data[0]"); 

   addVariable(new Variable("ResetPnLong", Variable::Configuration));   
   getVariable("ResetPnLong")->setPerInstance(true);  
   getVariable("ResetPnLong")->setTrueFalse();    
   getVariable("ResetPnLong")->setDescription(
      "Address = 0x00D, data[5]"); 
      
   addVariable(new Variable("ResetPnShort", Variable::Configuration));   
   getVariable("ResetPnShort")->setPerInstance(true);  
   getVariable("ResetPnShort")->setTrueFalse();    
   getVariable("ResetPnShort")->setDescription(
      "Address = 0x00D, data[4]");       
      
   addVariable(new Variable("TestPattern", Variable::Configuration));   
   getVariable("TestPattern")->setPerInstance(true);  
   getVariable("TestPattern")->setDescription(
      "\tAddress = 0x00D, data[3:0]\n"
      "When this register is set, the test data is "
      "placed on the output pins in place of normal data.");     
   vector<string> TestPattern;
   TestPattern.resize(8);
   TestPattern[0]   = "Off";
   TestPattern[1]   = "MidScaleShort";
   TestPattern[2]   = "+FullScaleShort";
   TestPattern[3]   = "-FullScaleShort";
   TestPattern[4]   = "CheckerBoard";
   TestPattern[5]   = "PnSeqLong";
   TestPattern[6]   = "PnSeqShort";
   TestPattern[7]   = "OneZeroWordToggle";
   getVariable("TestPattern")->setEnums(TestPattern);        

   addVariable(new Variable("EnableIntRef", Variable::Configuration));   
   getVariable("EnableIntRef")->setPerInstance(true);  
   getVariable("EnableIntRef")->setTrueFalse();     
   getVariable("EnableIntRef")->setDescription(
      "Address = 0x00F, data[7]");

   addVariable(new Variable("AnalogDisconnect", Variable::Configuration));   
   getVariable("AnalogDisconnect")->setPerInstance(true);  
   getVariable("AnalogDisconnect")->setTrueFalse();    
   getVariable("AnalogDisconnect")->setDescription(
      "Address = 0x00F, data[2]"); 

   addVariable(new Variable("OffsetAdj", Variable::Configuration));   
   getVariable("OffsetAdj")->setPerInstance(true);  
   getVariable("OffsetAdj")->setRange(0,0xFF);   
   getVariable("OffsetAdj")->setDescription(
      "Address = 0x010, data[7:0]");       

   addVariable(new Variable("OutputDisable", Variable::Configuration));   
   getVariable("OutputDisable")->setPerInstance(true);  
   getVariable("OutputDisable")->setTrueFalse();    
   getVariable("OutputDisable")->setDescription(
      "Address = 0x014, data[4]"); 
      
   addVariable(new Variable("OutputInvert", Variable::Configuration));   
   getVariable("OutputInvert")->setPerInstance(true);  
   getVariable("OutputInvert")->setTrueFalse();     
   getVariable("OutputInvert")->setDescription(
      "Address = 0x014, data[2]");    

   addVariable(new Variable("DataFormat", Variable::Configuration));   
   getVariable("DataFormat")->setPerInstance(true);  
   getVariable("DataFormat")->setDescription(
      "Address = 0x014, data[1:0]");    
   vector<string> DataFormat;
   DataFormat.resize(3);
   DataFormat[0]   = "OffsetBinary";
   DataFormat[1]   = "TwosComplement";
   DataFormat[2]   = "GrayCode";
   getVariable("DataFormat")->setEnums(DataFormat);  

   addVariable(new Variable("LvdsCoarseAdj", Variable::Configuration));   
   getVariable("LvdsCoarseAdj")->setPerInstance(true);  
   getVariable("LvdsCoarseAdj")->setDescription(
      "Address = 0x015, data[3]");    
   vector<string> LvdsCoarseAdj;
   LvdsCoarseAdj.resize(2);
   LvdsCoarseAdj[0]   = "3.00mA";
   LvdsCoarseAdj[1]   = "1.71mA";
   getVariable("LvdsCoarseAdj")->setEnums(LvdsCoarseAdj);  

   addVariable(new Variable("LvdsFineAdj", Variable::Configuration));   
   getVariable("LvdsFineAdj")->setPerInstance(true);  
   getVariable("LvdsFineAdj")->setDescription(
      "Address = 0x015, data[2:0]");    
   vector<string> LvdsFineAdj;
   LvdsFineAdj.resize(8);
   LvdsFineAdj[0]   = "undefined";
   LvdsFineAdj[1]   = "3.00mA";
   LvdsFineAdj[2]   = "2.79mA";
   LvdsFineAdj[3]   = "2.57mA";
   LvdsFineAdj[4]   = "2.35mA";
   LvdsFineAdj[5]   = "2.14mA";
   LvdsFineAdj[6]   = "1.93mA";
   LvdsFineAdj[7]   = "1.71mA";
   getVariable("LvdsFineAdj")->setEnums(LvdsFineAdj);     

   addVariable(new Variable("DcoInvert", Variable::Configuration));   
   getVariable("DcoInvert")->setPerInstance(true);  
   getVariable("DcoInvert")->setTrueFalse();   
   getVariable("DcoInvert")->setDescription(
      "\tAddress = 0x016, data[7]\n"
      "Determines digital clock output phase.");     
      
   addVariable(new Variable("InputRange", Variable::Configuration));   
   getVariable("InputRange")->setPerInstance(true);  
   getVariable("InputRange")->setDescription(
      "Address = 0x018, data[3:0]");    
   vector<string> InputRange;
   InputRange.resize(11);
   InputRange[0]   = "2.0Vpp";
   InputRange[1]   = "undefined0";
   InputRange[2]   = "undefined1";
   InputRange[3]   = "undefined2";
   InputRange[4]   = "undefined3";
   InputRange[5]   = "undefined4";
   InputRange[6]   = "2.1Vpp";
   InputRange[7]   = "2.2Vpp";
   InputRange[8]   = "2.3Vpp";
   InputRange[9]   = "2.4Vpp";
   InputRange[10]  = "2.5Vpp";
   getVariable("InputRange")->setEnums(InputRange);   

   addVariable(new Variable("InputCoupling", Variable::Configuration));   
   getVariable("InputCoupling")->setPerInstance(true);  
   getVariable("InputCoupling")->setDescription(
      "Address = 0x02C, data[2]");    
   vector<string> InputCoupling;
   InputCoupling.resize(2);
   InputCoupling[0]   = "AcCoupled";
   InputCoupling[1]   = "DcCoupled";
   getVariable("InputCoupling")->setEnums(InputCoupling);    
   
   addVariable(new Variable("BufferCurrentSelect1", Variable::Configuration));   
   getVariable("BufferCurrentSelect1")->setPerInstance(true);  
   getVariable("BufferCurrentSelect1")->setRange(0,0xFF);   
   getVariable("BufferCurrentSelect1")->setDescription(
      "Address = 0x036, data[7:0]");  

   addVariable(new Variable("BufferCurrentSelect2", Variable::Configuration));   
   getVariable("BufferCurrentSelect2")->setPerInstance(true);  
   getVariable("BufferCurrentSelect2")->setRange(0,0xFF);   
   getVariable("BufferCurrentSelect2")->setDescription(
      "Address = 0x107, data[7:0]");        

   getVariable("Enabled")->setHidden(true);   
   
   ///////////////////////////////////////////////////////// 
   //              Delay and System Registers             //
   ///////////////////////////////////////////////////////// 
   for (i=0;i<8;i++) {
      tmp.str(""); 
      tmp << "idelay_data_"  << dec << i; 
      addRegister(new Register(tmp.str(), (baseAddress_+(0x10+i)*addrSize)));
      v = new Variable(tmp.str(), Variable::Configuration);
      v->setDescription(" ");
      v->setRange(0,0x1F);
      v->setPerInstance(true);
      addVariable(v);          
   } 
   
   addRegister(new Register("pllLocked", (baseAddress_+(0x1D*addrSize))));
   v = new Variable("pllLocked", Variable::Status);
   v->setTrueFalse();
   addVariable(v); 
   
   addRegister(new Register("idelay_rdy", (baseAddress_+(0x1E*addrSize))));
   v = new Variable("idelay_rdy", Variable::Status);
   v->setTrueFalse();
   addVariable(v);

   addRegister(new Register("idelay_dmux", (baseAddress_+(0x1F*addrSize))));
   v = new Variable("idelay_dmux", Variable::Configuration);
   v->setPerInstance(true);
   v->setTrueFalse();
   addVariable(v);    

   for (i=0;i<16;i++) {
      tmp.str(""); 
      tmp << "adcDataMon_"  << dec << setw(2) << setfill('0') << i;
      addRegister(new Register(tmp.str(), (baseAddress_+(0x20+i)*addrSize)));
      v = new Variable(tmp.str(), Variable::Status);
      v->setRange(0,0xFFFF);
      addVariable(v);          
   }      
}

// Deconstructor
AxiAd9467::~AxiAd9467 ( ) { }

// Method to process a command
void AxiAd9467::command ( string name, string arg ) {
   asm("nop");//no operation
}

// Method to read status registers and update variables
void AxiAd9467::readStatus ( ) {
   stringstream tmp;
   uint32_t i;
   
   REGISTER_LOCK
   
   // Read registers
   regReadInt("Reg001","ChipId",0,0xFF);     
   
   regReadInt("Reg002","ChipGrade",4,0x7);  
   
   regReadInt("pllLocked",0,0x1);    
   
   regReadInt("idelay_rdy",0,0x1);    

   for (i=0;i<16;i++) {
      tmp.str(""); 
      tmp << "adcDataMon_"  << dec << setw(2) << setfill('0') << i;
      regReadInt(tmp.str(),0,0xFFFF);          
   }   
   
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void AxiAd9467::readConfig ( ) {
   stringstream tmp;
   uint32_t i;
   
   REGISTER_LOCK

   // Read registers
   regReadInt("Reg000","SoftRest",5,0x1);     
   
   regReadInt("Reg008","PowerDown",0,0x1);     

   regReadInt("Reg00D","ResetPnLong",5,0x1);     
   regReadInt("Reg00D","ResetPnShort",4,0x1);     
   regReadInt("Reg00D","TestPattern",0,0x7);     

   regReadInt("Reg00F","EnableIntRef",7,0x1);     
   regReadInt("Reg00F","AnalogDisconnect",2,0x1);     

   regReadInt("Reg010","OffsetAdj",0,0xFF);     
   
   regReadInt("Reg014","OutputDisable",4,0x1);     
   regReadInt("Reg014","OutputInvert",2,0x1);     
   regReadInt("Reg014","DataFormat",0,0x3);     
  
   regReadInt("Reg015","LvdsCoarseAdj",3,0x1);     
   regReadInt("Reg015","LvdsFineAdj",0,0x7);     

   regReadInt("Reg016","DcoInvert",7,0x1);     

   regReadInt("Reg018","InputRange",0,0xF);     

   regReadInt("Reg02C","InputCoupling",2,0x1);     
   
   regReadInt("Reg036","BufferCurrentSelect1",2,0x3F);     
   
   regReadInt("Reg107","BufferCurrentSelect2",2,0x3F); 

   for (i=0;i<8;i++) {
      tmp.str(""); 
      tmp << "idelay_data_"  << dec << i; 
      regReadInt(tmp.str(),0,0x1F);    
   }    
   
   regReadInt("idelay_dmux",0,0x1);   
   
   REGISTER_UNLOCK
}

// Method to write configuration registers
void AxiAd9467::writeConfig ( bool force ) {
   stringstream tmp;
   uint32_t i;  
  
   REGISTER_LOCK

   // Write registers     
   getRegister("Reg000")->set(getVariable("SoftRest")->getInt(),5,0x1);   
   getRegister("Reg000")->set(0x1,4,0x1);//Reg000 expects BIT4 = '1'
   getRegister("Reg000")->set(0x1,3,0x1);//Reg000 expects BIT3 = '1'
   writeRegister(getRegister("Reg000"),force,true);
 
   regWrite("Reg008","PowerDown",force,0,0x1); 
   
   regWrite("Reg00D","ResetPnLong",force,5,0x1); 
   regWrite("Reg00D","ResetPnShort",force,4,0x1); 
   regWrite("Reg00D","TestPattern",force,0,0x7); 

   regWrite("Reg00F","EnableIntRef",force,7,0x1); 
   regWrite("Reg00F","AnalogDisconnect",force,2,0x1); 
   
   regWrite("Reg010","OffsetAdj",force,0,0xFF); 

   getRegister("Reg014")->set(getVariable("OutputDisable")->getInt(),4,0x1); 
   getRegister("Reg014")->set(0x1,3,0x1);//Reg014 expects BIT3 = '1'
   getRegister("Reg014")->set(getVariable("OutputInvert")->getInt(),2,0x1); 
   getRegister("Reg014")->set(getVariable("DataFormat")->getInt(),0,0x3);    
   writeRegister(getRegister("Reg014"),force,true);
   
   regWrite("Reg015","LvdsCoarseAdj",force,3,0x1); 
   regWrite("Reg015","LvdsFineAdj",force,0,0x7); 

   regWrite("Reg016","DcoInvert",force,7,0x1); 

   regWrite("Reg018","InputRange",force,0,0xF); 

   regWrite("Reg02C","InputCoupling",force,2,0x1); 
   
   getRegister("Reg036")->set(getVariable("BufferCurrentSelect1")->getInt(),2,0x3F); 
   getRegister("Reg036")->set(0x1,1,0x1);//Reg036 expects BIT1 = '1'
   getRegister("Reg036")->set(0x1,0,0x1);//Reg036 expects BIT0 = '0'   
   writeRegister(getRegister("Reg036"),force,true);   
   
   regWrite("Reg107","BufferCurrentSelect2",force,2,0x3F); 
   
   //Update the device
   getRegister("Reg0FF")->set(0x1);
   writeRegister(getRegister("Reg0FF"),true,true);
   
   for (i=0;i<8;i++) {
      tmp.str(""); 
      tmp << "idelay_data_"  << dec << i; 
      regWrite(tmp.str(),force,0,0x1F);    
   }    
   
   regWrite("idelay_dmux",force,0,0x1);    
  
   REGISTER_UNLOCK   
}
