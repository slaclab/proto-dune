//-----------------------------------------------------------------------------
// File          : DataCompression.cpp
// Author        : JJRussell <jrussell@slac.stanford.edu>
// Created       : 2016/10.13
// Project       : 
//-----------------------------------------------------------------------------
// Description :  Container for Waveform Extraction constants
//
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
// Date        WHO   WHAT
// ----------  ---   ----------------------------------------------------------
// 2016/10/13  jjr   Added status variables    
//-----------------------------------------------------------------------------


#include <DataCompression.h>
#include <xdunedatacompressioncore_hw.h>

#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>

#include <string>


using namespace std;


/* ---------------------------------------------------------------------- */
/* LOCAL PROTOTYPES                                                       */
/* ---------------------------------------------------------------------- */
//static void addStatusRegisterLinks (DataCompression *device, 
//                                   uint32_t    baseAddress,
//                                   uint32_t       addrSize);
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Constuctor for the DataCompression HLS module. This adds the 
          HLS status registers and enables the HLS module.

   \param[in]  linkConfig  
   \param[in] baseAddress  Base address of the HLS status and control
                           registers
   \param[in]       index  The HLS module index. Currently there are 2
                           such modules, each servicing 128 channels
   \param[in]      parent  The parent in the device tree
   \param[in]    addrSize  The unit of addressing. This is almost always
                           4 bytes (\e i.e. a 32-bit integer) and is 
                           somewhat obsolete.
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::DataCompression(uint32_t  linkConfig, 
                                 uint32_t baseAddress,
                                 uint32_t       index,
                                 Device       *parent,
                                 uint32_t    addrSize) : 
   Device  (linkConfig, baseAddress, "DataCompression", index, parent),
   sr_ (this, baseAddress, addrSize)
{
   // Description
   desc_ = "DataCompression object";
   

   //addStatusRegisterLinks (this, baseAddress, addrSize);
  
   // Enable polling
   pollEnable_ = true;   

   enableHLSmodule (addrSize);

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief Adds all the status registers used in monitoring the HLS module

   \param[in]      device  The device representing the DataCompression modules
   \param[in] baseAddress  Base address of the HLS status and control
                           registers
   \param[in]    addrSize  The unit of addressing. This is almost always
                           4 bytes (\e i.e. a 32-bit integer) and is 
                           somewhat obsolete.
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::StatusRegisters::StatusRegisters (Device       *device,
                                                   uint32_t baseAddress,
                                                   uint32_t    addrSize)
{
   /* Encapsulates the register definition and configuration              */
   struct Svd_t
   {
      char const  *name;  /* The name of the register                     */
      uint32_t   offset;  /* Offset from the base address                 */
      uint8_t       n32;  /* The number 32 bit words serviced             */ 
      bool   pollEnable;  /* Flag to enable the polling of this register  */
   };


   /* 
    |  There are 4 sections the HLS status monitor block
    |        Common:  This acts kind like header information
    |        Cfg   :  The HLS module's configuration information
    |        Read  :  Counters and status words that monitor the frame reads
    |        Write :  Counters and status words that monitor the frame writes
   */
   #define STATUS_COMMON(_offset) ((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_STATUS_COMMON_PATTERN_DATA >> 2) + _offset)
   #define STATUS_CFG(_offset)    ((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_STATUS_CFG_DATA            >> 2) + _offset)
   #define STATUS_READ(_offset)   ((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_STATUS_READ_DATA           >> 2) + _offset)
   #define STATUS_WRITE(_offset)  ((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_STATUS_WRITE_DATA          >> 2) + _offset) 
 
   /*
    |  The register names are rather contrived so that the appear 
    |  somewhat in logical order that groups the registers in the
    |  various sections together.  (Unfortunately the ordering in
    |  gui is alphabetical.)
   */
   static const Svd_t Svd[] =
   {
      //  INDEX           NAME            OFFSET             N32  POLLENABLE
      //  ---------       ------------    -----------------  ---  ----------
      [Ca_Pattern  ] = { "Ca_Pattern",    STATUS_COMMON (0),   1,      true },

      [Cfg_Mode    ] = { "Cfg_Mode",      STATUS_CFG    (0),   1,      true },
      [Cfg_NCfgs   ] = { "Cfg_NCfgs",     STATUS_CFG    (1),   1,      true },

      [Rd_Status   ] = { "Rd_Status",     STATUS_READ   (0),   1,      true },
      [Rd_NFrames  ] = { "Rd_NFrames",    STATUS_READ   (1),   1,      true },
      [Rd_NNormal  ] = { "Rd_NNormal",    STATUS_READ   (2),   1,      true },
      [Rd_NDisabled] = { "Rd_NDisabled",  STATUS_READ   (3),   1,      true },
      [Rd_NFlush   ] = { "Rd_NFlush",     STATUS_READ   (4),   1,      true },
      [Rd_NDisFlush] = { "Rd_NDisFlush",  STATUS_READ   (5),   1,      true },
      [Rd_NErrSofM ] = { "Rd_NErrSofM",   STATUS_READ   (6),   1,      true },
      [Rd_NErrSofU ] = { "Rd_NErrSofU",   STATUS_READ   (7),   1,      true },
      [Rd_NErrEofM ] = { "Rd_NErrEofM",   STATUS_READ   (8),   1,      true },
      [Rd_NErrEofU ] = { "Rd_NErrEofU",   STATUS_READ   (9),   1,      true },
      [Rd_NErrEofE ] = { "Rd_NErrEofE",   STATUS_READ  (10),   1,      true },
      [Rd_NErrK28_5] = { "Rd_NErrK28_5",  STATUS_READ  (11),   1,      true },
      [Rd_NErrSeq  ] = { "Rd_NErrSeq",    STATUS_READ  (12),   1,      true },

      [Wr_NWrote   ] = { "Wr_NWrote",     STATUS_WRITE  (0),   1,      true },
   };


   unsigned int cnt = sizeof (Svd) / sizeof (Svd[0]);
   for (unsigned int idx = 0; idx < cnt; idx++)
   {
      Svd_t const &svd = Svd[idx];

      RegisterLink *rl = new RegisterLink (svd.name, 
                                           baseAddress + (svd.offset * addrSize), 
                                           svd.n32, 
                                           Variable::Status);

      rl_[idx] = rl;

      device->addRegisterLink (rl);
      rl->setPollEnable (svd.pollEnable);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Enables the HLS Data Compression module

   \param[in]    addrSize  The unit of addressing. This is almost always
                           4 bytes (\e i.e. a 32-bit integer) and is 
                           somewhat obsolete.
                                                                          */
/* ---------------------------------------------------------------------- */
void DataCompression::enableHLSmodule (uint32_t addrSize)
{
   uint32_t address = baseAddress_ 
                    + (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_AP_CTRL>>2)*addrSize;   
 
   Register *r = new Register ("AxiLiteCtrl", address);
   addRegister (r);

   REGISTER_LOCK
      r->set        (0x81); 
      writeRegister (r, true); 
      readRegister  (r);
   REGISTER_UNLOCK     
   
   return;
}
/* ---------------------------------------------------------------------- */



// Deconstructor
DataCompression::~DataCompression ( ) { }

// command
void DataCompression::command(string name, string arg) {
   Device::command(name, arg);
}

// Hard Reset
void DataCompression::hardReset () {
   Device::hardReset();
}

// Soft Reset
void DataCompression::softReset () {
   Device::softReset();
}

// Count Reset
void DataCompression::countReset () {
   Device::countReset();
}

// Method to read status registers and update variables
void DataCompression::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void DataCompression::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void DataCompression::writeConfig ( bool force ) {  
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void DataCompression::verifyConfig ( ) {
   Device::verifyConfig();
}
