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
#ifndef __REGISTER_LINK_H__
#define __REGISTER_LINK_H__

#include <string>
#include <Variable.h>
#include <Register.h>
#include <stdarg.h>
#include <stdint.h>
using namespace std;

//! Class to contain generic register data.
class RegisterLink {
      Register  *  r_; 
      Variable  ** v_; 
      uint32_t   * bit_;
      uint32_t   * mask_;
      uint32_t     verifyMask_;
      uint32_t     count_;
      bool         verify_;
      bool         hasStatus_;
      bool         hasConfig_;
      bool         pollEnable_;

      Variable::VariableType type_;

      // Common init function
      void init (string name, uint32_t address, uint32_t size, uint32_t count, 
                 Variable::VariableType type, uint32_t bit, uint32_t mask );

   public:

      //! Constructor When A Single Register to Variable Mapping Exists,
      /*! 
       * \param name    Register/Variable name
       * \param address Register address
       * \param size    Size of register, Default = 1
       * \param type    VariableType value (not used when count > 1)
       * \param count   Number of variables passed as an arg. Same name mapping when not used.
       *
       * If size = 1 and count > 1 then each variable accesses a portion of a single register location.
       * If size > 1 and count = 1 or 0 then one variable is populated with the contents of multiple register locations. 
       * If size > 1 and count > 1 then a 1:1 mapping of variable locations is used. size must equal count.
       *
       * Optional repeated args (one set for each count)
       * \param varName Variable Name
       * \param type    Variable Type
       * \param bitBase Bit base for register
       * \param bitMask Bit mask for register
      */
      RegisterLink ( string name, uint32_t address, Variable::VariableType type, uint32_t bit = 0, uint32_t mask = 0xFFFFFFFF);
      RegisterLink ( string name, uint32_t address, uint32_t size, Variable::VariableType type, uint32_t bit = 0, uint32_t mask = 0xFFFFFFFF);
      RegisterLink ( string name, uint32_t address, uint32_t size, uint32_t count, ... );

      //! DeConstructor
      ~RegisterLink ( );

      //! Set verify
      void setVerify(bool verify);

      //! Get verify
      bool verify();

      uint32_t getVerifyMask();

      //! Check type
      bool hasType(Variable::VariableType type);

      //! Get register object
      Register * getRegister ();

      //! Get variable count
      uint32_t getVariableCount ();

      //! Get variable at index
      Variable * getVariable (uint32_t x = 0);

      //! Set register from variables
      void variablesToRegister();

      //! Set variables from register
      void registerToVariables();

      //! Set poll enable
      void setPollEnable(bool enable);

      //! Get poll enable
      bool pollEnable();

};
#endif
