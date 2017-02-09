//-----------------------------------------------------------------------------
// File          : Command.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic command container
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
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <Command.h>
#include <sstream>
#include <stdint.h>
using namespace std;

// Constructor
Command::Command ( string name, uint32_t opCode ) {
   name_        = name;
   opCode_      = opCode;
   internal_    = false;
   desc_        = "";
   isHidden_    = false;
   hasArg_      = false;
}

// Constructor
Command::Command ( string name ) {
   name_        = name;
   opCode_      = 0;
   internal_    = true;
   desc_        = "";
   isHidden_    = false;
   hasArg_      = false;
}

// Set variable description
void Command::setDescription ( string description ) {
   desc_ = description;
}

// Method to get internal state
bool Command::internal () { return(internal_); }

// Method to get command name
string Command::name () { return(name_); }

// Method to get command opCode
uint32_t Command::opCode () { return(opCode_); }

// Method to get variable information in xml form.
string Command::getXmlStructure (bool hidden, uint32_t level) {
   stringstream           tmp;

   if ( isHidden_ && !hidden ) return(string(""));

   tmp.str("");
   if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) tmp << " ";
   tmp << "<command>" << endl;

   if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "<name>" << name_ << "</name>" << endl;

   if ( desc_ != "" ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<description>" << desc_ << "</description>" << endl;
   }

   if ( isHidden_ ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<hidden/>" << endl;
   }

   if ( hasArg_ ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<hasArg/>" << endl;
   }

   if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) tmp << " ";
   tmp << "</command>" << endl;
   return(tmp.str());
}

// Set hidden status
void Command::setHidden ( bool state ) {
   isHidden_ = state;
}

// Set has arg status
void Command::setHasArg ( bool state ) {
   hasArg_ = state;
}
