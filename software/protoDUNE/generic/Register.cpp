//-----------------------------------------------------------------------------
// File          : Register.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic register container
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

#include <Register.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <stdint.h>
using namespace std;

//! Constructor
Register::Register ( Register *reg ) {
   address_     = reg->address_;
   name_        = reg->name_;
   stale_       = reg->stale_;
   status_      = reg->status_;
   size_        = reg->size_;

   value_       = (uint32_t *)malloc(sizeof(uint32_t)*size_);
   memcpy(value_,reg->value_,size_*4);
}

// Constructor
Register::Register ( string name, uint32_t address, uint32_t size ) {
   address_     = address;
   name_        = name;
   value_       = (uint32_t *)malloc(sizeof(uint32_t)*size);
   stale_       = false;
   status_      = 0;
   size_        = size;

   memset(value_,0x00,(sizeof(uint32_t)*size_));
}

// DeConstructor
Register::~Register ( ) {
   free(value_);
}

// Method to get register name
string Register::name () { return(name_); }

// Method to get register address
uint32_t Register::address () { return(address_); }

// Method to set register address
void Register::setAddress(uint32_t address) { address_ = address;}

// Method to get register size
uint32_t Register::size () {
   return(size_);
}

// Method to get register data point32_ter
uint32_t *Register::data () {
   return(value_);
}

void Register::setData ( uint32_t *data ) {
   if ( memcmp(data,value_,size_*4) ) stale_ = true;
   memcpy(value_,data,size_*4);
}

// Method to set status
void Register::setStatus (uint32_t status) {
   status_ = status;
}

// Method to get status
uint32_t Register::status () {
   return(status_);
}

//! Clear register stale
void Register::clrStale() { stale_ = false; }

//! Set register stale
void Register::setStale() { stale_ = true; }

//! Get register stale
bool Register::stale() {
   return(stale_);
}

// Method to set register value
void Register::set ( uint32_t value, uint32_t bit, uint32_t mask ) {
   setIndex(0, value, bit, mask);
}

// Method to get register value
uint32_t Register::get ( uint32_t bit, uint32_t mask ) {
   return(getIndex(0, bit, mask));
}

void Register::setIndex ( uint32_t index, uint32_t value, uint32_t bit, uint32_t mask ) {
   if ( index >= size_ ) return;

   uint32_t newVal = value_[index];
   
   newVal &= (0xFFFFFFFF ^ (mask << bit));
   newVal |= ((value & mask) << bit);

   if ( newVal != value_[index] ) {
      value_[index] = newVal;
      stale_ = true;
   }

}

uint32_t Register::getIndex ( uint32_t index, uint32_t bit, uint32_t mask ) {
   if ( index >= size_ ) return(0xFFFFFFFF);
   return((value_[index]>>bit) & mask);
}

