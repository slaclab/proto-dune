//-----------------------------------------------------------------------------
// File          : AxiMicronN25Q.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 03/19/2014
// Project       :  
//-----------------------------------------------------------------------------
// Description :
//    Micron N25Q and Micron MT25Q PROM C++ Class
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
// 03/19/2014: created
//-----------------------------------------------------------------------------

#ifndef __AXI_MICRON_N25Q_H__
#define __AXI_MICRON_N25Q_H__

#include <stdint.h>
#include <string.h>

#include <Device.h>
#include <Register.h>
#include <MultDest.h>
#include <CommLink.h>

//! Class to contain generic register data.
class AxiMicronN25Q : public Device {
   public:

      //! Constructor
      AxiMicronN25Q (uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1);

      //! Deconstructor
      ~AxiMicronN25Q ( );

      void setPromSize (uint32_t promSize);
      
      uint32_t getPromSize (string pathToFile); 
      
      void setFilePath (string pathToFile);
      
      void setAddr32BitMode (bool addr32BitMode);
      
      void setPromStatusReg(uint8_t value);  
      
      uint8_t getPromStatusReg();    
      
      uint8_t getManufacturerId();      
      
      uint8_t getManufacturerType();        
      
      uint8_t getManufacturerCapacity();        
      
      void setLane (uint32_t lane);   
      
      void setVc (uint32_t vc);  

      void setTDest (uint32_t tDest);          
      
      //! Check if file exist
      bool fileExist ( );      
      
      //! Erase the PROM
      void eraseBootProm ( );    

      //! Write the .mcs file to the PROM
      bool writeBootProm ( ); 
      
      //! Write the .mcs file to the PROM
      bool bufferedWriteBootProm ( ); 
      
      //! Compare the .mcs file with the PROM
      bool verifyBootProm ( ); 
      
      //! Compare the .mcs file with the PROM
      bool bufferedVerifyBootProm ();           

      //! Print Reminder
      void rebootReminder ( bool pwrCycleReq );      
   
      //! Block Read of PROM (independent of .MCS file)
      void readBootProm (uint32_t address, uint32_t *data);      
   
   private:
      // Local Variables
      string   filePath_;
      uint32_t promSize_;
      uint32_t promStartAddr_;
      bool     addr32BitMode_;
      
      Register *addr32BitReg_;
      Register *addrReg_;
      Register *cmdReg_;
      Register *dataReg_;
      uint32_t data_[64];
      
      uint32_t lane_;
      uint32_t vc_;
      
      //! Enter 4-BYTE ADDRESS MODE Command
      void enter32BitMode( );     

      //! Exit 4-BYTE ADDRESS MODE Command
      void exit32BitMode( );      

      //! Erase Command
      void eraseCommand(uint32_t address);
      
      //! Write Command
      void writeCommand(uint32_t address); 
      
      //! Write Command
      bool bufferedWriteCommand(uint32_t baseAddr, uint8_t *data); 

      //! Read Command
      void readCommand(uint32_t address);       
      
      //! Buffered Read Command
      uint32_t bufferedReadCommand(uint32_t baseAddr, uint8_t *data);      
      
      //! Reset the FLASH memory Command
      void resetFlash ( );

      //! Enable Write commands
      void writeEnable ( );

      //! Disable Write commands
      void writeDisable ( );

      //! Wait for the FLASH memory to not be busy
      void waitForFlashReady ( );

      //! Pull the status register
      uint32_t statusReg ( );
      
      //! Set the address register
      void setAddr (uint32_t value);      

      //! Set the command register
      void setCmd (uint32_t value);
      
      //! Send the data register
      void setData ();

      //! Get the data register
      void getData ();
};
#endif
