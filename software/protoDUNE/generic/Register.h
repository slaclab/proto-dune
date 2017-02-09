//-----------------------------------------------------------------------------
// File          : Register.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic register container.
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
#ifndef __REGISTER_H__
#define __REGISTER_H__

#include <string>
#include <stdint.h>
using namespace std;

//! Class to contain generic register data.
class Register {

      // Register address
      uint32_t address_;

      // Register name
      string name_;

      // Current register value
      uint32_t *value_;

      // Current register size
      uint32_t size_;

      // Register stale
      bool stale_;

      // Register status
      uint32_t status_;

   public:

      //! Constructor
      Register ( Register *reg );

      //! Constructor
      /*! 
       * \param name        Register name
       * \param base        Register base address
       * \param size        Register size, default = 1
      */
      Register ( string name, uint32_t base, uint32_t size = 1);

      //! DeConstructor
      ~Register ( );

      //! Method to get register name
      string name ();

      //! Method to get register address
      uint32_t address ();

      //! Method to set register address
      void setAddress(uint32_t address);

      //! Method to get register size
      uint32_t size ();

      //! Method to get register data point32_ter
      uint32_t *data ();

      //! Set status value
      /*! 
       * \param status Status value
      */
      void setStatus(uint32_t status);

      //! Get status value
      uint32_t status();

      //! Clear register stale
      void clrStale();

      //! Set register stale
      void setStale();

      //! Get register stale
      bool stale();

      //! Method to set register value
      /*!
       * Update the shadow register with the new value. Optional start
       * bit and mask to set a field within the register. Register state
       * will be set to stale.
       * \param value register value
       * \param bit start bit for field
       * \param mask mask for field
      */
      void set ( uint32_t value, uint32_t bit=0, uint32_t mask=0xFFFFFFFF );

      //! Method to get register value
      /*!
       * Return the value of the register as a whole or a field within
       * the register. Optional start bit and mask to set a field within 
       * the register.
       * \param bit start bit for field
       * \param mask mask for field
      */
      uint32_t get ( uint32_t bit=0, uint32_t mask=0xFFFFFFFF );

      //! Method to set register value
      /*!
       * Update the shadow register with the new value. 
       * Register state will be set to stale.
       * \param index register index
       * \param value register value
      */
      void setIndex ( uint32_t index, uint32_t value, uint32_t bit=0, uint32_t mask=0xFFFFFFFF );

      //! Method to get register value
      /*!
       * Return the value of the register as a whole or a field within
       * the register. 
       * \param index register index
      */
      uint32_t getIndex ( uint32_t index, uint32_t bit=0, uint32_t mask=0xFFFFFFFF );

      //! Method to set register value
      /*!
       * Update the shadow register with the new value. 
       * Register state will be set to stale.
       * \param data  bulk register data
      */
      void setData ( uint32_t *data );

};
#endif
