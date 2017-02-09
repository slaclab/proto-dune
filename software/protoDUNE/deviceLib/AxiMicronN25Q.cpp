//-----------------------------------------------------------------------------
// File          : AxiMicronN25Q.cpp
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
#include <MultDest.h>
#include <Data.h>
#include <System.h>
#include "AxiMicronN25Q.h"
#include "McsRead.h"

using namespace std;

#define CMD_OFFSET 16

#define WRITE_3BYTE_CMD    (0x02 << CMD_OFFSET)
#define WRITE_4BYTE_CMD    (0x12 << CMD_OFFSET)

#define READ_3BYTE_CMD     (0x03 << CMD_OFFSET)
#define READ_4BYTE_CMD     (0x13 << CMD_OFFSET)
   
#define FLAG_STATUS_REG    (0x70 << CMD_OFFSET)
#define FLAG_STATUS_RDY    (0x80)
   
#define WRITE_ENABLE_CMD   (0x06 << CMD_OFFSET)
#define WRITE_DISABLE_CMD  (0x04 << CMD_OFFSET)

#define ADDR_ENTER_CMD     (0xB7 << CMD_OFFSET)
#define ADDR_EXIT_CMD      (0xE9 << CMD_OFFSET)

#define ENABLE_RESET_CMD   (0x66 << CMD_OFFSET)
#define RESET_CMD          (0x99 << CMD_OFFSET)
   
#define ERASE_CMD          (0xD8 << CMD_OFFSET)
#define ERASE_SIZE         (0x10000)

#define STATUS_REG_WR_CMD  (0x01 << CMD_OFFSET)
#define STATUS_REG_RD_CMD  (0x05 << CMD_OFFSET)

#define DEV_ID_RD_CMD      (0x9F << CMD_OFFSET)

#define WRITE_NONVOLATILE_CONFIG (0xB1 << CMD_OFFSET)
#define WRITE_VOLATILE_CONFIG    (0x81 << CMD_OFFSET)
#define READ_NONVOLATILE_CONFIG  (0xB5 << CMD_OFFSET)
#define READ_VOLATILE_CONFIG     (0x85 << CMD_OFFSET)

// Default Configuration:
// Number of dummy clock cycles = 0xF
// XIP mode at power-on reset = 0x7
// Output driver strength = x7
// Double transfer rate protocol = 0x1 ( = Disabled and only used in MT25Q only)
// Reset/hold = 0x0 (disabled to be MT25Q pin compatible with N25Q)
// Quad I/O protocol = 0x1
// Dual I/O protocol = 0x1
// 128Mb segment select = 0x1
#define DEFAULT_3BYTE_CONFIG     0xFFEF
#define DEFAULT_4BYTE_CONFIG     0xFFEE

#define READ_MASK   0x00000000 
#define WRITE_MASK  0x80000000 
#define VERIFY_MASK 0x40000000 

// Constructor
AxiMicronN25Q::AxiMicronN25Q (uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) :
      Device(linkConfig, baseAddress, "AxiMicronN25Q", index, parent) {
   // Declare the registers
   addRegisterLink(new RegisterLink("Test", baseAddress + 0x0*addrSize, Variable::Configuration));   
   addr32BitReg_ = new Register("addr32BitMode",baseAddress + (0x01 * addrSize)); 
   addrReg_      = new Register("ADDR",         baseAddress + (0x02 * addrSize)); 
   cmdReg_       = new Register("CMD",          baseAddress + (0x03 * addrSize));
   dataReg_      = new Register("DATA",         baseAddress + (0x80 * addrSize),64);
      
   // Default PROM size = 0x0 (must be set using setPromSize() )
   promSize_      = 0x0;
   
   // Default PROM size = 0x0 (must be set using setFilePath() )
   promStartAddr_ = 0x0;
   
   // Default file path = NULL (must be set using setFilePath() )
   filePath_ = "";

   // Default to 24-bit address mode
   addr32BitMode_ = false;
   
   // Default for optional streaming lane_ and vc_ pointers
   lane_ = 0xFF;
   vc_   = 0xFF;
}

// Deconstructor
AxiMicronN25Q::~AxiMicronN25Q ( ) { 
   delete addr32BitReg_;
   delete addrReg_;
   delete cmdReg_;
   delete dataReg_;
}

void AxiMicronN25Q::setPromSize (uint32_t promSize) {
   promSize_ = promSize;
}

uint32_t AxiMicronN25Q::getPromSize (string pathToFile) {
   McsRead mcsReader;
   uint32_t retVar;
   mcsReader.open(pathToFile);
   printf("Calculating PROM file (.mcs) Memory Address size ...");    
   retVar = mcsReader.addrSize();
   printf("PROM Size = 0x%08x\n", retVar); 
   mcsReader.close();
   return retVar; 
}

void AxiMicronN25Q::setFilePath (string pathToFile) {
   filePath_ = pathToFile;
   McsRead mcsReader;   
   mcsReader.open(filePath_);
   promStartAddr_ = mcsReader.startAddr();
   mcsReader.close();   
   resetFlash();     
}

//! Set the address mode
void AxiMicronN25Q::setAddr32BitMode (bool addr32BitMode) {
   addr32BitMode_ = addr32BitMode;
   if(addr32BitMode_){
      enter32BitMode();
   } else {
      exit32BitMode();
   }
   REGISTER_LOCK
   addr32BitReg_->set((uint32_t)addr32BitMode_);
   writeRegister(addr32BitReg_, true);    
   REGISTER_UNLOCK
}

//! Enter 4-BYTE ADDRESS MODE Command
void AxiMicronN25Q::enter32BitMode( ) {   
   // Send the enter command
   setCmd(WRITE_MASK|ADDR_ENTER_CMD);     
}

//! Exit 4-BYTE ADDRESS MODE Command
void AxiMicronN25Q::exit32BitMode( ) {   
   // Send the exit command
   setCmd(WRITE_MASK|ADDR_EXIT_CMD);      
}

//! Set the non-volatile status register 
void AxiMicronN25Q::setPromStatusReg(uint8_t value) {   
   if(addr32BitMode_){
      setAddr(((uint32_t)value)<<24); 
      setCmd(WRITE_MASK|STATUS_REG_WR_CMD|0x1);     
   } else {
      setAddr(((uint32_t)value)<<16); 
      setCmd(WRITE_MASK|STATUS_REG_WR_CMD|0x1);          
   }   
   waitForFlashReady();    
}

uint8_t AxiMicronN25Q::getPromStatusReg(){  
   waitForFlashReady();
   setCmd(READ_MASK|STATUS_REG_RD_CMD|0x1); 
   REGISTER_LOCK
   readRegister(cmdReg_);
   REGISTER_UNLOCK
   return (uint8_t)(cmdReg_->get()&0xFF);   
}

uint8_t AxiMicronN25Q::getManufacturerId(){  
   waitForFlashReady();
   setCmd(READ_MASK|DEV_ID_RD_CMD|0x1); 
   REGISTER_LOCK
   readRegister(cmdReg_);
   REGISTER_UNLOCK
   return (uint8_t)(cmdReg_->get()&0xFF);       
}

uint8_t AxiMicronN25Q::getManufacturerType(){  
   waitForFlashReady();
   setCmd(READ_MASK|DEV_ID_RD_CMD|0x2); 
   REGISTER_LOCK
   readRegister(cmdReg_);
   REGISTER_UNLOCK
   return (uint8_t)(cmdReg_->get()&0xFF);      
}        

uint8_t AxiMicronN25Q::getManufacturerCapacity(){  
   waitForFlashReady();
   setCmd(READ_MASK|DEV_ID_RD_CMD|0x3); 
   REGISTER_LOCK
   readRegister(cmdReg_);
   REGISTER_UNLOCK
   return (uint8_t)(cmdReg_->get()&0xFF);      
}    

void AxiMicronN25Q::setLane (uint32_t lane) {
   lane_ = lane;
}

void AxiMicronN25Q::setVc (uint32_t vc) {
   vc_ = vc;
}

void AxiMicronN25Q::setTDest (uint32_t tDest) {
   lane_ = (tDest >> 4)&0xF;
   vc_   = (tDest >> 0)&0xF;
}

//! Check if file exist (true=exists)
bool AxiMicronN25Q::fileExist ( ) {
  ifstream ifile(filePath_.c_str());
  return ifile.good();
}

//! Print Power Cycle Reminder
void AxiMicronN25Q::rebootReminder ( bool pwrCycleReq ) {
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
void AxiMicronN25Q::eraseBootProm ( ) {

   uint32_t address = promStartAddr_;
   double size = double(promSize_);
   double percentage;
   double skim = 5.0; 

   cout << "*******************************************************************" << endl;   
   cout << "Starting Erasing ..." << endl; 
   while(address<(promStartAddr_+promSize_)) {        
      /*
      // Print the status to screen
      cout << hex << "Erasing PROM from 0x" << address << " to 0x" << (address+ERASE_SIZE-1);
      cout << setprecision(3) << " ( " << ((double(address))/size)*100 << " percent done )" << endl;      
      */
      
      // execute the erase command
      eraseCommand(address);
      
      //increment the address pointer
      address += ERASE_SIZE;
      
      percentage = (((double)address-promStartAddr_)/size)*100;
      if( (percentage>=skim) && (percentage<100.0) ) {
         skim += 5.0;
         cout << "Erasing the PROM: " << floor(percentage) << " percent done" << endl;
      }               
   }   
   cout << "Erasing completed" << endl;
}

//! Erase Command
void AxiMicronN25Q::eraseCommand(uint32_t address) {    
   // Set the address
   setAddr(address); 
   // Send the erase command      
   if(addr32BitMode_){   
      setCmd(WRITE_MASK|ERASE_CMD|0x4);     
   }else{
      setCmd(WRITE_MASK|ERASE_CMD|0x3);  
   }           
}

//! Write the .mcs file to the PROM
bool AxiMicronN25Q::writeBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Writing ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   
   uint32_t fileData;
   double size = double(promSize_);
   double percentage;
   double skim = 1.0;    
   uint32_t byteCnt = 0;    
   uint32_t wordCnt = 0;    
   uint32_t baseAddr = 0;
   uint32_t cnt     = 0;     

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
   while(cnt < promSize_) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      if( (byteCnt==0) && (wordCnt==0) ) {
         baseAddr = mem.address;
      }
      if( byteCnt==0 ){
         fileData = ((uint32_t)(mem.data&0xFF)) << (8*(3-byteCnt));
      }else{
         fileData |= ((uint32_t)(mem.data&0xFF)) << (8*(3-byteCnt));
      }
      
      cnt++;
      byteCnt++;
      if(byteCnt==4){
         byteCnt = 0;
         data_[wordCnt] = fileData;
         wordCnt++;
         if(wordCnt==64){
            wordCnt = 0;
            setData();
            writeCommand(baseAddr);
         }                 
      }
      
      percentage = (((double)(mem.address-promStartAddr_))/size)*100;
      if( (percentage>=skim) && (percentage<100.0) ) {
         skim += 5.0;
         cout << "Writing the PROM: " << dec << round(percentage)-1 << " percent done" << endl;
      }       
   }
   
   // Check for the end of the buffer
   if( (byteCnt!=0) || (wordCnt!=0) ){
      while( (byteCnt!=0) || (wordCnt!=0) ){
         // Pad with ones
         if( byteCnt==0 ){
            fileData = 0xFF << (8*(3-byteCnt));
         }else{
            fileData |= 0xFF << (8*(3-byteCnt));
         }         
         byteCnt++;
         if(byteCnt==4){
            byteCnt = 0;
            data_[wordCnt] = fileData;
            wordCnt++;            
            if(wordCnt==64){
               wordCnt = 0;
               setData();
               writeCommand(baseAddr);
            }                 
         }         
      }
   }

   mcsReader.close();   
   cout << "Writing completed" << endl;   
   return true;
}

//! Write the .mcs file to the PROM
bool AxiMicronN25Q::bufferedWriteBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Writing ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   
   uint32_t baseAddr;  
   uint32_t byteCnt     = 0;    
   uint8_t  fileData[256];
   double   size = double(promSize_);
   double   percentage;
   double   skim = 1.0;  
   uint32_t cnt     = 0;     
   
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
   while(cnt < promSize_) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      if( byteCnt==0 ) {
         baseAddr = mem.address;
      }
      
      fileData[byteCnt] = (uint8_t)(mem.data & 0xFF);

      cnt++;
      byteCnt++;
      if(byteCnt==256){
         byteCnt = 0;
         bufferedWriteCommand(baseAddr,fileData);              
      }
      
      percentage = (((double)(mem.address-promStartAddr_))/size)*100;
      if( (percentage>=skim) && (percentage<100.0) ) {
         skim += 5.0;
         cout << "Writing the PROM: " << dec << round(percentage)-1 << " percent done" << endl;
      }       
   }
   
   // Check for the end of the buffer
   if( byteCnt!=0 ){
      while( byteCnt!=0 ){
         // Pad with ones
         fileData[byteCnt] = 0xFF;       
         byteCnt++;
         if(byteCnt==256){
            byteCnt = 0;
            bufferedWriteCommand(baseAddr,fileData);              
         }       
      }
   }
   
   // Verify wont work after buffered program unless a bogus readCommand is called here
   // I have no idea why.
   readCommand(0);   

   mcsReader.close();   
   cout << "Writing completed" << endl;   
   return true;
}

//! Write Command
void AxiMicronN25Q::writeCommand(uint32_t address) {   
   // Set the address
   setAddr(address); 
   // Send the write command      
   if(addr32BitMode_){   
      setCmd(WRITE_MASK|WRITE_4BYTE_CMD|0x104);     
   }else{
      setCmd(WRITE_MASK|WRITE_3BYTE_CMD|0x103);  
   }    
}
//! Streaming Transmit Command
bool AxiMicronN25Q::bufferedWriteCommand(uint32_t baseAddr, uint8_t *data) {
   uint32_t i;
   uint32_t buffer[257];
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

   // Load the data
   for(i=0;i<256;i++){
      buffer[i+1] = (uint32_t)data[i];
   }
   
   // Send the data
   system_->commLink()->queueDataTx(linkConfig,address,buffer,257);   
   return(0);
}

//! Compare the .mcs file with the PROM (true=matches)
bool AxiMicronN25Q::verifyBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Verification ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   
   uint8_t promData,fileData;
   double size = double(promSize_);
   double percentage;
   double skim = 5.0; 
   uint32_t byteCnt = 0;    
   uint32_t wordCnt = 0;  
   uint32_t cnt     = 0;  

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

   //read the entire mcs file
   while(cnt < promSize_) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      fileData = (uint8_t)(mem.data & 0xFF);
      if( (byteCnt==0) && (wordCnt==0) ){ 
         readCommand(mem.address);
         getData();
      }
      promData = (uint8_t)( (data_[wordCnt] >> 8*(3-byteCnt)) & 0xFF);
      cnt++;
      if(fileData != promData) {
         cout << "verifyBootProm error = ";
         cout << "invalid read back" <<  endl;
         cout << hex << "\taddress: 0x"  << mem.address << endl;
         cout << hex << "\tfileData: 0x" << (uint32_t)fileData << endl;
         cout << hex << "\tpromData: 0x" << (uint32_t)promData << endl;
         mcsReader.close();
         return false;
      }      
      
      byteCnt++;
      if(byteCnt==4){
         byteCnt = 0;
         wordCnt++;
         if(wordCnt==64){
            wordCnt = 0;
         }                 
      }      
      
      percentage = (((double)(mem.address-promStartAddr_))/size)*100;
      if( (percentage>=skim) && (percentage<100.0) ) {
         skim += 5.0;
         cout << "Verifying the PROM: " << dec << uint16_t(percentage) << " percent done" << endl;
      }       
   }
   
   mcsReader.close();  
   cout << "Verification completed" << endl;
   cout << "*******************************************************************" << endl;   
   return true;
}

//! Compare the .mcs file with the PROM (true=matches)
bool AxiMicronN25Q::bufferedVerifyBootProm ( ) {
   cout << "*******************************************************************" << endl;
   cout << "Starting Verification ..." << endl; 
   McsRead mcsReader;
   McsReadData mem;
   
   uint32_t byteCnt = 0;    
   uint8_t  fileData;
   uint8_t  promData[256];  
   uint32_t retAddr;  
   uint32_t cnt     = 0;      

   double size = double(promSize_);
   double percentage;
   double skim = 5.0;  

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

   //read the entire mcs file
   while(cnt < promSize_) {
   
      //read a line of the mcs file
      if (mcsReader.read(&mem)<0){
         cout << "mcsReader.close() = line read error" << endl;
         mcsReader.close();
         return false;
      }
      if ( mem.endOfFile ){
         break;
      }
      
      // Latch the .MCS file data
      fileData = (uint8_t)(mem.data & 0xFF);
      
      // Check if we need to readout the firmware
      if( byteCnt==0 ) {
         // Get the new PROM data array
         while((retAddr = bufferedReadCommand(mem.address,promData)) != mem.address){
            if(retAddr==0x1){
               return false;
            }
         }
      }
       
      if(fileData != promData[byteCnt]) {
         cout << "verifyBootProm error = ";
         cout << "invalid read back" <<  endl;
         cout << hex << "\taddress: 0x"  << mem.address << endl;
         cout << hex << "\tfileData: 0x" << (uint32_t)fileData << endl;
         cout << hex << "\tpromData: 0x" << (uint32_t)promData[byteCnt] << endl;
         mcsReader.close();
         return false;
      }      
      
      cnt++;
      byteCnt++;
      if(byteCnt==256){
         byteCnt = 0;                
      }      
      
      percentage = (((double)(mem.address-promStartAddr_))/size)*100;
      if( (percentage>=skim) && (percentage<100.0) ) {
         skim += 5.0;
         cout << "Verifying the PROM: " << dec << uint16_t(percentage) << " percent done" << endl;
      }       
   }
   
   mcsReader.close();  
   cout << "Verification completed" << endl;
   cout << "*******************************************************************" << endl;   
   return true;
}

//! Read Command
void AxiMicronN25Q::readCommand(uint32_t address) {  
   //Wait while the Flash is busy.
   waitForFlashReady();   
   
   // Set the address
   setAddr(address); 
   
   // Send the read command      
   if(addr32BitMode_){   
      setCmd(READ_MASK|READ_4BYTE_CMD|0x104);     
   }else{
      setCmd(READ_MASK|READ_3BYTE_CMD|0x103);  
   }          
}

//! Buffered Read Command
uint32_t AxiMicronN25Q::bufferedReadCommand( uint32_t baseAddr, uint8_t *data ) {
   uint32_t  i;  
   Data     *dat;
   uint32_t *buff;
   
   // Check if lane is defined
   if(lane_==0xFF){
      cout << "AxiMicronN25Q.bufferedReadCommand() = lane_ not defined " << endl;
      return 0x1;// ERROR    
   }

   // Check if vc is defined
   if(vc_==0xFF){
      cout << "AxiMicronN25Q.bufferedReadCommand() = vc_ not defined " << endl;
      return 0x1;// ERROR     
   }
   
   // Set the address
   setAddr(baseAddr);        
      
   // Flush the data queue
   while( system_->commLink()->pollDataQueue() != NULL );

   // Send the read command      
   if(addr32BitMode_){   
      setCmd(READ_MASK|VERIFY_MASK|READ_4BYTE_CMD|0x104);     
   }else{
      setCmd(READ_MASK|VERIFY_MASK|READ_3BYTE_CMD|0x103);  
   }   
   
   while( ((dat = system_->commLink()->pollDataQueue(10000)) == NULL) ){
      // Send the read command      
      if(addr32BitMode_){   
         setCmd(READ_MASK|VERIFY_MASK|READ_4BYTE_CMD|0x104);     
      }else{
         setCmd(READ_MASK|VERIFY_MASK|READ_3BYTE_CMD|0x103);  
      }    
   }

   // Check the size
   if( dat->size() != 257 ){
      return 0x2;// ERROR      
   }   
   
   // Get the data
   buff = dat->data();

   // Load the data
   for(i=0;i<256;i++){
      data[i] = (uint8_t)(buff[i+1]&0xFF);
   }   
   
   return buff[0];
}

//! Reset the FLASH memory Command
void AxiMicronN25Q::resetFlash ( ) {
   REGISTER_LOCK
   // Send the enable reset command
   cmdReg_->set(WRITE_MASK|ENABLE_RESET_CMD); 
   writeRegister(cmdReg_, true);   
   
   // Send the reset command
   cmdReg_->set(WRITE_MASK|RESET_CMD); 
   writeRegister(cmdReg_, true);   
   
   // Set the firmware register
   addr32BitReg_->set((uint32_t)addr32BitMode_);
   writeRegister(addr32BitReg_, true);   
   REGISTER_UNLOCK
   
   // Check the address mode
   if(addr32BitMode_){
      enter32BitMode();
      setAddr(DEFAULT_4BYTE_CONFIG<<16); 
      setCmd(WRITE_MASK|WRITE_NONVOLATILE_CONFIG|0x2);
      waitForFlashReady();      
      setCmd(WRITE_MASK|WRITE_VOLATILE_CONFIG|0x2);         
   } else {
      exit32BitMode();
      setAddr(DEFAULT_3BYTE_CONFIG<<8); 
      setCmd(WRITE_MASK|WRITE_NONVOLATILE_CONFIG|0x2);         
      waitForFlashReady();
      setCmd(WRITE_MASK|WRITE_VOLATILE_CONFIG|0x2);         
   }
}

//! Enable Write commands
void AxiMicronN25Q::writeEnable ( ) {
   //Wait while the Flash is busy.
   waitForFlashReady();   
   
   // Enable Writes
   REGISTER_LOCK
   cmdReg_->set(WRITE_MASK|WRITE_ENABLE_CMD);
   writeRegister(cmdReg_, true); 
   REGISTER_UNLOCK
}

//! Disable Write commands
void AxiMicronN25Q::writeDisable ( ) {
   //Wait while the Flash is busy.
   waitForFlashReady();   
   
   // Disable Writes
   REGISTER_LOCK
   cmdReg_->set(WRITE_MASK|WRITE_DISABLE_CMD); 
   writeRegister(cmdReg_, true);   
   REGISTER_UNLOCK   
}

//! Wait for the FLASH memory to not be busy
void AxiMicronN25Q::waitForFlashReady ( ) { 
	while((statusReg() & FLAG_STATUS_RDY) == 0);
}

//! Pull the status register
uint32_t AxiMicronN25Q::statusReg ( ) {  
   setCmd(READ_MASK|FLAG_STATUS_REG|0x1); 
   REGISTER_LOCK
   readRegister(cmdReg_);
   REGISTER_UNLOCK
   return cmdReg_->get();  
}

//! Set the address register
void AxiMicronN25Q::setAddr (uint32_t value) {
   REGISTER_LOCK
   addrReg_->set(value);
   writeRegister(addrReg_, true);
   REGISTER_UNLOCK
}

//! Set the command register
void AxiMicronN25Q::setCmd (uint32_t value) {
   // Check for write command and not write disable command
   if ( value&WRITE_MASK ){
      writeEnable(); 
   }
   // Send the command
   REGISTER_LOCK
   cmdReg_->set(value);
   writeRegister(cmdReg_, true); 
   REGISTER_UNLOCK
}

//! Set the data register
void AxiMicronN25Q::setData () {
   REGISTER_LOCK
   dataReg_->setData(data_);
   writeRegister(dataReg_, true); 
   REGISTER_UNLOCK
}

//! Get the data register
void AxiMicronN25Q::getData () {
   uint32_t i;
   REGISTER_LOCK
   readRegister(dataReg_);
   for(i=0;i<64;i++){
      data_[i] = dataReg_->data()[i];
   }
   REGISTER_UNLOCK   
}

//! Block Read of PROM (independent of .MCS file)
void AxiMicronN25Q::readBootProm (uint32_t address, uint32_t *data){
   uint32_t i;
   readCommand(address);
   getData();
   for(i=0;i<64;i++){
      data[i] = data_[i];
   }
}   
