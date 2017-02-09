//-----------------------------------------------------------------------------
// File          : AxiMicronP30.cpp
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
#include <fcntl.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <RegisterLink.h>
#include <Register.h>
#include <Data.h>
#include <System.h>
#include "AxiMicronP30.h"
#include "McsRead.h"

using namespace std;

#define PROM_BLOCK_SIZE    0x4000 // Assume the smallest block size of 16-kword/block
#define READ_MASK          0x80000000

// Constructor
AxiMicronP30::AxiMicronP30 (uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) :
      Device(linkConfig, baseAddress, "AxiMicronP30", index, parent) {

   dataReg_ = new Register("Data",     baseAddress + (0x0 * addrSize));
   addrReg_ = new Register("Addr",     baseAddress + (0x1 * addrSize));
   readReg_ = new Register("Read",     baseAddress + (0x2 * addrSize));
   
   addRegisterLink(new RegisterLink("Test", baseAddress + 0x3*addrSize, Variable::Configuration));
   
   fastAddr_  = new Register("FastAddr",  baseAddress + (0x4 * addrSize));
   fastProg_  = new Register("FastProg",  baseAddress + (0x5 * addrSize));
   burstReg_  = new Register("BurstAddr", baseAddress + (0x6 * addrSize));
   
   // Default PROM size = 0x0 (must be set using setPromSize() )
   promSize_      = 0x0;
   
   // Default PROM size = 0x0 (must be set using setFilePath() )
   promStartAddr_ = 0x0;
   
   // Default file path = NULL (must be set using setFilePath() )
   filePath_ = "";

   // Default for optional streaming lane_ and vc_ pointers
   lane_ = 0xFF;
   vc_   = 0xFF;
}

// Deconstructor
AxiMicronP30::~AxiMicronP30 ( ) { 
   delete dataReg_;
   delete addrReg_;
   delete readReg_;
   delete fastAddr_;
   delete fastProg_;
}

void AxiMicronP30::setPromSize (uint32_t promSize) {
   promSize_ = promSize;
}

uint32_t AxiMicronP30::getPromSize (string pathToFile) {
   McsRead mcsReader;
   uint32_t retVar;
   mcsReader.open(pathToFile);
   printf("Calculating PROM file (.mcs) Memory Address size ...");       
   retVar = mcsReader.addrSize();
   printf("PROM Size = 0x%08x\n", retVar); 
   mcsReader.close();
   return retVar; 
}

void AxiMicronP30::setFilePath (string pathToFile) {
   filePath_ = pathToFile;
   McsRead mcsReader;   
   mcsReader.open(filePath_);
   promStartAddr_ = mcsReader.startAddr();
   mcsReader.close();      
   // Configuration: Force default configurations
   writeToFlash(0xFD4F,0x60,0x03);
}

void AxiMicronP30::setLane (uint32_t lane) {
   lane_ = lane;
}

void AxiMicronP30::setVc (uint32_t vc) {
   vc_ = vc;
}

void AxiMicronP30::setTDest (uint32_t tDest) {
   lane_ = (tDest >> 4)&0xF;
   vc_   = (tDest >> 0)&0xF;
}

//! Check if file exist (true=exists)
bool AxiMicronP30::fileExist ( ) {
  ifstream ifile(filePath_.c_str());
  return ifile.good();
}

//! Print Power Cycle Reminder
void AxiMicronP30::rebootReminder ( bool pwrCycleReq ) {
   cout << "\n\n\n\n\n";
   cout << "***************************************" << endl;
   cout << "***************************************" << endl;
   cout << "The new data written in the PROM has "   << endl;
   cout << "has been loaded into the FPGA. "         << endl;
   if(pwrCycleReq){
      cout << endl;
      cout << "A reboot or power cycle is required " << endl;
   }
   cout << "***************************************" << endl;
   cout << "***************************************" << endl;
   cout << "\n\n\n\n\n";
}

//! Erase the PROM
void AxiMicronP30::eraseBootProm ( ) {

   uint32_t address = promStartAddr_;
   double size = double(promSize_);
   double percentage;
   double skim = 5.0; 

   cout << "*******************************************************************" << endl;   
   cout << "Starting Erasing ..." << endl; 
   while(address<(promStartAddr_+promSize_)) {       
      /*
      // Print the status to screen
      cout << hex << "Erasing PROM from 0x" << address << " to 0x" << (address+PROM_BLOCK_SIZE-1);
      cout << setprecision(3) << " ( " << ((double(address))/size)*100 << " percent done )" << endl;      
      */
      
      // execute the erase command
      eraseCommand(address);
      
      //increment the address pointer
      address += PROM_BLOCK_SIZE;
      
      percentage = (((double)(address-promStartAddr_))/size)*100;
      if( (percentage>=skim) && (percentage<100.0) ) {
         skim += 5.0;
         cout << "Erasing the PROM: " << floor(percentage) << " percent done" << endl;
      }               
   }   
   cout << "Erasing completed" << endl;
}

//! Write the .mcs file to the PROM
bool AxiMicronP30::writeBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Writing ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   uint32_t address = promStartAddr_;
   uint16_t fileData;
   double size = double(promSize_);
   double percentage;
   double skim = 1.0; 
   bool   toggle = false;

   //check for valid file path
   if ( !mcsReader.open(filePath_) ) {
      mcsReader.close();
      cout << "mcsReader.close() = file path error" << endl;
      return false;
   }  
   
   if(promSize_ == 0x0){
      cout << "Invalid promSize_: 0x0" << endl;
      return false;      
   }
   
   //reset the flags
   mem.endOfFile = false;      
   
   //read the entire mcs file
   while(1) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      // Check if this is the upper or lower byte
      if(!toggle) {
         toggle = true;
         fileData = (uint16_t)mem.data;
      } else {
         toggle = false;
         fileData |= ((uint16_t)mem.data << 8);
         programCommand(address,fileData);
         address++;
         percentage = (((double)(address-promStartAddr_))/size)*100;
         percentage *= 2.0;//factor of two from two 8-bit reads for every write 16 bit write
         if( (percentage>=skim) && (percentage<100.0) ) {
            skim += 5.0;
            cout << "Writing the PROM: " << dec << round(percentage)-1 << " percent done" << endl;
         }         
      }
   }
   
   mcsReader.close();   
   cout << "Writing completed" << endl;   
   return true;
}

//! Write the .mcs file to the PROM
bool AxiMicronP30::bufferedWriteBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Writing ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   
   uint32_t i;
   uint32_t address = 0;   
   uint16_t fileData;
   
   uint32_t bufAddr;  
   uint16_t bufData[256];   
   uint16_t bufSize = 0;
   
   double size = double(promSize_);
   double percentage;
   double skim = 5.0; 
   bool   toggle = false;

   //check for valid file path
   if ( !mcsReader.open(filePath_) ) {
      mcsReader.close();
      cout << "mcsReader.close() = file path error" << endl;
      return false;
   }  
   
   if(promSize_ == 0x0){
      cout << "Invalid promSize_: 0x0" << endl;
      return false;      
   }   
   
   //reset the flags
   mem.endOfFile = false;      
   
   //read the entire mcs file
   while(1) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      // Check if this is the upper or lower byte
      if(!toggle) {
         toggle = true;
         fileData = (uint16_t)mem.data;
      } else {
         toggle = false;
         fileData |= ((uint16_t)mem.data << 8);
         
         // Latch the values
         if(bufSize==0){
            bufAddr = address;
         }
         bufData[bufSize] = fileData;
         bufSize++;
         
         // Check if we need to send the buffer
         if(bufSize==256) {
            if(bufferedProgramCommand(bufAddr,bufSize,bufData)) {
               return false;
            }    
            bufSize = 0;
         }
         address++;
         percentage = (((double)(address))/size)*100;
         percentage *= 2.0;//factor of two from two 8-bit reads for every write 16 bit write
         if( (percentage>=skim) && (percentage<100.0) ) {
            skim += 5.0;
            cout << "Writing the PROM: " << floor(percentage) << " percent done" << endl;
         }         
      }
   }
   
   // Check if we need to send the buffer
   if(bufSize != 0) {
      // Pad the end of the block with ones
      for(i=bufSize;i<256;i++){
         bufData[bufSize] = 0xFFFF;
      }
      // Send the last block program 
      if(bufferedProgramCommand(bufAddr,256,bufData)) {
         return false;
      }      
   }   
   
   // Verify wont work after buffered program unless a bogus readFromFlash is called here
   // I have no idea why.
   readFromFlash(0);
   
   mcsReader.close();   
   cout << "Writing completed" << endl;   
   return true;
}

//! Compare the .mcs file with the PROM (true=matches)
bool AxiMicronP30::verifyBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Verification ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   uint32_t address = promStartAddr_;
   uint16_t promData,fileData;
   double size = double(promSize_);
   double percentage;
   double skim = 5.0; 
   bool   toggle = false;

   //check for valid file path
   if ( !mcsReader.open(filePath_) ) {
      mcsReader.close();
      cout << "mcsReader.close() = file path error" << endl;
      return(1);
   }

   if(promSize_ == 0x0){
      cout << "Invalid promSize_: 0x0" << endl;
      return false;      
   }   
   
   //reset the flags
   mem.endOfFile = false;   
   
   // Places the device in Read Array mode
   REGISTER_LOCK
   dataReg_->set(genReqWord(0xFF,0xFF));
   writeRegister(dataReg_, true); 
   REGISTER_UNLOCK   

   //read the entire mcs file
   while(1) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      // Check if this is the upper or lower byte
      if(!toggle) {
         toggle = true;
         fileData = (uint16_t)mem.data;
      } else {
         toggle = false;
         fileData |= ((uint16_t)mem.data << 8);
         promData = readFromFlash(address);                
         if(fileData != promData) {
            cout << "verifyBootProm error = ";
            cout << "invalid read back" <<  endl;
            cout << hex << "\taddress: 0x"  << address << endl;
            cout << hex << "\tfileData: 0x" << fileData << endl;
            cout << hex << "\tpromData: 0x" << promData << endl;
            mcsReader.close();
            return false;
         }
         address++;
         percentage = (((double)(address-promStartAddr_))/size)*100;
         percentage *= 2.0;//factor of two from two 8-bit reads for every write 16 bit write
         if( (percentage>=skim) && (percentage<100.0) ) {
            skim += 5.0;
            cout << "Verifying the PROM: " << dec << uint16_t(percentage) << " percent done" << endl;
         }         
      }
   }
   
   mcsReader.close();  
   cout << "Verification completed" << endl;
   cout << "*******************************************************************" << endl;   
   return true;
}

//! Compare the .mcs file with the PROM (true=matches)
bool AxiMicronP30::bufferedVerifyBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Verification ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   
   uint32_t address = 0;
   uint32_t index;  
   uint32_t retAddr;  
   uint16_t fileData;
   uint16_t promData[256];
   double size = double(promSize_);
   double percentage;
   double skim = 5.0; 
   bool   toggle = false;

   //check for valid file path
   if ( !mcsReader.open(filePath_) ) {
      mcsReader.close();
      cout << "mcsReader.close() = file path error" << endl;
      return(1);
   }

   if(promSize_ == 0x0){
      cout << "Invalid promSize_: 0x0" << endl;
      return false;      
   }   
   
   //reset the flags
   mem.endOfFile = false;   
   
   // Places the device in Read Array mode
   REGISTER_LOCK   
   dataReg_->set(genReqWord(0xFF,0xFF));
   writeRegister(dataReg_, true);
   REGISTER_UNLOCK   

   //read the entire mcs file
   while(1) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      // Calculate index
      index = address%256;
      
      // Check the index
      if( index == 0){
         
         // Get the new PROM data array
         while((retAddr = bufferedReadCommand(address,promData)) != address){
            if(retAddr==0x1){
               return false;
            }
         }             
      }        

      // Check if this is the upper or lower byte
      if(!toggle) {
         toggle = true;
         fileData = (uint16_t)mem.data;
      } else {
         toggle = false;
         fileData |= ((uint16_t)mem.data << 8);           
         if(fileData != promData[index]) {
            cout << "verifyBootProm error = ";
            cout << "invalid read back" <<  endl;
            cout << hex << "\taddress: 0x"  << address << endl;
            cout << hex << "\tfileData: 0x" << fileData << endl;
            cout << hex << "\tpromData: 0x" << promData[index] << endl;
            mcsReader.close();
            return false;
         }
         address++;
         percentage = (((double)(address))/size)*100;
         percentage *= 2.0;//factor of two from two 8-bit reads for every write 16 bit write
         if( (percentage>=skim) && (percentage<100.0) ) {
            skim += 5.0;
            cout << "Verifying the PROM: " << dec << uint16_t(percentage) << " percent done" << endl;
         }         
      }
   }
   
   mcsReader.close();  
   cout << "Verification completed" << endl;
   cout << "*******************************************************************" << endl;   
   return true;
}

//! Erase Command
void AxiMicronP30::eraseCommand(uint32_t address) {
   uint16_t status = 0;
   
   // Unlock the Block
   writeToFlash(address,0x60,0xD0);
   
   // Reset the status register
   writeToFlash(address,0x50,0x50);
   
   // Send the erase command
   writeToFlash(address,0x20,0xD0);
   
   while(1) {
      // Get the status register
      REGISTER_LOCK
      dataReg_->set(genReqWord(0x70,0xFF));
      writeRegister(dataReg_, true);
      addrReg_->set(READ_MASK | address);
      writeRegister(addrReg_, true);
      readRegister(readReg_);
      status = (uint16_t)(readReg_->get()&0xFFFF);
      REGISTER_UNLOCK
      
      // Check for erasing failure
      if ( (status&0x20) != 0 ) {
         cout << "failed, retrying" << endl;
      
         // Unlock the Block
         writeToFlash(address,0x60,0xD0);
         
         // Reset the status register
         writeToFlash(address,0x50,0x50);   
         
         // Send the erase command
         writeToFlash(address,0x20,0xD0);      
      
      // Check for FLASH not busy
      } else if ( (status&0x80) != 0 ) {
         break;
      }
   } 

   // Lock the Block
   writeToFlash(address,0x60,0x01);
}

//! Program Command
void AxiMicronP30::programCommand(uint32_t address, uint16_t data) {
   REGISTER_LOCK
   fastAddr_->set(address);
   writeRegister(fastAddr_, true);
   fastProg_->set((uint32_t)data);
   writeRegister(fastProg_, true);   
   REGISTER_UNLOCK
}

//! Generate request word 
uint32_t AxiMicronP30::genReqWord(uint16_t cmd, uint16_t data) {
   uint32_t readReq;
   readReq = ( ((uint32_t)cmd << 16) | ((uint32_t)data) );
   return readReq;
}

//! Generic FLASH write Command 
void AxiMicronP30::writeToFlash(uint32_t address, uint16_t cmd, uint16_t data) {
   REGISTER_LOCK
   // Set the data bus         
   dataReg_->set(genReqWord(cmd, data));
   writeRegister(dataReg_, true);
   
   // Set the address bus and initiate the transfer
   addrReg_->set(~READ_MASK & address);
   writeRegister(addrReg_, true);
   REGISTER_UNLOCK
}

//! Generic FLASH read Command
uint16_t AxiMicronP30::readFromFlash(uint32_t address) {
   uint32_t readData;
   
   // Set the address bus and initiate the transfer
   REGISTER_LOCK
   addrReg_->set(READ_MASK | address);
   writeRegister(addrReg_, true);
   
   // Read the data register
   readRegister(readReg_);
   readData = readReg_->get();
   REGISTER_UNLOCK
   
   // return the readout data
   return (uint16_t)(readData&0xFFFF);
}

//! Streaming Transmit Command
bool AxiMicronP30::bufferedProgramCommand(uint32_t baseAddr, uint32_t size, uint16_t *data) {
   uint32_t i;
   uint32_t sizeInWords = size+2;
   uint32_t buffer[sizeInWords];
   uint32_t linkConfig  = ((lane_&0xF) << 28) | ((vc_&0xF) << 24) | (linkConfig_&0xFF);
   uint32_t address     = ((0x1<<lane_) << 4)  | ((0x1<<vc_) << 0);
   
   // Check if lane is defined
   if(lane_==0xFF){
      cout << "AxiMicronP30.bufferedProgramCommand() = lane_ not defined " << endl;
      return(1);// ERROR    
   }

   // Check if vc is defined
   if(vc_==0xFF){
      cout << "AxiMicronP30.bufferedProgramCommand() = vc_ not defined " << endl;
      return(1);// ERROR    
   }

   // Load the based Address
   buffer[0] = baseAddr;
   
   // Load the size
   buffer[1] = size-1;
   
   // Load the data
   for(i=0;i<size;i++){
      buffer[i+2] = (uint32_t)data[i];
   }
   
   // Send the data
   system_->commLink()->queueDataTx(linkConfig,address,buffer,sizeInWords);
   return(0);
}

//! Buffered Read Command
uint32_t AxiMicronP30::bufferedReadCommand( uint32_t baseAddr, uint16_t *data ) {
   uint32_t i;  
   Data    *dat;
   uint32_t * buff;

   // Check if lane is defined
   if(lane_==0xFF){
      cout << "AxiMicronP30.bufferedReadCommand() = lane_ not defined " << endl;
      return 0x1;// ERROR   
   }

   // Check if vc is defined
   if(vc_==0xFF){
      cout << "AxiMicronP30.bufferedReadCommand() = vc_ not defined " << endl;
      return 0x1;// ERROR    
   }
   
   // Flush the data queue
   while( system_->commLink()->pollDataQueue() != NULL );
   
   // Set the bursting base address (start a burst ready)
   REGISTER_LOCK
   burstReg_->set(baseAddr);
   writeRegister(burstReg_, true); 
   REGISTER_UNLOCK

   while( ((dat = system_->commLink()->pollDataQueue(10000)) == NULL) ){
      // Set the bursting base address (start a burst ready)
      REGISTER_LOCK
      burstReg_->set(baseAddr);
      writeRegister(burstReg_, true);
      REGISTER_UNLOCK
   }
   
   // Check the size
   if( dat->size() != 257 ){
      //cout << "AxiMicronP30.bufferedReadCommand(): detected invalid dat->size() = " << dec << dat->size() << endl;
      return 0x2;// ERROR      
   }   
   
   // Get the data
   buff = dat->data();

   // Load the data
   for(i=0;i<256;i++){
      data[i] = (uint16_t)(buff[i+1]&0xFFFF);
   }   
   
   return buff[0];
}
