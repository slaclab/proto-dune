//-----------------------------------------------------------------------------
// File          : Device.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic device container.
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
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <libxml/tree.h>
#include <pthread.h>
#include <stdint.h>
#include <Variable.h>
using namespace std;

class Register;
class Command;
class Device;
class CommLink;
class System;
//class Variable;
class RegisterLink;

// Define local types
typedef map<string,Variable *>     VariableMap;
typedef map<string,Register *>     RegisterMap;
typedef map<string,Command  *>     CommandMap;
typedef vector<Device *>           DeviceVector;
typedef map<string,DeviceVector*>  DeviceMap;
typedef vector<RegisterLink *>     RegisterLinkVector;

// Macro to create lock and start try block
#define REGISTER_LOCK pthread_mutex_lock(&mutex_); try { 

// Macro to remove lock and end try block. Errors are caught and re-thrown
#define REGISTER_UNLOCK } catch (string error) { \
                           pthread_mutex_unlock(&mutex_); \
                           throw(error); \
                          } pthread_mutex_unlock(&mutex_);

//! Class to contain generic device data.
class Device {

   protected:

      // Mutux variable for thread locking
      pthread_mutex_t mutex_;

      // Device linkConfig
      uint32_t linkConfig_;

      // Device base address
      uint32_t baseAddress_;

      // Device name
      string name_;

      // Device index
      uint32_t index_;

      // Register Links
      RegisterLinkVector registerLinks_;

      // Map of variables
      VariableMap variables_;
      VariableMap hiddenVariables_;

      // Map of registers
      RegisterMap registers_;

      // Map of commands
      CommandMap commands_;

      // Map of device vectors
      DeviceMap devices_;

      // Description
      string desc_;

      // Debug flag
      bool debug_;

      // Poll Enable
      bool pollEnable_;

      // Parent device & top system
      Device *parent_;
      System *system_;

      // Write register if stale or if force = true
      // Throws string on error
      void writeRegister ( Register *reg, bool force, bool wait=true );

      // Read register
      // Throws string on error
      void readRegister ( Register *reg );

      // Verify register
      // Throws string on verify fail
      void verifyRegister ( Register *reg, bool warnOnly = false, uint32_t mask = 0xFFFFFFFF );

      // Method to set variable values from xml tree
      bool setXmlConfig ( xmlNode *node );

      // Method to get config variable values in xml form.
      // Two different types of config variables can be returned
      // per-device variables and common variables. The common flag
      // determines which type is to be returned. The hidden flag
      // determines if hidden variables should be included in the
      // string. Hidden variables will always be sent for the top
      // level device, determine by the top flag.
      string getXmlConfig ( bool top, bool common, bool hidden, uint32_t level );

      // Method to get status variable values in xml form.
      // The hidden flag determines if hidden status variables should 
      // be included in the string. Hidden variables will always be sent 
      // for the top level device, determine by the top flag.
      string getXmlStatus (bool top, bool hidden, uint32_t level, bool compact, bool recursive );

      // Method to execute commands from xml tree
      // Throws string on error
      void execXmlCommand ( xmlNode *node );

      // Method to get device structure in xml form.
      // Two different types of variables and commands can be returned
      // per-device and common The common flag determines which type 
      // is to be returned. The hidden flag determines if hidden entries 
      // should be included in the string. Hidden values will always be 
      // sent for the top level device, determine by the top flag.
      string getXmlStructure ( bool top, bool common, bool hidden, uint32_t level);

      // Register read and write helper functions
      void regReadInt ( string nameReg, string nameVar, uint32_t bit=0, uint32_t mask=0xFFFFFFFF);
      void regReadInt ( string name, uint32_t bit=0, uint32_t mask=0xFFFFFFFF );
      void regWrite   ( string nameReg, string nameVar, bool force, uint32_t bit=0, uint32_t mask=0xFFFFFFFF);
      void regWrite   ( string name, bool force, uint32_t bit=0, uint32_t mask=0xFFFFFFFF  );

   public:

      // Add registers
      void addRegister(Register *reg);

      // Add variables
      void addVariable(Variable *variable);

      // Add Direct Variable
      void addRegisterLink(RegisterLink *regLink);

      // Add devices
      void addDevice(Device *device);

      // Add commands
      void addCommand(Command *cmd);

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param baseAddress Device base address
       * \param name        Device name
       * \param index       Device index
       * \param parent      Parent device
      */
      Device ( uint32_t linkConfig, uint32_t baseAddress, string name, uint32_t index, Device *parent );

      //! Deconstructor
      virtual ~Device ( );

      //! Set debug flag
      /*! 
       * \param enable    Debug state
      */
      void setDebug( bool enable );

      //! Method to get name
      string name ();

      //! Method to get index
      uint32_t index();

      //! Method to get linkConfig
      uint32_t linkConfig();

      //! Method to get base address
      uint32_t baseAddress();

      //! Method to get sub device
      /*!
       * Throws string if device can't be found
       * \param name Device name
       * \param index Device index
      */
      Device * device ( string name, uint32_t index = 0 );

      //! Method to get sub device count
      /*!
       * Throws string if device can't be found
       * \param name Device name
      */
      uint32_t deviceCount ( string name );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      virtual void command ( string name, string arg );

      //! Method to set command for running
      /*!
       * \param name     Command name
      */
      void setRunCommand ( string name );

      //! Method to set a single variable
      /*!
       * Throws string on error
       * \param variable Variable name
       * \param value    Variable value
      */
      void set ( string variable, string value );

      //! Method to get a single variable
      /*! 
       * Return variable value
       * Throws string on error
       * \param variable Variable name
      */
      string get ( string variable );

      //! Method to set a single variable, integer value
      /*!
       * Throws string on error
       * \param variable Variable name
       * \param value    Variable value
      */
      void setInt ( string variable, uint32_t value );

      //! Method to get a single variable, int32_teger value
      /*! 
       * Return variable value
       * Throws string on error
       * \param variable Variable name
      */
      uint32_t getInt ( string variable );

      //! Method to set a single variable, float value
      /*!
       * Throws string on error
       * \param variable Variable name
       * \param value    Variable value
       * \param format   Variable format for sprintf
      */
      void setFloat ( string variable, float value, const char *format );

      //! Method to get a single variable, float value
      /*! 
       * Return variable value
       * Throws string on error
       * \param variable Variable name
      */
      float getFloat ( string variable );

      //! Method to read a specific register
      /*! 
       * Read a specific register without updaing associated variables.
       * Used for debug purposes.
       * Throws string on error.
       * \param name Register name
      */
      uint32_t readSingle ( string name );

      //! Method to write a specific register
      /*! 
       * Write a specific register ignoring associated variable values
       * Used for debug purposes.
       * Throws string on error.
       * \param name Register name
       * \param value Register value
      */
      void writeSingle( string name, uint32_t value );
      void writeSingleV2( string name, uint32_t value );

      //! Method to read status registers and update variables
      /*! 
       * Throws string on error.
      */
      virtual void readStatus ( );

      //! Method to poll status registers and update variables
      /*! 
       * Throws string on error.
      */
      virtual void pollStatus ( );

      //! Method to read configuration registers and update variables
      /*! 
       * Throws string on error.
      */
      virtual void readConfig ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      virtual void writeConfig ( bool force );

      //! Verify hardware state of configuration
      virtual void verifyConfig ( );

      //! Hide all commands in the device 
      void hideAllCommands();

      //! Hide all variables in the device 
      void hideAllVariables();

      //! Hide all variables of a given type
      void hideAllVariables(Variable::VariableType vt);

      //! Unhide all variables in the device
      void unhideVariables();

      //! Set all registers stale
      void setAllStale(bool stale=true, bool recursive=true);

      // Return register, throws exception when not found
      Register *getRegister(string name);

      // Return variable, throw exception when not found
      Variable *getVariable(string name);

      // Return command, throw exception when not found
      Command *getCommand(string name);

      // Set the device as pollable
      void pollEnable ( bool enable );

      // Hard reset
      virtual void hardReset ();

      // Soft reset
      virtual void softReset ();

      // Count reset
      virtual void countReset ();

};
#endif
