//-----------------------------------------------------------------------------
// File          : Command.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic command container.
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
#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <string>
#include <stdint.h>
using namespace std;

//! Class to contain generic register data.
class Command {

      // Command opCode
      uint32_t opCode_;

      // Command name
      string name_;

      // Internal state
      bool internal_;

      // Description
      string desc_;

      // Command is hidden
      bool isHidden_;

      // Command has arg
      bool hasArg_;

   public:

      //! Constructor for external commands
      /*! 
       * \param name        Command name
       * \param opCode      Command opCode
       *
       * Note: OpCode[07:00] = OP Code Value
       *       OpCode[15:08] = Virtual Channel Number
       *       OpCode[23:16] = Lane Number
      */
      Command ( string name, uint32_t opCode );

      //! Constructor for internal commands
      /*! 
       * \param name        Command name
      */
      Command ( string name );

      //! Set variable description
      /*! 
       * \param description variable description
      */
      void setDescription ( string description );

      //! Method to get internal state
      bool internal ();

      //! Method to get command name
      string name ();

      //! Method to get command opCode
      uint32_t opCode ();

      //! Method to get variable information in xml form.
      /*! 
       * \param hidden Include hidden commands.
       * \param level  level for indents
      */
      string getXmlStructure ( bool hidden, uint32_t level );

      //! Set hidden status
      /*! 
       * This field determines if the command is hidden.
       * \param state hidden status
      */
      void setHidden ( bool state );

      //! Set has arg status
      /*! 
       * This field determines if the command has an arg
       * \param state has arg status
      */
      void setHasArg ( bool state );
};
#endif
