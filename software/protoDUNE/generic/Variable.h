//-----------------------------------------------------------------------------
// File          : Variable.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic variable container.
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
// 02/07/2014: Added map option
//-----------------------------------------------------------------------------
#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <pthread.h>
#include <stdint.h>
using namespace std;

class Device;

// Conversion and link function types
typedef void (*VarConvFunc_t)(string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData);
typedef void (*VarLinkFunc_t)(string *value, bool set, void *userData);

// Class to hold a variable link
class VariableLink {
   public:
      Device      * device;
      string        variable;
      VarLinkFunc_t function;
      void        * data;
};

// Local types
typedef vector<VariableLink *> VariableLinkVector;
typedef vector<string>         EnumVector;
typedef map<uint32_t,string>   EnumMap;

//! Class to contain generic variable data.
class Variable {

      // Mutux variable for thread locking
      pthread_mutex_t mutex_;

   public:

      //! Variable Type Constants
      enum VariableType {
         Configuration      = 0, /*!< Variable Is Configuration */
         Status             = 1, /*!< Variable Is Status */
         Feedback           = 2  /*!< Variable Is configuration feedback */
      };

   private:

      // Variable name
      string name_;

      // Current variable value
      string value_;
      bool   hasBeenSet_;

      // Enum map
      EnumMap values_;

      // Variable Type
      VariableType type_;

      // Compute constants
      bool   compValid_;
      double compA_;
      double compB_;
      double compC_;
      string compUnits_;

      // Range values
      uint32_t rangeMin_;
      uint32_t rangeMax_;

      // Base value
      uint32_t base_;

      // Variable description
      string desc_;

      // Variable is instance specific
      bool perInstance_;

      // Variable is hidden
      bool isHidden_;

      // conversion function
      VarConvFunc_t convFunc_;
      void        * convData_;
         
      // Variable Linking
      VariableLinkVector links_;

      // Variable is slave
      bool noConfig_;

   public:

      //! Constructor
      /*! 
       * \param name    name of variable
       * \param type    VariableType value
      */
      Variable ( string name, VariableType type);

      //! Set enum list 
      /*! 
       * \param enums Vector of enum values
      */
      void setEnums ( EnumVector enums );

      //! Set enum list      
      /*! 
       * \param enums Vector of enum values
      */
      void setMap ( EnumMap map );
      
      //! Set as base 10
      void setBase10();

      //! Set as base 16
      void setBase16();

      //! Set as base 2
      void setBase2();

      //! Set as string
      void setString();

      //! Set conversion function
      void setConversion( VarConvFunc_t function, void * userData);

      //! Set variable as true/false
      void setTrueFalse ();

      //! Set computation constants for GUI
      /*! 
       * These values determine how to convert the variable 
       * integer into a readable value. The equation used is:
       * (value + compA) * compB + compC.
       * \param compA compA constant
       * \param compB compB constant
       * \param compC compC constant
       * \param compUnits Units value to add to end of computed value
      */
      void setComp ( double compA, double compB, double compC, string compUnits );

      //! Set range values
      /*! 
       * \param min   Range minimum
       * \param max   Range maximum
      */
      void setRange ( uint32_t min, uint32_t max );

      //! Set variable description
      /*! 
       * \param description variable description
      */
      void setDescription ( string description );

      //! Set per-instance status
      /*! 
       * This field determines if the variable is unique per-instance.
       * \param state per-instance status
      */
      void setPerInstance ( bool state );

      //! Get per-instance status
      bool perInstance ( );

      //! Keep out of config reads
      void setNoConfig ( bool state );

      //! Get no config state
      bool noConfig ( );

      //! Set hidden status
      /*! 
       * This field determines if the variable is hidden.
       * \param state hidden status
      */
      void setHidden ( bool state );

      //! Get hidden status
      bool hidden ( );

      //! Get variable name
      string name();

      //! Get variable type
      VariableType type();

      //! Method to set variable value
      /*! 
       * \param value variable value
      */
      void set ( string value );

      //! Special get call for status variables
      /*! 
       * Returns variable value
       * \param compact Compact mode enabled
       * \param include pointer to include boolean
      */
      string get (bool compact, bool *include );

      //! Method to get variable value
      /*! 
       * Returns variable value
      */
      string get ( );

      //! Method to set variable integer value
      /*!
       * Throws string on error
       * \param value integer value
      */
      void setInt ( uint32_t value );

      //! Method to set variable integer value from multiple integers.
      /*!
       * Throws string on error
       * \param count number of values to convert
       * \param value array of integers
       * \param bit   bit base
       * \param mask  mask value
      */
      void setInt ( uint32_t count, uint32_t *values, uint32_t bit, uint32_t mask );

      //! Method to get variable integer value
      /*!
       * Returns integer value 
       * Throws string on error
      */
      uint32_t getInt ( );

      //! Method to get variable int32_teger value to multiple int32_tegers.
      /*!
       * Throws string on error
       * \param count number of values to convert
       * \param value array of integers
       * \param bit   bit base
       * \param mask  mask value
      */
      void getInt ( uint32_t count, uint32_t *values, uint32_t bit, uint32_t mask );

      //! Method to set variable float value
      /*!
       * Throws string on error
       * \param value float value
       * \param format float format
      */
      void setFloat ( float value, const char *format );

      //! Method to get variable float value
      /*!
       * Returns float value 
       * Throws string on error
      */
      float getFloat ( );

      //! Method to get variable information in xml form.
      /*!
       * \param hidden include hidden variables
       * \param level  level for indents
      */
      string getXmlStructure ( bool hidden, uint32_t level );

      // Create a variable link
      void addLink ( Device *device, string variable, VarLinkFunc_t function = NULL, void *userData = NULL);

      // Default callback for base16/base10, user field contains base
      static void convInt (string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData);

      // Default callback for enum, pass enum vector in user field
      static void convEnum (string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData);

      // Default callback for string, user field unused
      static void convString (string *value, uint32_t size, uint32_t *data, uint32_t bit, uint32_t mask, bool set, void *userData);

};
#endif
