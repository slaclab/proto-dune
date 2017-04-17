//-----------------------------------------------------------------------------
// File          : DataDpmSystem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
// DataDpmSystem Top Device
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
// 06/19/2014: created
//-----------------------------------------------------------------------------

#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <CommLink.h>

#include <MultLink.h>
#include <MultDestMapped.h>
#include <MultDestAxis.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>

#include <DataDpmSystem.h>
#include <DataDpm.h>
#include <DataBuffer.h>

using namespace std;

// Constructor
DataDpmSystem::DataDpmSystem (string defaults, uint32_t idx ) : System("DataDpmSystem") {
   stringstream tmp;
   MultLink   *link;

   dest_[0] = new MultDestMapped (8, 
      0x80000000,0x2000,  // Core registers
      0x84000000,0x1000,  // BSI registers
      0xA0000000,0x1000,  // HLS[0] registers
      0xA0010000,0x1000,  // HLS[1] registers
      0xA1000000,0x1000,  // WIB[0] registers
      0xA1010000,0x1000,  // WIB[1] registers
      0xA2000000,0x1000,  // EMU registers
      0xA3000000,0x1000); // Timing registers


   //dest_[0]->setDebug(true);      

   /*
    | 2017.04.08 -- jjr
    | -----------------
    | Adjustmets for the V2 driver
   */
   dest_[1] = new MultDestAxis("/dev/axi_stream_dma_0", 0); // Future FEB path???
   //dest_[2] = new MultDestAxis("/dev/axi_stream_dma_0",64); // DMA'd Local register Interface
   dest_[2] = new MultDestAxis("/dev/axi_stream_dma_1",0); // DMA'd Local register Interface

   link = new MultLink();
   //link->setDebug(true);
   link->setMaxRxTx(0x800000);
   link->open(3,dest_[0],dest_[1],dest_[2]);

   tmp.str("");
   tmp << "DataDpm_" << std::dec << std::setw(3) << setfill('0') << idx;
 
   // Description
   desc_      = tmp.str();
   defaults_  = defaults;
   idx_       = idx;
   commLink_  = link;


   // Set run states
   static const char *States[] = { "Stopped", "Enable" };
   vector<string> states (States, States + sizeof (States) /sizeof (*States));
   getVariable("RunState")->setEnums(states);

   set("HideRunControl","True");
   set("HideDataControl","True");
   getVariable("PollPeriod")->setInt(1);

   addDevice(new DataDpm(0x00000000,idx,this));
}

// Deconstructor
DataDpmSystem::~DataDpmSystem ( ) { 
   delete commLink_;
   delete dest_[0];
   delete dest_[1];
   delete dest_[2];
}

//! Method to perform soft reset
void DataDpmSystem::softReset ( ) { 

   setRunState("Stopped");
   command("CloseDataFile","");
   Device::softReset();
   allStatusReq_ = true;
   System::countReset();

   // Removed in favor of the above
   //System::softReset();   
}

//! Method to perform hard reset
void DataDpmSystem::hardReset ( ) {

   errorFlag_     = false;
   configureFlag_ = false;
   configureMsg_  = "System Is Not Configured.\nSet Defaults Or Load Settings!\n";
   setRunState ("Stopped");
   command     ("CloseDataFile","");
   Device::hardReset();


   topStatusReq_ = true;
   Device::countReset();

   // Removed in favor of the above
   //System::hardReset();
}


/* ---------------------------------------------------------------------- *//*!
 *                                                                             
 *  \brief  Local method to retrieve the data buffer
 *  \return A pointer to the data buffer
 *
\* ---------------------------------------------------------------------- */
inline DataBuffer *DataDpmSystem::dataBuffer ()
{
   DataBuffer   *buff = (DataBuffer*)(device("DataDpm", idx_)->device("DataBuffer", 0));
   return buff;
}
/* ---------------------------------------------------------------------- */


//! Method to set run state
void DataDpmSystem::setRunState ( string state ) {

   static const char Name[] = "DataDpmSystem::setRunState ";
   DataBuffer         *buff = dataBuffer ();


   // Pass run state to device configuration
   if ( state == "Enable")
   {
      buff->enableRun();      
      cout << Name << state <<"  Should be Enable " << endl;
   } 
   else 
   {
      cout << "Stopping Run..." << endl;
      cout << "\t\t\t disabling run in buffer" << endl;
      buff->disableRun();
      cout << Name << state << "  Should be Stopped " << endl;
   }

   getVariable("RunState")->set(state);
   allStatusReq_ = true;

   // Removed in favor of the above
   //System::setRunState(state);
}

//! Method to process a command
void DataDpmSystem::command ( string name, string arg ) {
   System::command(name, arg);
}


//! Return local state, specific to each implementation
string DataDpmSystem::localState() {


   DataBuffer *buff = dataBuffer  ();
   uint32_t  errors = buff->getInt("RxErrors") + buff->getInt("TxErrors");
   uint32_t  events = buff->getInt("TxCount");

   getVariable("ErrorCount" )->setInt(errors);
   getVariable("DataRxCount")->setInt(events);

   if      ( errors != lastErrors_ ) buffState_ = 2;
   else if ( errors >  0           ) buffState_ = 1;
   else                              buffState_ = 0;

   lastErrors_ = errors;


   if (buffState_)
   {
      Variable *v = getVariable ("SystemStatus");
      if ( v->get() == "Ready" )
      {
         if      ( buffState_ == 1 ) v->set("Warning");
         else if ( buffState_ == 2 ) v->set("Error");
      }
   }

   return("");
}

