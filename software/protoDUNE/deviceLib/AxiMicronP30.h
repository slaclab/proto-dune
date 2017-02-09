//-----------------------------------------------------------------------------
// File          : AxiMicronP30.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 03/19/2014
// Project       :  
//-----------------------------------------------------------------------------
// Description :
//    PgpCardG2 PROM C++ Class
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

#ifndef __AXI_MICRON_P30_H__
#define __AXI_MICRON_P30_H__

#include <stdint.h>
#include <string.h>

#include <Device.h>
#include <Register.h>
#include <MultDest.h>
#include <CommLink.h>

using namespace std;

//! Class to contain generic register data.
class AxiMicronP30 : public Device {
   public:

      //! Constructor
      AxiMicronP30 (uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize=1);

      //! Deconstructor
      ~AxiMicronP30 ( );

      void setPromSize (uint32_t promSize);
      
      uint32_t getPromSize (string pathToFile); 
      
      void setFilePath (string pathToFile);
      
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
   
   private:
      // Local Variables
      string filePath_;
      uint32_t promSize_;
      uint32_t promStartAddr_;
      
      Register *dataReg_;
      Register *addrReg_;
      Register *readReg_;
      
      Register *fastAddr_;
      Register *fastProg_;
      
      Register *burstReg_;
      
      uint32_t lane_;
      uint32_t vc_;

      //! Erase Command
      void eraseCommand(uint32_t address);
      
      //! Program Command
      void programCommand(uint32_t address, uint16_t data);

      //! Generate request word 
      uint32_t genReqWord(uint16_t cmd, uint16_t data);

      //! Generic FLASH write Command
      void writeToFlash(uint32_t address, uint16_t cmd, uint16_t data);

      //! Generic FLASH read Command
      uint16_t readFromFlash(uint32_t address);    

      //! Buffered Program Command
      bool bufferedProgramCommand(uint32_t baseAddr, uint32_t size, uint16_t *data);     

      //! Buffered Read Command
      uint32_t bufferedReadCommand(uint32_t baseAddr, uint16_t *data);
};
#endif
