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
// 2018/03/16  jjr   Added a statistics to monitor possible read errors and
//                   many other quantities.
// 2016/10/13  jjr   Added status variables    
//-----------------------------------------------------------------------------

#ifdef ARM

#include <DataCompression.h>
#include <xdunedatacompressioncore_hw.h>

#include <RegisterLink.h>

#include <string>

/* ---------------------------------------------------------------------- *//*!
   \brief Encapsulates the register definition and configuration
                                                                          */
/* ---------------------------------------------------------------------- */
struct Svd_t
{
   char const  *name;  /* The name of the register                        */
   uint32_t   offset;  /* Offset from the base address                    */
   uint8_t       n32;  /* The number 32 bit words serviced                */ 
   bool   pollEnable;  /* Flag to enable the polling of this register    */
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
/* LOCAL PROTOTYPES                                                       */
/* ---------------------------------------------------------------------- */
static void populate (RegisterLink *rlArray[],
                      Svd_t const        *svd, 
                      int                 cnt,
                      Device          *device,
                      uint32_t    baseAddress, 
                      uint32_t       addrSize);
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *\
 |                                                                        |
 |  There are 4 sections the HLS status monitor block                     |
 |        Common:  This acts kind like header information                 |
 |        Cfg   :  The HLS module's configuration information             |
 |        Read  :  Counters and status words that monitor the frame reads |
 |        Write :  Counters and status words that monitor the frame writes|
 |                                                                        |
\* ---------------------------------------------------------------------- */
/*
#define MONITOR_COMMON_PATTERN  (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_COMMON_PATTERN_DATA >> 2)
#define MONITOR_CFG_MODE        (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_CFG_MODE_DATA       >> 2)
#define MONITOR_CFG_NCFGS       (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_CFG_NCFGS_DATA      >> 2)

#define MONITOR_READ_MASK       (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_MASK_V_DATA    >> 2)
#define MONITOR_READ_NFRAMES    (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_NFRAMES_DATA   >> 2)
*/


#define MONITOR_COMMON_PATTERN  (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_COMMON_PATTERN_DATA >> 2)

#define MONITOR_CFG(_offset)          \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_CFG_DATA          >> 2)+_offset)

#define MONITOR_READ_SUMMARY(_offset) \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_SUMMARY_DATA >> 2)+_offset)

#define MONITOR_READ_NSTATES(_offset) \
   ((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_SUMMARY_DATA >> 2)+(2+_offset))

#define MONITOR_READ_ERRS(_offset)    \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_ERRS_DATA    >> 2)+_offset)

#define MONITOR_READ_NFRAMEERRS(_offset)    \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_ERRS_DATA    >> 2)+_offset)

#define MONITOR_READ_NWIBERRS(_offset)    \
   ((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_ERRS_DATA >> 2)+ (5+_offset))


#define MONITOR_WRITE(_offset)         \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_WRITE_DATA        >> 2)+_offset)


/*
#define MONITOR_READ_NWIBERRS(_offset)   \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_NWIBERRS_BASE  >> 2)+_offset)

#define MONITOR_READ_NSTATES(_offset)   \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_NSTATES_BASE   >> 2)+_offset)

#define MONITOR_READ_NFRAMEERRS(_offset)   \
((XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_READ_NFRAMEERRS_BASE >> 2)+_offset)
*/

#define MONITOR_WRITE_NBYTES    (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_WRITE_NBYTES_DATA    >> 2)
#define MONITOR_WRITE_NPROMOTED (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_WRITE_NPROMOTED_DATA >> 2)
#define MONITOR_WRITE_NDROPPED  (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_WRITE_NDROPPED_DATA  >> 2)
#define MONITOR_WRITE_NPACKETS  (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_MONITOR_WRITE_NPACKETS_DATA  >> 2)
/* ---------------------------------------------------------------------- */




/* ====================================================================== */
/* BEGIN: DataCompression                                                 */
/* ---------------------------------------------------------------------- *//*!
   \brief Constuctor for the DataCompression HLS module. This adds the 
          HLS monitor registers and enables the HLS module.
   \param[in]  linkConfig  
   \param[in] baseAddress  Base address of the HLS monitor and control
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
   sr_ (this, baseAddress, addrSize),

   m_read  (DataCompression::Read  (linkConfig, baseAddress, 0, this, 4)),
   m_write (DataCompression::Write (linkConfig, baseAddress, 0, this, 4))
{
   // Description
   desc_ = "DataCompression object";


   addDevice (&m_read);  m_read .pollEnable (true);
   addDevice (&m_write); m_write.pollEnable (true);


   // Enable polling
   pollEnable_ = true;   

   enableHLSmodule (addrSize);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
   \brief Adds all the monitor registers used in monitoring the HLS module
   \param[in]      device  The device representing the DataCompression modules
   \param[in] baseAddress  Base address of the HLS monitor and control
                           registers
   \param[in]    addrSize  The unit of addressing. This is almost always
                           4 bytes (\e i.e. a 32-bit integer) and is 
                           somewhat obsolete.
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Registers::Registers (Device       *device,
                                       uint32_t baseAddress,
                                       uint32_t    addrSize)
{
   /*
    |  The register names are rather contrived so that the appear 
    |  somewhat in logical order that groups the registers in the
    |  various sections together.  (Unfortunately the ordering in
    |  gui is alphabetical.)
   */
   static const Svd_t Svd[] =
   {
      //  INDEX          NAME         OFFSET                  N32  POLLENABLE
      //  --------       ------------ ----------------        ---  ----------
      [Ca_Pattern] = { "Ca_Pattern",  MONITOR_COMMON_PATTERN,   1,      true },
      [Cfg_Mode  ] = { "Cfg_Mode",    MONITOR_CFG (0)       ,   1,      true },
      [Cfg_NCfgs ] = { "Cfg_NCfgs",   MONITOR_CFG (1)       ,   1,      true },
   };


   static unsigned int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);
 
   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Destruction for the Data Compression Monitor Registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompression                                                   */
/* ====================================================================== */





/* ====================================================================== */
/* BEGIN: DataCompression:Read                                      */
/* ---------------------------------------------------------------------- *//*!
  \brief Initialize the Data Compression Monitor Read Device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::Read (uint32_t linkConfig, 
                             uint32_t baseAddress, 
                             uint32_t       index,
                             Device       *parent,
                             uint32_t    addrSize) :
   Device      (linkConfig, baseAddress, "Read", index, parent),
   sr_         (this, baseAddress, addrSize),

   m_wibErrs   (linkConfig, baseAddress, 0, this, 4),
   m_cd0Errs   (linkConfig, baseAddress, 0, this, 4),
   m_cd1Errs   (linkConfig, baseAddress, 1, this, 4),
   m_states    (linkConfig, baseAddress, 0, this, 4),
   m_frameErrs (linkConfig, baseAddress, 0, this, 4)
{
   // Description
   desc_ = "DataCompressionMonitorRead object";


   addDevice (&m_wibErrs  );
   addDevice (&m_cd0Errs  );
   addDevice (&m_cd1Errs  );
   addDevice (&m_states   );
   addDevice (&m_frameErrs);


   // Enable polling  
   m_wibErrs  .pollEnable (true);
   m_cd0Errs  .pollEnable (true);
   m_cd1Errs  .pollEnable (true);
   m_states   .pollEnable (true);
   m_frameErrs.pollEnable (true);


   pollEnable_ = true;   

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
DataCompression::Read::~Read ()
{
   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
DataCompression::Read::Registers::Registers (Device       *device,
                                             uint32_t baseAddress,
                                             uint32_t    addrSize)
{
   /*
    |  The register names are rather contrived so that the appear 
    |  somewhat in logical order that groups the registers in the
    |  various sections together.  (Unfortunately the ordering in
    |  gui is alphabetical.)
   */
   static const Svd_t Svd[] =
   {
      //  INDEX         NAME                  OFFSET         N32  POLLENABLE
      //  --------      ---------  ------------------------  ---  ----------
      [Rd_Status ] = { "Status",   MONITOR_READ_SUMMARY(0),   1,      true },
      [Rd_NFrames] = { "NFrames",  MONITOR_READ_SUMMARY(1),   1,      true },
   };


   static int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);
   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
DataCompression::Read::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompressionMontor::Read                                       */
/* ====================================================================== */




/* ====================================================================== */
/* BEGIN: DataCompression::Read::FrameErrs                                */
/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::FrameErrs device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::FrameErrs::FrameErrs (uint32_t linkConfig, 
                                             uint32_t baseAddress, 
                                             uint32_t       index,
                                             Device       *parent,
                                             uint32_t    addrSize) :
   Device  (linkConfig, baseAddress, "FrameErrs", index, parent),
   sr_ (this, baseAddress, addrSize)
{
   // Description
   desc_ = "DataCompressionMonitroReadFrameErrs object";

   // Enable polling
   pollEnable_ = true;   

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Destructor for the DataCompression::Read::FrameErrs device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::FrameErrs::~FrameErrs ()
 {
    return;
 }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::FrameErrs device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::FrameErrs::Registers::Registers (Device       *device,
                                                        uint32_t baseAddress,
                                                        uint32_t    addrSize)
{

   static const Svd_t Svd[] =
   { 
      //  INDEX         NAME                 OFFSET            N32  POLLENABLE
      //  ---------     -------  ----------------------------  ---  ----------
      [Rd_NErrSofM] = { "SofM",  MONITOR_READ_NFRAMEERRS (0),   1,      true },
      [Rd_NErrSofU] = { "SofU",  MONITOR_READ_NFRAMEERRS (1),   1,      true },
      [Rd_NErrEofM] = { "EofM",  MONITOR_READ_NFRAMEERRS (2),   1,      true },
      [Rd_NErrEofU] = { "EofU",  MONITOR_READ_NFRAMEERRS (3),   1,      true },
      [Rd_NErrEofE] = { "EofE",  MONITOR_READ_NFRAMEERRS (4),   1,      true },
   };


   static int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);
   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Deconstructor for the DataCompression::Read::FrameErrs
         device's registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::FrameErrs::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompression::Read::FrameErrs                           */
/* ====================================================================== */





/* ====================================================================== */
/* BEGIN: DataCompression::Read::WibErrs                                  */
/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::WibErrs device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::WibErrs::WibErrs (uint32_t linkConfig, 
                                         uint32_t baseAddress, 
                                         uint32_t       index,
                                         Device       *parent,
                                         uint32_t    addrSize) :
   Device  (linkConfig, baseAddress, "WibErrs", index, parent),
   sr_ (this, baseAddress, addrSize)
{
   // Description
   desc_ = "DataCompressionMonitroReadWibErrs object";

   // Enable polling
   pollEnable_ = true;   

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Destructor for the DataCompression::Read::WibErrs device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::WibErrs::~WibErrs ()
 {
    return;
 }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::WibErrs device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::WibErrs::Registers::Registers (Device       *device,
                                                      uint32_t baseAddress,
                                                      uint32_t    addrSize)
{ 
   /*
    |  The register names are rather contrived so that the appear 
    |  somewhat in logical order that groups the registers in the
    |  various sections together.  (Unfortunately the ordering in
    |  gui is alphabetical.)
   */
   static const Svd_t Svd[] =
   {
      //  INDEX                 NAME                    OFFSET            N32  POLLENABLE
      //  ----------------      ----------  ----------------------------  ---  ----------
      [Rd_NErrWibComma    ] = { "Comma",     MONITOR_READ_NWIBERRS ( 0),   1,     true },
      [Rd_NErrWibVersion  ] = { "Version",   MONITOR_READ_NWIBERRS ( 1),   1,     true },
      [Rd_NErrWibId       ] = { "Id",        MONITOR_READ_NWIBERRS ( 2),   1,     true },
      [Rd_NErrWibRsvd     ] = { "Rsvd",      MONITOR_READ_NWIBERRS ( 3),   1,     true },
      [Rd_NErrWibErrors   ] = { "Errors",    MONITOR_READ_NWIBERRS ( 4),   1,     true },
      [Rd_NErrWibTimestamp] = { "Timestamp", MONITOR_READ_NWIBERRS ( 5),   1,     true },
      [Rd_NErrWibUnused6  ] = { "Unused6",   MONITOR_READ_NWIBERRS ( 6),   1,     true },
      [Rd_NErrWibUnused7  ] = { "Unused7",   MONITOR_READ_NWIBERRS ( 7),   1,     true }
   };


   static int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);
   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Deconstructor for the DataCompression::Read::WibErrs device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */

DataCompression::Read::WibErrs::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompression::Read::WibErrs                                    */
/* ====================================================================== */





/* ====================================================================== */
/* BEGIN: DataCompression::Read::CdErrs                                   */
/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::CdErrs device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::CdErrs::CdErrs (uint32_t linkConfig, 
                                       uint32_t baseAddress, 
                                       uint32_t       index,
                                       Device       *parent,
                                       uint32_t    addrSize) :
   Device  (linkConfig, baseAddress, "CdErrs", index, parent),
   sr_ (this, baseAddress, addrSize)
{
   // Description
   desc_ = "DataCompressionMonitorReadCdErrs object";

   // Enable polling
   pollEnable_ = true;   

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Destructor for the DataCompression::Read::CdErrs device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::CdErrs::~CdErrs ()
 {
    return;
 }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::ReadCdErrs device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::CdErrs::Registers::Registers (Device       *device,
                                                     uint32_t baseAddress,
                                                     uint32_t    addrSize)
{
   static const Svd_t Svd[] =
   { 
      //  INDEX               NAME     OFFSET                       N32  POLLENABLE
      //  ---------           -------  ---------------------------  ---  ----------
      [Rd_NErrCdStrErr1] = {"StrErr1", MONITOR_READ_NWIBERRS ( 8),   1,      true },
      [Rd_NErrCdStrErr2] = {"StrErr2", MONITOR_READ_NWIBERRS ( 9),   1,      true },
      [Rd_NErrCdRsvd0  ] = {"Rsvd0",   MONITOR_READ_NWIBERRS (10),   1,      true },
      [Rd_NErrCdChkSum ] = {"ChkSum",  MONITOR_READ_NWIBERRS (11),   1,      true },
      [Rd_NErrCdCvtCnt ] = {"CvtCnt",  MONITOR_READ_NWIBERRS (12),   1,      true },
      [Rd_NErrCdErrReg ] = {"ErrReg",  MONITOR_READ_NWIBERRS (13),   1,      true },
      [Rd_NErrCdRsvd1  ] = {"Rsvd1",   MONITOR_READ_NWIBERRS (14),   1,      true },
      [Rd_NErrCdHdrs   ] = {"Hdrs",    MONITOR_READ_NWIBERRS (15),   1,      true }
   };

   static int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);


   // If this is the second set of colddata statistics, displace them by first
   if (device->index () == 1)
   {
      baseAddress +=  SvdCnt * addrSize;
   }


   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Deconstructor for the DataCompression::Read::WibErrs device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */

DataCompression::Read::CdErrs::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompression::Read::WibErrs                                    */
/* ====================================================================== */




/* ====================================================================== */
/* BEGIN: DataCompression::Read::States                                   */
/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::States device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::States::States (uint32_t linkConfig, 
                                       uint32_t baseAddress, 
                                       uint32_t       index,
                                       Device       *parent,
                                       uint32_t    addrSize) :
   Device  (linkConfig, baseAddress, "States", index, parent),
   sr_ (this, baseAddress, addrSize)
{
   // Description
   desc_ = "DataCompressionMonitroReadStates object";

   // Enable polling
   pollEnable_ = true;   

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Destructor for the DataCompression::Read::States device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::States::~States ()
 {
    return;
 }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Read::CdErrs device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::States::Registers::Registers (Device       *device,
                                                     uint32_t baseAddress,
                                                     uint32_t    addrSize)
{

   static const Svd_t Svd[] =
   { 
      //  INDEX           NAME          OFFSET                      N32  POLLENABLE
      //  ---------       -------       --------------------------  ---  ----------
      [Rd_NNormal  ] = { "Normal",      MONITOR_READ_NSTATES ( 0),   1,      true },
      [Rd_NDisabled] = { "RunDisabled", MONITOR_READ_NSTATES ( 1),   1,      true },
      [Rd_NFlush   ] = { "Flush",       MONITOR_READ_NSTATES ( 2),   1,      true },
      [Rd_NDisFlush] = { "DisFlush",    MONITOR_READ_NSTATES ( 3),   1,      true },
   };


   static int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);
   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Deconstructor for the DataCompression::Read::States device's
         registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Read::States::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompression::Read::States                                     */
/* ====================================================================== */




/* ====================================================================== */
/* BEGIN: DataCompressionWrite                                            */
/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Write device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Write::Write (uint32_t linkConfig, 
                               uint32_t baseAddress, 
                               uint32_t       index,
                               Device       *parent,
                                                uint32_t    addrSize) :
   Device  (linkConfig, baseAddress, "Write", index, parent),
   sr_ (this, baseAddress, addrSize)
{
   // Description
   desc_ = "DataCompressionMonitorWrite object";

   // Enable polling
   pollEnable_ = true;   

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Destructor for the DataCompression::Write device
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Write::~Write ()
{
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Constructor for the DataCompression::Write device's registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Write::Registers::Registers (Device       *device,
                                              uint32_t baseAddress,
                                              uint32_t    addrSize)
{
   static const Svd_t Svd[] =
   {
      //  INDEX           NAME        OFFSET             N32  POLLENABLE
      //  ---------       -------     ------------------ ---  ----------
      [Wr_NBytes   ] = { "NBytes",    MONITOR_WRITE (0),   1,      true },
      [Wr_NPromoted] = { "NPromoted", MONITOR_WRITE (1),   1,      true },
      [Wr_NDropped ] = { "NDropped",  MONITOR_WRITE (2),   1,      true },
      [Wr_NPackets ] = { "NPackets",  MONITOR_WRITE (3),   1,      true }
 
   };

   static int SvdCnt = sizeof (Svd) / sizeof (Svd[0]);
   populate (rl_, Svd, SvdCnt, device, baseAddress, addrSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
  \brief Deconstructor for the DataCompression::Write device's registers
                                                                          */
/* ---------------------------------------------------------------------- */
DataCompression::Write::Registers::~Registers ()
{
   return;
}
/* ---------------------------------------------------------------------- */
/* END: DataCompression::Write                                            */
/* ====================================================================== */



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


/* ---------------------------------------------------------------------- *//*!
   \brief Convenience method for initialization an array of status 
          register links.
   \param rlArray  Receives the array of register links
   \param     svd  The vector of status vector descriptors that define 
                   the register
   \param   device The parent device
   \param  baseAddress The base address of these registers
   \param  addrSize    The size, in bytes, of the register
                                                                          */
/* ---------------------------------------------------------------------- */
static void populate (RegisterLink *rlArray[],
                      Svd_t const        *svd, 
                      int                 cnt,
                      Device          *device,
                      uint32_t    baseAddress, 
                      uint32_t       addrSize)
{
   for (int idx = 0; idx < cnt; svd++, idx++)
   {

      printf ("%20s Address = %8.8x\n", svd->name, baseAddress + (svd->offset * addrSize));
      RegisterLink *rl = new RegisterLink (svd->name, 
                                           baseAddress + (svd->offset * addrSize), 
                                           svd->n32, 
                                           Variable::Status);

      rlArray[idx] = rl;

      device->addRegisterLink (rl);
      rl->setPollEnable (svd->pollEnable);
   }
}
/* ---------------------------------------------------------------------- */
#endif
