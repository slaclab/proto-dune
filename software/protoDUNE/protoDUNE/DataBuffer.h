//-----------------------------------------------------------------------------
// File          : DataBuffer.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    LBNE Data Buffer Control
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
//
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2016.10.28 jjr Added history block, creation date, unknown
//-----------------------------------------------------------------------------
#ifndef __DATA_BUFFER_H__
#define __DATA_BUFFER_H__

#include <Device.h>


class DaqBuffer;
class BufferStatus;

using namespace std;

//! Class to contain DataBuffer
class DataBuffer : public Device {

public:
   class StatusVariables
   {
   public:
      StatusVariables (Device *device);

   public:
      enum Enum 
      {
         BufferCount =  0,
         RxCount     =  1,
         RxRate      =  2,
         RxBw        =  3,
         RxSize      =  4,
         RxPend      =  5,
         RxErrors    =  6,
         DropCount   =  7,
         Triggers    =  8,
         TriggerRate =  9,
         TxErrors    = 10,
         TxCount     = 11,
         TxSize      = 12,
         TxBw        = 13,
         TxRate      = 14,
         TxPend      = 15,
         StatusCnt   = 16
      };

      Variable *v[StatusCnt];

      void set (BufferStatus const &status);
   };

   
   class ConfigurationVariables
   {
   public:
      ConfigurationVariables (Device *d);

   public:
      enum Enum
      {
         RunMode          = 0,
         BlowOffDmaData   = 1,
         BlowOffTxEth     = 2,
         PreTrigger       = 3,
         Duration         = 4,
         Period           = 5,
         DaqPort          = 6,
         DaqHost          = 7,

         ConfigurationCnt = 8
      };

      Variable *v[ConfigurationCnt];
   };
      

   public:

      //! Constructor
      /*! 
       * \param linkConfig Device linkConfig
       * \param index       Device index
       * \param parent      Parent device
      */
      DataBuffer ( uint32_t linkConfig, uint32_t index, Device *parent );

      //! Deconstructor
      ~DataBuffer ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      //! Hard Reset
      void hardReset ();

      //! Soft Reset
      void softReset ();

      //! Count Reset
      void countReset (); 

      //! Method to write configuration registers
      void writeConfig ( bool force );

      // Enable running
      void enableRun();

      // Enable running
      void disableRun();

      // Poll Device State
      void pollStatus();

      // Poll State
      void statusPoll();

    void readStatus ( );

private:
      // DAQ Handlers
      DaqBuffer       *daqBuffer_;
      StatusVariables         sv_;
      ConfigurationVariables  cv_;
};

#endif
