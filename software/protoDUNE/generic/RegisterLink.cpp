//-----------------------------------------------------------------------------
// File          : RegisterLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/01/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Complex class containing a number of variables linked to a register.
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
// 10/01/2014: created
//-----------------------------------------------------------------------------
#include <RegisterLink.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <iomanip>
#include <stdint.h>
using namespace std;

// Common init function
void RegisterLink::init ( string name, uint32_t address, uint32_t size, uint32_t count, 
                          Variable::VariableType type, uint32_t bit, uint32_t mask) {
   stringstream tmp;

   if ( count == 0 ) count_ = 1;
   else count_ = count;

   hasStatus_  = false;
   hasConfig_  = false;
   verify_     = true;
   pollEnable_ = false;

   // Sanity Check
   if ( (size == 0) || (size != 1 && count_ != 1 && size != count_) ) {
      tmp.str("");
      tmp << "RegisterLink::RegisterLink -> Invalid register to variable link for " << name;
      throw(tmp.str());
   }

   r_ = new Register (name,address,size);

   bit_    = (uint32_t *) malloc(sizeof(uint32_t) * count_);
   mask_   = (uint32_t *) malloc(sizeof(uint32_t) * count_);

   v_ = (Variable **) malloc (sizeof(Variable*) * count_);

   // Direct mapping
   if ( count == 0 ) {
      v_[0]    = new Variable(name,type);
      bit_[0]  = bit;
      mask_[0] = mask;
      verifyMask_ = mask << bit;
      if ( type == Variable::Configuration ) hasConfig_ = true;
      if ( type == Variable::Status        ) hasStatus_ = true;
   }
}

// Constructors
RegisterLink::RegisterLink ( string name, uint32_t address, Variable::VariableType type, uint32_t bit, uint32_t mask) {
   init ( name, address, 1, 0, type, bit, mask);
}

RegisterLink::RegisterLink ( string name, uint32_t address, uint32_t size, Variable::VariableType type, uint32_t bit, uint32_t mask) {
   init ( name, address, size, 0, type, bit, mask );
}

RegisterLink::RegisterLink ( string name, uint32_t address, uint32_t size, uint32_t count, ... ) {
   Variable::VariableType type;

   va_list      a_list;
   const char * varName;
   uint32_t         x;

   init ( name, address, size, count, Variable::Status, 0, 0 );

   va_start(a_list,count);
   verifyMask_ = 0;

   for (x=0; x < count_; x++) {
      varName  = va_arg(a_list,const char *);
      type     = (Variable::VariableType)va_arg(a_list,uint32_t);
      bit_[x]  = va_arg(a_list,uint32_t);
      mask_[x] = va_arg(a_list,uint32_t);

      if (type == Variable::Configuration) {
         verifyMask_ |= (mask_[x] << bit_[x]);
      }

      v_[x]    = new Variable(varName,type);

      if ( type == Variable::Configuration ) hasConfig_ = true;
      if ( type == Variable::Status        ) hasStatus_ = true;
   }
   va_end(a_list);
}

//! DeConstructor
RegisterLink::~RegisterLink ( ) { 
   delete(bit_);
   delete(mask_);
   delete(v_);
}

// Set verify
void RegisterLink::setVerify(bool verify) {
   verify_ = verify;
}

// Get verify
bool RegisterLink::verify() {
   return(verify_);
}

// Check type
bool RegisterLink::hasType(Variable::VariableType type) {
   if ( type == Variable::Configuration ) return(hasConfig_);
   if ( type == Variable::Status        ) return(hasStatus_);
   return(false);
}

//! Get register object
Register * RegisterLink::getRegister () {
   return(r_);
}

//! Get variable count
uint32_t RegisterLink::getVariableCount () {
   return(count_);
}

//! Get variable at index
Variable * RegisterLink::getVariable (uint32_t x ) {
   if ( x >= count_ ) return(v_[0]);
   else return(v_[x]);
}

//! Set register from variables
void RegisterLink::variablesToRegister() {
   uint32_t newData[r_->size()];
   uint32_t x;

   // Get copy of data
   memcpy(newData,r_->data(),r_->size()*4);

   // Single variable
   if ( count_ == 1 ) v_[0]->getInt (r_->size(), newData, bit_[0], mask_[0]);

   // Multiple variables for one register
   else if ( r_->size() == 1 ) {
      for (x = 0; x < count_; x++) v_[x]->getInt (1, newData, bit_[x], mask_[x]);
   }
   
   // 1 register location for each variable
   else {
      for (x=0; x < count_; x++) v_[x]->getInt (1, newData+x, bit_[x], mask_[x]);
   }

   // Set new data
   r_->setData(newData);
}

//! Set variables from register
void RegisterLink::registerToVariables() {
   uint32_t x;

   // Single variable
   if ( count_ == 1 ) v_[0]->setInt (r_->size(), r_->data(), bit_[0], mask_[0]);

   // Multiple variables for one register
   else if ( r_->size() == 1 ) {
      for (x = 0; x < count_; x++) v_[x]->setInt (1, r_->data(), bit_[x], mask_[x]);
   }

   // 1 register location for each variable
   else {
      for (x = 0; x < count_; x++) v_[x]->setInt (1, r_->data()+x, bit_[x], mask_[x]);
   }
}

uint32_t RegisterLink::getVerifyMask() {
   return verifyMask_;
}

void RegisterLink::setPollEnable(bool enable) {
   pollEnable_ = enable;
}

bool RegisterLink::pollEnable() {
   return(pollEnable_);
}

