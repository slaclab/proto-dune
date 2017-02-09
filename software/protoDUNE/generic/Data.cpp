//-----------------------------------------------------------------------------
// File          : Data.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic data container
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

#include <Data.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <stdint.h>
using namespace std;


// Update frame state
void Data::update() { }

// Constructor
Data::Data ( uint32_t *data, uint32_t size ) {
   size_  = size;
   alloc_ = size;
   data_  = (uint32_t *)malloc(alloc_ * sizeof(uint32_t));
   memcpy(data_,data,size_*sizeof(uint32_t));
   update();
}

// Constructor
Data::Data () {
   size_  = 0;
   alloc_ = 1;
   data_  = (uint32_t *)malloc(sizeof(uint32_t));
   update();
}

// Deconstructor
Data::~Data ( ) {
   free(data_);
}

// Read data from file descriptor
bool Data::read ( int32_t fd, uint32_t size ) {
   if ( size > alloc_ ) {
      free(data_);
      alloc_ = size;
      data_ = (uint32_t *)malloc(alloc_ * sizeof(uint32_t));
   }
   size_ = size;
   if ( ::read(fd, data_, size_*(sizeof(uint32_t))) != (int)(size_ *sizeof(uint32_t))) {
      size_ = 0;
      update();
      return(false);
   }
   update();
   return(true);
}

// Copy data from buffer
void Data::copy ( uint32_t *data, uint32_t size ) {
   if ( size > alloc_ ) {
      free(data_);
      alloc_ = size;
      data_ = (uint32_t *)malloc(alloc_ * sizeof(uint32_t));
   }
   size_ = size;

   // Copy data
   memcpy(data_,data,size_*sizeof(uint32_t));
   update();
}

// Get pointer to data buffer
uint32_t *Data::data ( ) {
   return(data_);
}

// Get data size
uint32_t Data::size ( ) {
   return(size_);
}

