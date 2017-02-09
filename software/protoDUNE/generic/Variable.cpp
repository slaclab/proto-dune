//-----------------------------------------------------------------------------
// File          : Variable.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic variable container
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
// 02/07/2011: Added map option
//-----------------------------------------------------------------------------

#include <Variable.h>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <Device.h>
#include <stdint.h>
using namespace std;

// Constructor
Variable::Variable ( string name, VariableType type ) {
   name_         = name;
   value_        = "";
   type_         = type;
   compValid_    = false;
   compA_        = 0;
   compB_        = 0;
   compC_        = 0;
   compUnits_    = "";
   rangeMin_     = 0;
   rangeMax_     = 0;
   desc_         = "";
   perInstance_  = (type == Status);
   isHidden_     = false;
   convFunc_     = convInt;
   convData_     = (void *)(&base_);
   base_         = 16;
   noConfig_     = false;
   hasBeenSet_   = true;

   pthread_mutex_init(&mutex_,NULL);
}

// Set enum list      
void Variable::setEnums ( EnumVector enums ) {
   uint32_t x;

   values_.clear();

   for ( x=0; x < enums.size(); x++ ) 
      values_.insert(pair<uint32_t,string>(x,enums[x]));

   value_ = values_[0];

   setConversion(convEnum,&values_);
}

// Set map 
void Variable::setMap ( EnumMap map ) {
   values_ = map;

   value_ = values_.begin()->second;

   setConversion(convEnum,&values_);
}

//! Set as base 10
void Variable::setBase10() {
   base_ = 10;
   setConversion(convInt,(void *)(&base_));
}

//! Set as base 16
void Variable::setBase16() {
   base_ = 16;
   setConversion(convInt,(void *)(&base_));
}

void Variable::setBase2() {
   base_ = 2;
   setConversion(convInt,(void *)(&base_));
}

//! Set as string
void Variable::setString() {
   setConversion(convString,NULL);
}

// Create a variable link
void Variable::setConversion ( VarConvFunc_t function, void *userData ) {
   convFunc_     = function;
   convData_     = userData;
}

// Set as true/false
void Variable::setTrueFalse ( ) {
   values_.clear();
   values_[0] = "False";
   values_[1] = "True";

   value_ = values_[0];
   setConversion(convEnum,&values_);
}

// Set computation constants
void Variable::setComp ( double compA, double compB, double compC, string compUnits ) {
   compValid_ = true;
   compA_     = compA;
   compB_     = compB;
   compC_     = compC;
   compUnits_ = compUnits;
}

// Set range
void Variable::setRange ( uint32_t min, uint32_t max ) {
   rangeMin_    = min;
   rangeMax_    = max;
}

// Set variable description
void Variable::setDescription ( string description ) {
   desc_ = description;
}

// Set per-instance status
void Variable::setPerInstance ( bool state ) {
   perInstance_ = state;
}

// Get per-instance status
bool Variable::perInstance ( ) {
   return(perInstance_);
}

// Set hidden status
void Variable::setHidden ( bool state ) {
   isHidden_ = state;
}

// Get hidden status
bool Variable::hidden () {
   return(isHidden_);
}

// Set no config
void Variable::setNoConfig ( bool state ) {
   noConfig_ = state;
}

// Get hidden status
bool Variable::noConfig () {
   return(noConfig_);
}

// Get type
Variable::VariableType Variable::type() {
   return(type_);
}

// Get name
string Variable::name() {
   return(name_);
}

// Method to set variable value
void Variable::set ( string value ) {
   VariableLinkVector::iterator linkIter;

   string temp;

   pthread_mutex_lock(&mutex_);
   value_ = value.c_str(); // Force copy
   hasBeenSet_ = true;
   pthread_mutex_unlock(&mutex_);

   // Device is linked
   if ( links_.size() > 0 ) {
      for (linkIter = links_.begin(); linkIter != links_.end(); linkIter++) {
         temp = value;
         if ( (*linkIter)->function != NULL ) (*linkIter)->function(&temp,true,(*linkIter)->data);
         (*linkIter)->device->set((*linkIter)->variable,temp);
      }
   }
}

// Method to get variable value, status mode
string Variable::get ( ) {
   return(get(false,NULL));
}

// Method to get variable value
string Variable::get (bool compact, bool *include) {
   string temp;
   VariableLink * link;

   // Device is linked only once
   if ( links_.size() == 1 ) {

      link = links_.at(0);
      temp = link->device->getVariable(link->variable)->get(compact,include);

      if ( link->function != NULL ) link->function(&temp,false,link->data);

      pthread_mutex_lock(&mutex_);
      if ( compact && include != NULL && value_ != temp ) *include = true;
      value_ = temp.c_str(); // Force copy
      pthread_mutex_unlock(&mutex_);
   } 
  
   // Not linked 
   else {
      pthread_mutex_lock(&mutex_);

      // Determine if variable has been set since last compact get call
      if ( include != NULL ) {
         if ( compact ) {
            *include = hasBeenSet_;
            hasBeenSet_ = false;
         }
         else *include = true;
      }

      temp = value_.c_str(); // Force copy
      pthread_mutex_unlock(&mutex_);
   }

   return(temp);
}

// Method to set variable register value
void Variable::setInt ( uint32_t value ) {
   setInt(1,&value,0,0xFFFFFFFF);
}

// Method to set variable register value
void Variable::setInt ( uint32_t count, uint32_t *values, uint32_t bit, uint32_t mask ) {
   string       newValue;
   stringstream tmp;

   try {
      convFunc_(&newValue,count,values,bit,mask,true,convData_);
   } catch ( string error ) {
      tmp.str("");
      tmp << "Variable::setInt -> Got error in " << name_ << ". " << error;
      throw(tmp.str());
   }

   set(newValue);
}

// Method to get variable register value
uint32_t Variable::getInt ( ) {
   uint32_t value;

   getInt(1,&value,0,0xFFFFFFFF);
   return(value);
}

// Method to get variable int32_teger value to multiple int32_tegers.
void Variable::getInt ( uint32_t count, uint32_t *values, uint32_t bit, uint32_t mask ) {
   string       value;
   stringstream tmp;

   value = get();

   try {
      convFunc_(&value,count,values,bit,mask,false,convData_);
   } catch ( string error ) {
      tmp.str("");
      tmp << "Variable::getInt -> Got error in " << name_ << ". " << error;
      throw(tmp.str());
   }
}

// Method to set variable register value
void Variable::setFloat ( float value, const char * format ) {
   string       newValue;
   stringstream tmp;
   char         buffer[100];

   tmp.str("");
   newValue = "";

   // Variable is an enum
   if ( values_.size() != 0 ) {
      tmp << "Variable::setInt -> Name: " << name_ << endl;
      tmp << "   Invalid enum value: " << value << endl;
      throw(tmp.str());
   }
   else {
      sprintf(buffer,format,value);
      newValue = buffer;
   }

   set(newValue);
}

// Method to get variable register value
float Variable::getFloat ( ) {
   string  lvalue;

   lvalue = get();

   // Value can't be converted to int32_teger
   if ( lvalue == "" ) return(0);

   // Enum
   if ( values_.size() != 0 ) return(0);

   return(atof(lvalue.c_str()));
}

//! Method to get variable information in xml form.
string Variable::getXmlStructure (bool hidden, uint32_t level) {
   EnumMap::iterator enumIter;
   stringstream      tmp;

   if ( (noConfig_ && type_ != Variable::Status) || (isHidden_ && !hidden) ) return(string(""));

   tmp.str("");
   if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) tmp << " ";
   tmp << "<variable>" << endl;
   if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "<name>" << name_ << "</name>" << endl;
   if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "<type>";
   switch ( type_ ) {
      case Configuration : tmp << "Configuration";  break;
      case Status        : tmp << "Status";         break;
      case Feedback      : tmp << "Feedback";       break;
      default : tmp << "Unkown"; break;
   }
   if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "</type>" << endl;

   // Enums
   if ( values_.size() != 0 ) {
      for ( enumIter = values_.begin(); enumIter != values_.end(); enumIter++ ) {
         if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
         tmp << "<enum>" << enumIter->second << "</enum>" << endl;
      }
   }

   // Computations
   if ( compValid_ ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compA>" << compA_ << "</compA>" << endl;
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compB>" << compB_ << "</compB>" << endl;
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compC>" << compC_ << "</compC>" << endl;
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compUnits>" << compUnits_ << "</compUnits>" << endl;
   }

   // Range
   if ( rangeMin_ != rangeMax_ ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<min>" << dec << rangeMin_ << "</min>" << endl;
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<max>" << dec << rangeMax_ << "</max>" << endl;
   }

   if ( desc_ != "" ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<description>" << desc_ << "</description>" << endl;
   }

   if ( perInstance_ ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<perInstance/>" << endl;
   }

   if ( isHidden_ ) {
      if ( level != 0 ) for (uint32_t l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<hidden/>" << endl;
   }

   if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) tmp << " ";
   tmp << "</variable>" << endl;
   return(tmp.str());
}

// Create a variable link
void Variable::addLink ( Device *device, string variable, VarLinkFunc_t function, void *userData ) {

   VariableLink * link = new VariableLink;

   link->device   = device;
   link->variable = variable;
   link->function = function;
   link->data     = userData;

   link->device->getVariable(variable)->setNoConfig(true);

   links_.push_back(link);
}

// Default callback for base16/base10, user field contains base
void Variable::convInt (string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData) {
   stringstream   tmp;
   const char   * sptr;
   char         * eptr;
   uint32_t       shift;
   uint32_t       base;
   uint64_t       longInt;
   uint32_t       invMask;

   base  = *((uint32_t *)userData);
   shift = 0;

   if ( size > 2 ) throw(string("Integer data can not be larger than 64-bits"));
   else if ( size == 2 ) {
      while ( (mask >> shift) != 0 ) shift++;
   }

   // Set
   if ( set ) {

      longInt = ((data[0] >> bit) & mask);
      if ( size > 1 ) {
         longInt |= (uint64_t)((data[1] >> bit) & mask) << shift;
      }
         
      tmp.str("");
      if ( base == 16 ) tmp << "0x" << hex << setw(0) << longInt;
      else  tmp << dec << setw(0) << longInt;
      *value = tmp.str();
   }

   // Get
   else {
      invMask = (mask << bit) ^ 0xFFFFFFFF;
      data[0] &= invMask;
      if ( size > 1 ) data[1] &= invMask;

      if ( *value != "" ) {

         sptr    = value->c_str();
         longInt = (uint32_t)strtoull(sptr,&eptr,0);

         // Check for error
         if ( *eptr != '\0' || eptr == sptr ) throw(string("Value is not an int32_teger"));

         data[0] |= ((uint32_t)(longInt & mask) << bit);
         if ( size > 1 ) data[1] |= (uint32_t)(((longInt >> shift) & mask) << bit);
      }
   }
}

// Default callback for enum, pass enum vector in user field
void Variable::convEnum (string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData) {
   stringstream      tmp;
   EnumMap::iterator enumIter;
   EnumMap         * values;
   uint32_t              invMask;

   values = (EnumMap *)userData;

   if ( size > 1 ) throw(string("Enums larger than 32-bits not allowed"));

   // Set
   if ( set ) {
      try {
         *value = values->at((data[0]>>bit)&mask);
      }
      catch (const out_of_range& oor) {
         tmp << "Invalid enum value: 0x" << hex << setw(0) << data[0];
         throw(tmp.str());
      }
   }

   // Get
   else {
      invMask = (mask << bit) ^ 0xFFFFFFFF;

      // Find the value
      for (enumIter = values->begin(); enumIter != values->end(); enumIter++) {
         if ( enumIter->second == *value ) {
            data[0] &= invMask;
            data[0] |= ((enumIter->first & mask) << bit);
            return;
         }
      }

      // Value was not found
      tmp.str("");
      tmp << "Invalid enum string: " << *value << endl;
      throw(tmp.str());
   }
}

// Default callback for string, user field unused
void Variable::convString (string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData) {
   char locData[size*4];
   uint32_t x;

   // Bit and mask are ignored for string conversion

   // Set
   if ( set ) {
      memcpy(locData,data,size*4);
      locData[(size*4)-1] = 0; // Just in case
      *value = locData;
   }

   // Get does nothing
   else {
      for (x=0; x < size; x++) data[x] = 0;
   }
}


