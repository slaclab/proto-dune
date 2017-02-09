//-----------------------------------------------------------------------------
// File          : AxiXadc.cpp
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 09/04/2013
// Project       : Generic 
//-----------------------------------------------------------------------------
// Description :
// DAQ Device Driver for the Xilinx's AxiXadc
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

#include <AxiXadc.h>

#include <Register.h>
#include <Variable.h>
#include <Command.h>

#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

#define ENABLE_AxiXadc_CONFIG false

// Constructor
AxiXadc::AxiXadc ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"AxiXadc",index,parent) {

   stringstream tmp;    
   uint32_t i;   
                        
   // Description
   desc_ = "Xilinx AxiXadc object.";

   ////////////////////////////////////////////////////////////////
   // Registers && Variables
   //////////////////////////////////////////////////////////////// 
   addRegister(new Register("Temperature",   baseAddress_ + (0x200 >> 2)*addrSize));
   addVariable(new Variable("Temperature", Variable::Status));
   getVariable("Temperature")->setDescription(
      "The result of the on-chip temperature sensor measurement is " 
      "stored in this location. The data is MSB justified in the "
      "16-bit register (Read Only).  The 12 MSBs correspond to the "
      "temperature sensor transfer function shown in Figure 2-8, "
      "page 31 of UG480 (v1.2)"); 
   
   addRegister(new Register("VccInt",        baseAddress_ + (0x204 >> 2)*addrSize));
   addVariable(new Variable("VccInt", Variable::Status));
   getVariable("VccInt")->setDescription(
      "The result of the on-chip VccInt supply monitor measurement "
      "is stored at this location. The data is MSB justified in the "
      "16-bit register (Read Only). The 12 MSBs correspond to the "
      "supply sensor transfer function shown in Figure 2-9, "
      "page 32 of UG480 (v1.2)");    
   
   addRegister(new Register("VccAux",        baseAddress_ + (0x208 >> 2)*addrSize));
   addVariable(new Variable("VccAux", Variable::Status));
   getVariable("VccAux")->setDescription(
      "The result of the on-chip VccAux supply monitor measurement "
      "is stored at this location. The data is MSB justified in the "
      "16-bit register (Read Only). The 12 MSBs correspond to the "
      "supply sensor transfer function shown in Figure 2-9, "
      "page 32 of UG480 (v1.2)");      
   
   addRegister(new Register("Vin",           baseAddress_ + (0x20C >> 2)*addrSize));// Vp/Vn
   addVariable(new Variable("Vin", Variable::Status));
   getVariable("Vin")->setDescription(
      "The result of a conversion on the dedicated analog input " 
      "channel is stored in this register. The data is MSB justified "
      "in the 16-bit register (Read Only). The 12 MSBs correspond to the "
      "transfer function shown in Figure 2-5, page 29 or "
      "Figure 2-6, page 29 of UG480 (v1.2) depending on analog input mode "
      "settings.");    
   getVariable("Vin")->setHidden(true);
   
   addRegister(new Register("VrefP",         baseAddress_ + (0x210 >> 2)*addrSize));
   addVariable(new Variable("VrefP", Variable::Status));
   getVariable("VrefP")->setDescription(
      "The result of a conversion on the reference input VrefP is "
      "stored in this register. The 12 MSBs correspond to the ADC "
      "transfer function shown in Figure 2-9  of UG480 (v1.2). The data is MSB "
      "justified in the 16-bit register (Read Only). The supply sensor is used "
      "when measuring VrefP.");   
   getVariable("VrefP")->setHidden(true);      
     
   addRegister(new Register("VrefN",         baseAddress_ + (0x214 >> 2)*addrSize));
   addVariable(new Variable("VrefN", Variable::Status));
   getVariable("VrefN")->setDescription(   
      "The result of a conversion on the reference input VREFN is "
      "stored in this register (Read Only). This channel is measured in bipolar "
      "mode with a 2's complement output coding as shown in "
      "Figure 2-2, page 25. By measuring in bipolar mode, small "
      "positive and negative offset around 0V (VrefN) can be "
      "measured. The supply sensor is also used to measure "
      "VrefN, thus 1 LSB = 3V/4096. The data is MSB justified in "
      "the 16-bit register.");
   getVariable("VrefN")->setHidden(true);       
      
   addRegister(new Register("VccBram",       baseAddress_ + (0x218 >> 2)*addrSize));
   addVariable(new Variable("VccBram", Variable::Status));
   getVariable("VccBram")->setDescription(
      "The result of the on-chip VccBram supply monitor measurement "
      "is stored at this location. The data is MSB justified in the "
      "16-bit register (Read Only). The 12 MSBs correspond to the "
      "supply sensor transfer function shown in Figure 2-9, "
      "page 32 of UG480 (v1.2)");      
   
   //0x21C is undefined
   
   addRegister(new Register("SupplyOffsetA", baseAddress_ + (0x220 >> 2)*addrSize));
   addVariable(new Variable("SupplyOffsetA", Variable::Status));
   getVariable("SupplyOffsetA")->setDescription(
      "The calibration coefficient for the supply sensor offset "
      "using ADC A is stored at this location (Read Only).");  
   getVariable("SupplyOffsetA")->setHidden(true);      
   
   addRegister(new Register("AdcOffsetA",    baseAddress_ + (0x224 >> 2)*addrSize));
   addVariable(new Variable("AdcOffsetA", Variable::Status));
   getVariable("AdcOffsetA")->setDescription(
      "The calibration coefficient for the ADC A offset is stored at "
      "this location (Read Only).");  
   getVariable("AdcOffsetA")->setHidden(true);
   
   addRegister(new Register("AdcGainA",      baseAddress_ + (0x228 >> 2)*addrSize));
   addVariable(new Variable("AdcGainA", Variable::Status));
   getVariable("AdcGainA")->setDescription(
      "The calibration coefficient for the ADC A gain error is "
      "stored at this location (Read Only).");     
   getVariable("AdcGainA")->setHidden(true);
   
   //0x22C:0x230 is undefined
   
   addRegister(new Register("VccpInt",       baseAddress_ + (0x234 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpInt", Variable::Status));
   getVariable("VccpInt")->setDescription(
      "The result of a conversion on the PS supply, VccpInt is "
      "stored in this register. The 12 MSBs correspond to the ADC "
      "transfer function shown in Figure 2-9, page 32 of UG480 (v1.2). The data is "
      "MSB justified in the 16-bit register (Zynq Only and Read Only)."
      "The supply sensor is used when measuring VccpInt.");
   getVariable("VccpInt")->setHidden(true);
   
   addRegister(new Register("VccpAux",       baseAddress_ + (0x238 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpAux", Variable::Status));
   getVariable("VccpAux")->setDescription(
      "The result of a conversion on the PS supply, VccpAux is "
      "stored in this register. The 12 MSBs correspond to the ADC "
      "transfer function shown in Figure 2-9, page 32 of UG480 (v1.2). The data is "
      "MSB justified in the 16-bit register (Zynq Only and Read Only)."
      "The supply sensor is used when measuring VccpAux.");   
   getVariable("VccpAux")->setHidden(true);      
   
   addRegister(new Register("VccpDdr",       baseAddress_ + (0x23C >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpDdr", Variable::Status));
   getVariable("VccpDdr")->setDescription(
      "The result of a conversion on the PS supply, VccpDdr is "
      "stored in this register. The 12 MSBs correspond to the ADC "
      "transfer function shown in Figure 2-9, page 32 of UG480 (v1.2). The data is "
      "MSB justified in the 16-bit register (Zynq Only and Read Only)."
      "The supply sensor is used when measuring VccpDdr."); 
   getVariable("VccpDdr")->setHidden(true);      
   
   for (i=0;i<16;i++)//0x10:0x1F 0x240:0x27C
   {
      tmp.str("");
      tmp << "VinAux" << dec << setw(2) << setfill('0') << i;// VauxP/VauxN[15:0]
      
      addRegister(new Register(tmp.str(), (baseAddress_ + ((i*4 + 0x240) >> 2)*addrSize)));
      addVariable(new Variable(tmp.str(), Variable::Status));
      getVariable(tmp.str())->setDescription(
         "The results of the conversions on auxiliary analog input "
         "channels are stored in this register. The data is MSB "
         "justified in the 16-bit register (Read Only). The 12 MSBs correspond to "
         "the transfer function shown in Figure 2-1, page 24 or "
         "Figure 2-2, page 25 of UG480 (v1.2) depending on analog input mode "
         "settings.");
      getVariable(tmp.str())->setHidden(false); 
   }
   
   addRegister(new Register("TemperatureMax",       baseAddress_ + (0x280 >> 2)*addrSize));
   addVariable(new Variable("TemperatureMax", Variable::Status));
   getVariable("TemperatureMax")->setDescription(
      "Maximum temperature measurement recorded since "
      "power-up or the last AxiXadc reset (Read Only).");      
   
   addRegister(new Register("VccIntMax",     baseAddress_ + (0x284 >> 2)*addrSize));
   addVariable(new Variable("VccIntMax", Variable::Status));
   getVariable("VccIntMax")->setDescription(
      "Maximum VccInt measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");        
   
   addRegister(new Register("VccAuxMax",     baseAddress_ + (0x288 >> 2)*addrSize));
   addVariable(new Variable("VccAuxMax", Variable::Status));
   getVariable("VccAuxMax")->setDescription(
      "Maximum VccAux measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");     
   
   addRegister(new Register("VccBramMax",    baseAddress_ + (0x28C >> 2)*addrSize));
   addVariable(new Variable("VccBramMax", Variable::Status));
   getVariable("VccBramMax")->setDescription(
      "Maximum VccBram measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");    
   
   addRegister(new Register("TemperatureMin",       baseAddress_ + (0x290 >> 2)*addrSize));
   addVariable(new Variable("TemperatureMin", Variable::Status));
   getVariable("TemperatureMin")->setDescription(
      "Minimum temperature measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");      
   
   addRegister(new Register("VccIntMin",     baseAddress_ + (0x294 >> 2)*addrSize));
   addVariable(new Variable("VccIntMin", Variable::Status));
   getVariable("VccIntMin")->setDescription(
      "Minimum VccInt measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");    
   
   addRegister(new Register("VccAuxMin",     baseAddress_ + (0x298 >> 2)*addrSize));
   addVariable(new Variable("VccAuxMin", Variable::Status));
   getVariable("VccAuxMin")->setDescription(
      "Minimum VccAux measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");     
   
   addRegister(new Register("VccBramMin",    baseAddress_ + (0x29C >> 2)*addrSize));  
   addVariable(new Variable("VccBramMin", Variable::Status));
   getVariable("VccBramMin")->setDescription(
      "Minimum VccBram measurement recorded since power-up "
      "or the last AxiXadc reset (Read Only).");     
   
   addRegister(new Register("VccpIntMax",    baseAddress_ + (0x2A0 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpIntMax", Variable::Status));
   getVariable("VccpIntMax")->setDescription(
      "Maximum VccpInt measurement recorded since power-up "
      "or the last AxiXadc reset (Zynq Only and Read Only)."); 
   getVariable("VccpIntMax")->setHidden(true);       
   
   addRegister(new Register("VccpAuxMax",    baseAddress_ + (0x2A4 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpAuxMax", Variable::Status));
   getVariable("VccpAuxMax")->setDescription(
      "Maximum VccpAux measurement recorded since power-up "
      "or the last AxiXadc reset (Zynq Only and Read Only).");    
   getVariable("VccpAuxMax")->setHidden(true);
   
   addRegister(new Register("VccpDdrMax",    baseAddress_ + (0x2A8 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpDdrMax", Variable::Status));
   getVariable("VccpDdrMax")->setDescription(
      "Maximum VccpDdr measurement recorded since power-up "
      "or the last AxiXadc reset (Zynq Only and Read Only)."); 
   getVariable("VccpDdrMax")->setHidden(true);      
   
   //0x2AC is undefined
   
   addRegister(new Register("VccpIntMin",    baseAddress_ + (0x2B0 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpIntMin", Variable::Status));
   getVariable("VccpIntMin")->setDescription(
      "Minimum VccpInt measurement recorded since power-up "
      "or the last AxiXadc reset (Zynq Only and Read Only)."); 
   getVariable("VccpIntMin")->setHidden(true);      
   
   addRegister(new Register("VccpAuxMin",    baseAddress_ + (0x2B4 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpAuxMin", Variable::Status));
   getVariable("VccpAuxMin")->setDescription(
      "Minimum VccpAux measurement recorded since power-up "
      "or the last AxiXadc reset (Zynq Only and Read Only).");  
   getVariable("VccpAuxMin")->setHidden(true);       
   
   addRegister(new Register("VccpDdrMin",    baseAddress_ + (0x2B8 >> 2)*addrSize));//Zynq Specific Register
   addVariable(new Variable("VccpDdrMin", Variable::Status));
   getVariable("VccpDdrMin")->setDescription(
      "Minimum VccpDdr measurement recorded since power-up "
      "or the last AxiXadc reset (Zynq Only and Read Only).");
   getVariable("VccpDdrMin")->setHidden(true);       
   
   //0x2BC:2F8 is undefined    
   //Not sure what these are, nothing about them in documentation
//    addRegister(new Register("SupplyOffsetB", baseAddress_ + 0x30));
//    addVariable(new Variable("SupplyOffsetB", Variable::Status));
//    getVariable("SupplyOffsetB")->setDescription(
//       "The calibration coefficient for the supply sensor offset "
//       "using ADC B is stored at this location (Read Only).");    
//    getVariable("SupplyOffsetB")->setHidden(true);      
   
//    addRegister(new Register("AdcOffsetB",    baseAddress_ + 0x31));
//    addVariable(new Variable("AdcOffsetB", Variable::Status));
//    getVariable("AdcOffsetB")->setDescription(
//       "The calibration coefficient for the ADC B offset is stored at "
//       "this location (Read Only).");     
//    getVariable("AdcOffsetB")->setHidden(true);       
   
//    addRegister(new Register("AdcGainB",      baseAddress_ + 0x32));   
//    addVariable(new Variable("AdcGainB", Variable::Status));
//    getVariable("AdcGainB")->setDescription(
//       "The calibration coefficient for the ADC B gain error is "
//       "stored at this location (Read Only)."); 
//    getVariable("AdcGainB")->setHidden(true);  
   
   
   addRegister(new Register("Flag",          baseAddress_ + (0x2FC >> 2)*addrSize)); 
   addVariable(new Variable("Flag", Variable::Status));
   getVariable("Flag")->setDescription(
      "This register contains general status information (Read Only). "
      "Flag Register Bits are defined in Figure 3-2 and Table 3-2 on "
      "page 37 of UG480 (v1.2)");
   getVariable("Flag")->setHidden(true);      
      
   addVariable(new Variable("Flag.JTGD", Variable::Status));//0x3F.BIT11
   getVariable("Flag.JTGD")->setRange(0,0x1);   
   getVariable("Flag.JTGD")->setDescription(
      "A logic 1 indicates that the JTAG_AxiXadc BitGen option has "
      "been used to disable all JTAG access. See DRP JTAG Interface "
      "for more information."); 
   getVariable("Flag.JTGD")->setHidden(true);      

   addVariable(new Variable("Flag.JTGR", Variable::Status));//0x3F.BIT10
   getVariable("Flag.JTGR")->setRange(0,0x1);   
   getVariable("Flag.JTGR")->setDescription(
      "A logic 1 indicates that the JTAG_AxiXadc BitGen option has "
      "been used to restrict JTAG access to read only. See DRP JTAG "
      "Interface for more information.");
   getVariable("Flag.JTGR")->setHidden(true);      

   addVariable(new Variable("Flag.REF", Variable::Status));//0x3F.BIT9
   getVariable("Flag.REF")->setRange(0,0x1);   
   getVariable("Flag.REF")->setDescription(
      "When this bit is a logic 1, the ADC is using the internal "
      "voltage reference. When this bit is a logic 0, the external "
      "reference is being used."); 
   getVariable("Flag.REF")->setHidden(true);       

   addVariable(new Variable("Flag.VccpDdr", Variable::Status));//0x3F.BIT7
   getVariable("Flag.VccpDdr")->setRange(0,0x1);   
   getVariable("Flag.VccpDdr")->setDescription(
      "When this bit is a logic 1, "
      "VccpDdr tripped on either VccpDdrMin or VccpDdrMax the ADC is using the internal.");  
   getVariable("Flag.VccpDdr")->setHidden(true); 

   addVariable(new Variable("Flag.VccpAux", Variable::Status));//0x3F.BIT6
   getVariable("Flag.VccpAux")->setRange(0,0x1);   
   getVariable("Flag.VccpAux")->setDescription(
      "When this bit is a logic 1, "
      "VccpAux tripped on either VccpAuxMin or VccpAuxMax the ADC is using the internal."); 
   getVariable("Flag.VccpAux")->setHidden(true);
      
   addVariable(new Variable("Flag.VccpInt", Variable::Status));//0x3F.BIT5
   getVariable("Flag.VccpInt")->setRange(0,0x1);   
   getVariable("Flag.VccpInt")->setDescription(
      "When this bit is a logic 1, "
      "VccpInt tripped on either VccpIntMin or VccpIntMax the ADC is using the internal."); 
   getVariable("Flag.VccpInt")->setHidden(true);   
   
   addVariable(new Variable("Flag.VccBram", Variable::Status));//0x3F.BIT4
   getVariable("Flag.VccBram")->setRange(0,0x1);   
   getVariable("Flag.VccBram")->setDescription(
      "When this bit is a logic 1, "
      "VccBram tripped on either VccBramMin or VccBramMax the ADC is using the internal."); 
   getVariable("Flag.VccBram")->setHidden(true);

   addVariable(new Variable("Flag.OT", Variable::Status));//0x3F.BIT3
   getVariable("Flag.OT")->setRange(0,0x1);   
   getVariable("Flag.OT")->setDescription(
      "This bit reflects the status of the Over Temperature logic output."); 
   getVariable("Flag.OT")->setHidden(true);      

   addVariable(new Variable("Flag.VccAux", Variable::Status));//0x3F.BIT2
   getVariable("Flag.VccAux")->setRange(0,0x1);   
   getVariable("Flag.VccAux")->setDescription(
      "When this bit is a logic 1, "
      "VccAux tripped on either VccAuxMin or VccAuxMax the ADC is using the internal.");   
   getVariable("Flag.VccAux")->setHidden(true); 
      
   addVariable(new Variable("Flag.VccInt", Variable::Status));//0x3F.BIT1
   getVariable("Flag.VccInt")->setRange(0,0x1);   
   getVariable("Flag.VccInt")->setDescription(
      "When this bit is a logic 1, "
      "VccInt tripped on either VccIntMin or VccIntMax the ADC is using the internal.");   
   getVariable("Flag.VccInt")->setHidden(true); 
      
   addVariable(new Variable("Flag.Temperature", Variable::Status));//0x3F.BIT0
   getVariable("Flag.Temperature")->setRange(0,0x1);   
   getVariable("Flag.Temperature")->setDescription(
      "When this bit is a logic 1, "
      "Temperature tripped on either TemperatureMin or TemperatureMax the ADC is using the internal.");     
   getVariable("Flag.Temperature")->setHidden(true); 
  
#if ENABLE_AxiXadc_CONFIG  
   for (i=0;i<3;i++)//0x300:0x308  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x40);// Configuration Registers[2:0]

      addRegister(new Register(tmp.str(), (baseAddress_ + ((i*4 + 0x300) >> 2)*addrSize)));
      addVariable(new Variable(tmp.str(), Variable::Configuration));
      getVariable(tmp.str())->setPerInstance(true);
      getVariable(tmp.str())->setRange(4,0xFFFF);         
      getVariable(tmp.str())->setDescription(
         "These are AxiXadc configuration registers (see Configuration "
         "Registers (40hto 42h)) on page 39 of UG480 (v1.2)"); 
      getVariable(tmp.str())->setHidden(true);
   }   

   //0x43:0x47 is Factory Test Registers (reserved)
   
   for (i=0;i<8;i++)//0x320:0x330  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x48);// Sequence Registers[7:0]

      addRegister(new Register(tmp.str(), (baseAddress_ + ((i*4 + 0x320) >> 2)*addrSize)));
      addVariable(new Variable(tmp.str(), Variable::Configuration));
      getVariable(tmp.str())->setPerInstance(true);
      getVariable(tmp.str())->setRange(4,0xFFFF);         
      getVariable(tmp.str())->setDescription(
         "These registers are used to program the channel sequencer "
         "function (see Chapter 4, AxiXadc Operating Modes) of UG480 (v1.2)");  
      getVariable(tmp.str())->setHidden(true);
   }  

   for (i=0;i<16;i++)//0x340:0x37C  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x50);// Alarm Registers[15:0]

      addRegister(new Register(tmp.str(), (baseAddress_ + ((i*4 + 0x340) >> 2)*addrSize));
      addVariable(new Variable(tmp.str(), Variable::Configuration));
      getVariable(tmp.str())->setPerInstance(true);
      getVariable(tmp.str())->setRange(4,0xFFFF);         
      getVariable(tmp.str())->setDescription(
         "These are the alarm threshold registers for the AxiXadc alarm "
         "function (see Automatic Alarms, page 59) of UG480 (v1.2)");  
      getVariable(tmp.str())->setHidden(true);         
   }     

   addVariable(new Variable("ConfigEn", Variable::Configuration));
   getVariable("ConfigEn")->setPerInstance(true);
   getVariable("ConfigEn")->setDescription("Set to 'True' to enable register writes to ADC.");
   getVariable("ConfigEn")->setTrueFalse();
   getVariable("ConfigEn")->set("False");
   getVariable("ConfigEn")->setHidden(true);
   
#endif 

   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
AxiXadc::~AxiXadc ( ) { }

// Method to process a command
void AxiXadc::command ( string name, string arg) {
   asm("nop");//no operation
}

// Method to read status registers and update variables
void AxiXadc::readStatus ( ) {
   stringstream tmp;    
   uint32_t i;   

   REGISTER_LOCK

   // Read registers
   readRegister(getRegister("Temperature"));
   getVariable("Temperature")->set(convMonTemp(getRegister("Temperature")->get(4,0xFFF)));
   
   readRegister(getRegister("VccInt"));
   getVariable("VccInt")->set(convMonPwr(getRegister("VccInt")->get(4,0xFFF))); 

   readRegister(getRegister("VccAux"));
   getVariable("VccAux")->set(convMonPwr(getRegister("VccAux")->get(4,0xFFF)));  

   readRegister(getRegister("Vin"));
   getVariable("Vin")->setInt(getRegister("Vin")->get(4,0xFFF));  

   readRegister(getRegister("VrefP"));
   getVariable("VrefP")->setInt(getRegister("VrefP")->get(4,0xFFF));  

   readRegister(getRegister("VrefN"));
   getVariable("VrefN")->setInt(getRegister("VrefN")->get(4,0xFFF));  

   readRegister(getRegister("VccBram"));
   getVariable("VccBram")->set(convMonPwr(getRegister("VccBram")->get(4,0xFFF)));  

   readRegister(getRegister("SupplyOffsetA"));
   getVariable("SupplyOffsetA")->setInt(getRegister("SupplyOffsetA")->get(4,0xFFF));  
   
   readRegister(getRegister("AdcOffsetA"));
   getVariable("AdcOffsetA")->setInt(getRegister("AdcOffsetA")->get(4,0xFFF));     

   readRegister(getRegister("AdcGainA"));
   getVariable("AdcGainA")->setInt(getRegister("AdcGainA")->get(4,0xFFF));  

   readRegister(getRegister("VccpInt"));
   getVariable("VccpInt")->set(convMonPwr(getRegister("VccpInt")->get(4,0xFFF)));  

   readRegister(getRegister("VccpAux"));
   getVariable("VccpAux")->set(convMonPwr(getRegister("VccpAux")->get(4,0xFFF)));  

   readRegister(getRegister("VccpDdr"));
   getVariable("VccpDdr")->set(convMonPwr(getRegister("VccpDdr")->get(4,0xFFF)));  
   
   for (i=0;i<16;i++)//0x10:0x1F 
   {
      tmp.str("");
      tmp << "VinAux" << dec << setw(2) << setfill('0') << i;// VauxP/VauxN[15:0]
      
      readRegister(getRegister(tmp.str()));
      getVariable(tmp.str())->setInt(getRegister(tmp.str())->get(4,0xFFF));  
   }   
   
   readRegister(getRegister("TemperatureMax"));
   getVariable("TemperatureMax")->set(convMonTemp(getRegister("TemperatureMax")->get(4,0xFFF)));

   readRegister(getRegister("VccIntMax"));
   getVariable("VccIntMax")->set(convMonPwr(getRegister("VccIntMax")->get(4,0xFFF)));  

   readRegister(getRegister("VccAuxMax"));
   getVariable("VccAuxMax")->set(convMonPwr(getRegister("VccAuxMax")->get(4,0xFFF)));     
   
   readRegister(getRegister("VccBramMax"));
   getVariable("VccBramMax")->set(convMonPwr(getRegister("VccBramMax")->get(4,0xFFF))); 

   readRegister(getRegister("TemperatureMin"));
   getVariable("TemperatureMin")->set(convMonTemp(getRegister("TemperatureMin")->get(4,0xFFF))); 

   readRegister(getRegister("VccIntMin"));
   getVariable("VccIntMin")->set(convMonPwr(getRegister("VccIntMin")->get(4,0xFFF)));   
   
   readRegister(getRegister("VccAuxMin"));
   getVariable("VccAuxMin")->set(convMonPwr(getRegister("VccAuxMin")->get(4,0xFFF)));  

   readRegister(getRegister("VccBramMin"));
   getVariable("VccBramMin")->set(convMonPwr(getRegister("VccBramMin")->get(4,0xFFF))); 

   readRegister(getRegister("VccpIntMax"));
   getVariable("VccpIntMax")->set(convMonPwr(getRegister("VccpIntMax")->get(4,0xFFF))); 

   readRegister(getRegister("VccpAuxMax"));
   getVariable("VccpAuxMax")->set(convMonPwr(getRegister("VccpAuxMax")->get(4,0xFFF)));   

   readRegister(getRegister("VccpDdrMax"));
   getVariable("VccpDdrMax")->set(convMonPwr(getRegister("VccpDdrMax")->get(4,0xFFF))); 

   readRegister(getRegister("VccpIntMin"));
   getVariable("VccpIntMin")->set(convMonPwr(getRegister("VccpIntMin")->get(4,0xFFF)));   

   readRegister(getRegister("VccpAuxMin"));
   getVariable("VccpAuxMin")->set(convMonPwr(getRegister("VccpAuxMin")->get(4,0xFFF))); 

   readRegister(getRegister("VccpDdrMin"));
   getVariable("VccpDdrMin")->set(convMonPwr(getRegister("VccpDdrMin")->get(4,0xFFF))); 

//    readRegister(getRegister("SupplyOffsetB"));
//    getVariable("SupplyOffsetB")->setInt(getRegister("SupplyOffsetB")->get(4,0xFFF));  

//    readRegister(getRegister("AdcOffsetB"));
//    getVariable("AdcOffsetB")->setInt(getRegister("AdcOffsetB")->get(4,0xFFF));  

//    readRegister(getRegister("AdcGainB"));
//    getVariable("AdcGainB")->setInt(getRegister("AdcGainB")->get(4,0xFFF));  

   readRegister(getRegister("Flag"));
   getVariable("Flag")->setInt(getRegister("Flag")->get());                   //0x3F  
   getVariable("Flag.JTGD")->setInt(getRegister("Flag")->get(11,0x1));        //0x3F.BIT11
   getVariable("Flag.JTGR")->setInt(getRegister("Flag")->get(10,0x1));        //0x3F.BIT10
   getVariable("Flag.REF")->setInt(getRegister("Flag")->get(9,0x1));          //0x3F.BIT9
   getVariable("Flag.VccpDdr")->setInt(getRegister("Flag")->get(7,0x1));      //0x3F.BIT7
   getVariable("Flag.VccpAux")->setInt(getRegister("Flag")->get(6,0x1));      //0x3F.BIT6
   getVariable("Flag.VccpInt")->setInt(getRegister("Flag")->get(5,0x1));      //0x3F.BIT5
   getVariable("Flag.VccBram")->setInt(getRegister("Flag")->get(4,0x1));      //0x3F.BIT4
   getVariable("Flag.OT")->setInt(getRegister("Flag")->get(3,0x1));           //0x3F.BIT3
   getVariable("Flag.VccAux")->setInt(getRegister("Flag")->get(2,0x1));       //0x3F.BIT2
   getVariable("Flag.VccInt")->setInt(getRegister("Flag")->get(1,0x1));       //0x3F.BIT1
   getVariable("Flag.Temperature")->setInt(getRegister("Flag")->get(0,0x1));  //0x3F.BIT0

   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void AxiXadc::readConfig ( ) {

#if ENABLE_AxiXadc_CONFIG 
   stringstream tmp;    
   uint32_t i; 
   
   REGISTER_LOCK

   for (i=0;i<3;i++)//0x40:0x42  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x40);// Configuration Registers[2:0]    
      
      readRegister(getRegister(tmp.str()));
      getVariable(tmp.str())->setInt(getRegister(tmp.str())->get(4,0xFFFF));  
   }
   
   for (i=0;i<8;i++)//0x48:0x4F  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x48);// Sequence Registers[7:0]
      
      readRegister(getRegister(tmp.str()));
      getVariable(tmp.str())->setInt(getRegister(tmp.str())->get(4,0xFFFF));  
   }   
   
   for (i=0;i<16;i++)//0x50:0x5F  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x50);// Alarm Registers[15:0]
      
      readRegister(getRegister(tmp.str()));
      getVariable(tmp.str())->setInt(getRegister(tmp.str())->get(4,0xFFFF));  
   }    
   
   REGISTER_UNLOCK
#else
   asm("nop");
#endif 
}

// Method to write configuration registers
void AxiXadc::writeConfig ( bool force ) {
#if ENABLE_AxiXadc_CONFIG 
   stringstream tmp;    
   uint32_t i;  

   // Writing is enabled?
   if ( getVariable("ConfigEn")->get() == "False" ) return;

   REGISTER_LOCK

   // Set registers
   for (i=0;i<3;i++)//0x40:0x42  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x40);// Configuration Registers[2:0]    
      
      getRegister(tmp.str())->set(getVariable(tmp.str())->getInt(),4,0xFFFF);
      writeRegister(getRegister(tmp.str()),force,true);
   }
   
   for (i=0;i<8;i++)//0x48:0x4F  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x48);// Sequence Registers[7:0]
      
      getRegister(tmp.str())->set(getVariable(tmp.str())->getInt(),4,0xFFFF);
      writeRegister(getRegister(tmp.str()),force,true);
   }   
   
   for (i=0;i<16;i++)//0x50:0x5F  
   {
      tmp.str("");
      tmp << "INIT_" << hex << uppercase << setw(1) << setfill('0') << (i + 0x50);// Alarm Registers[15:0]
      
      getRegister(tmp.str())->set(getVariable(tmp.str())->getInt(),4,0xFFFF);
      writeRegister(getRegister(tmp.str()),force,true);
   }   
  
   REGISTER_UNLOCK
#else
   asm("nop");
#endif 
}

string AxiXadc::convMonTemp (uint32_t value) {
   stringstream tmp;
   float fpValue = ((float)value)*(503.975/4096.0);
   fpValue -= 273.15;
   
   tmp.str("");
   tmp << dec << fixed << setprecision(1) << fpValue << " degC";
   
   return tmp.str();
}

string AxiXadc::convMonPwr (uint32_t value) {
   stringstream tmp;
   float fpValue = ((float)value)*732e-6;//1 LSB = 732 uV

   tmp.str("");
   tmp << dec << fixed << setprecision(3) << fpValue << " V";
   
   return tmp.str();
}

string AxiXadc::convInput (uint32_t value) {
   stringstream tmp;
   float fpValue = ((float)value)*244e-6;//1 LSB = 244 uV

   tmp.str("");
   tmp << dec << fixed << setprecision(3) << fpValue << " V";
   
   return tmp.str();
}

// Method to read status registers and update variables
void AxiXadc::pollStatus ( ) {
   stringstream tmp;    
   uint32_t i;   

   if ( pollEnable_ ) {

      REGISTER_LOCK

      // Read registers
      readRegister(getRegister("Temperature"));
      getVariable("Temperature")->set(convMonTemp(getRegister("Temperature")->get(4,0xFFF)));
      
      readRegister(getRegister("VccInt"));
      getVariable("VccInt")->set(convMonPwr(getRegister("VccInt")->get(4,0xFFF))); 

      readRegister(getRegister("VccAux"));
      getVariable("VccAux")->set(convMonPwr(getRegister("VccAux")->get(4,0xFFF)));  

      //readRegister(getRegister("Vin"));
      //getVariable("Vin")->setInt(getRegister("Vin")->get(4,0xFFF));  

      //readRegister(getRegister("VrefP"));
      //getVariable("VrefP")->setInt(getRegister("VrefP")->get(4,0xFFF));  

      //readRegister(getRegister("VrefN"));
      //getVariable("VrefN")->setInt(getRegister("VrefN")->get(4,0xFFF));  

      //readRegister(getRegister("VccBram"));
      //getVariable("VccBram")->set(convMonPwr(getRegister("VccBram")->get(4,0xFFF)));  

      //readRegister(getRegister("SupplyOffsetA"));
      //getVariable("SupplyOffsetA")->setInt(getRegister("SupplyOffsetA")->get(4,0xFFF));  
      
      //readRegister(getRegister("AdcOffsetA"));
      //getVariable("AdcOffsetA")->setInt(getRegister("AdcOffsetA")->get(4,0xFFF));     

      //readRegister(getRegister("AdcGainA"));
      //getVariable("AdcGainA")->setInt(getRegister("AdcGainA")->get(4,0xFFF));  

      //readRegister(getRegister("VccpInt"));
      //getVariable("VccpInt")->set(convMonPwr(getRegister("VccpInt")->get(4,0xFFF)));  

      //readRegister(getRegister("VccpAux"));
      //getVariable("VccpAux")->set(convMonPwr(getRegister("VccpAux")->get(4,0xFFF)));  

      //readRegister(getRegister("VccpDdr"));
      //getVariable("VccpDdr")->set(convMonPwr(getRegister("VccpDdr")->get(4,0xFFF)));  
      
      for (i=0;i<16;i++)//0x10:0x1F 
      {
         tmp.str("");
         tmp << "VinAux" << dec << setw(2) << setfill('0') << i;// VauxP/VauxN[15:0]
         
         readRegister(getRegister(tmp.str()));
         getVariable(tmp.str())->setInt(getRegister(tmp.str())->get(4,0xFFF));  
      }   
      
      //readRegister(getRegister("TemperatureMax"));
      //getVariable("TemperatureMax")->set(convMonTemp(getRegister("TemperatureMax")->get(4,0xFFF)));

      //readRegister(getRegister("VccIntMax"));
      //getVariable("VccIntMax")->set(convMonPwr(getRegister("VccIntMax")->get(4,0xFFF)));  

      //readRegister(getRegister("VccAuxMax"));
      //getVariable("VccAuxMax")->set(convMonPwr(getRegister("VccAuxMax")->get(4,0xFFF)));     
      
      //readRegister(getRegister("VccBramMax"));
      //getVariable("VccBramMax")->set(convMonPwr(getRegister("VccBramMax")->get(4,0xFFF))); 

      //readRegister(getRegister("TemperatureMin"));
      //getVariable("TemperatureMin")->set(convMonTemp(getRegister("TemperatureMin")->get(4,0xFFF))); 

      //readRegister(getRegister("VccIntMin"));
      //getVariable("VccIntMin")->set(convMonPwr(getRegister("VccIntMin")->get(4,0xFFF)));   
      
      //readRegister(getRegister("VccAuxMin"));
      //getVariable("VccAuxMin")->set(convMonPwr(getRegister("VccAuxMin")->get(4,0xFFF)));  

      //readRegister(getRegister("VccBramMin"));
      //getVariable("VccBramMin")->set(convMonPwr(getRegister("VccBramMin")->get(4,0xFFF))); 

      //readRegister(getRegister("VccpIntMax"));
      //getVariable("VccpIntMax")->set(convMonPwr(getRegister("VccpIntMax")->get(4,0xFFF))); 

      //readRegister(getRegister("VccpAuxMax"));
      //getVariable("VccpAuxMax")->set(convMonPwr(getRegister("VccpAuxMax")->get(4,0xFFF)));   

      //readRegister(getRegister("VccpDdrMax"));
      //getVariable("VccpDdrMax")->set(convMonPwr(getRegister("VccpDdrMax")->get(4,0xFFF))); 

      //readRegister(getRegister("VccpIntMin"));
      //getVariable("VccpIntMin")->set(convMonPwr(getRegister("VccpIntMin")->get(4,0xFFF)));   

      //readRegister(getRegister("VccpAuxMin"));
      //getVariable("VccpAuxMin")->set(convMonPwr(getRegister("VccpAuxMin")->get(4,0xFFF))); 

      //readRegister(getRegister("VccpDdrMin"));
      //getVariable("VccpDdrMin")->set(convMonPwr(getRegister("VccpDdrMin")->get(4,0xFFF))); 

      //readRegister(getRegister("Flag"));
      //getVariable("Flag")->setInt(getRegister("Flag")->get());                   //0x3F  
      //getVariable("Flag.JTGD")->setInt(getRegister("Flag")->get(11,0x1));        //0x3F.BIT11
      //getVariable("Flag.JTGR")->setInt(getRegister("Flag")->get(10,0x1));        //0x3F.BIT10
      //getVariable("Flag.REF")->setInt(getRegister("Flag")->get(9,0x1));          //0x3F.BIT9
      //getVariable("Flag.VccpDdr")->setInt(getRegister("Flag")->get(7,0x1));      //0x3F.BIT7
      //getVariable("Flag.VccpAux")->setInt(getRegister("Flag")->get(6,0x1));      //0x3F.BIT6
      //getVariable("Flag.VccpInt")->setInt(getRegister("Flag")->get(5,0x1));      //0x3F.BIT5
      //getVariable("Flag.VccBram")->setInt(getRegister("Flag")->get(4,0x1));      //0x3F.BIT4
      //getVariable("Flag.OT")->setInt(getRegister("Flag")->get(3,0x1));           //0x3F.BIT3
      //getVariable("Flag.VccAux")->setInt(getRegister("Flag")->get(2,0x1));       //0x3F.BIT2
      //getVariable("Flag.VccInt")->setInt(getRegister("Flag")->get(1,0x1));       //0x3F.BIT1
      //getVariable("Flag.Temperature")->setInt(getRegister("Flag")->get(0,0x1));  //0x3F.BIT0

      REGISTER_UNLOCK
   }
   Device::pollStatus();
}

