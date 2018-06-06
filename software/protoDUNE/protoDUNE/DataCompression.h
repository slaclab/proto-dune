//-----------------------------------------------------------------------------
// File          : DataCompression.h
// Author        : JJRussell <russell@slac.stanford.edu>
// Created       : 2018/03/16
// Project       : 
//-----------------------------------------------------------------------------
// Description :  Container for DataCompression Monitor Registers
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
// DATE       WHO WHAT
// ---------- --- -------------------------------------------------------------
// 2018.03.16 jjr Added many more HLS Data Compression variables to monitor
//                Eliminated 'using namespace std
// 2016.11.19 jjr Added enableHLSmodule method
//
// 11/05/2015: created
//-----------------------------------------------------------------------------

#ifndef __DATA_COMPRESSION_DEVICE_H__
#define __DATA_COMPRESSION_DEVICE_H__

#include <Device.h>
#include <stdint.h>


/* ---------------------------------------------------------------------- *//*!
  \brief Provides the methods to make the Data Compression variables 
         to be monitored available to the GUI status tab.
  \par
   The variables being monitored fall into the falling major categories
       =# Variables that monitor the reconfiguration
       -# Variables that monitor the reading of the frame
       -# Variables that monitor the writing of the frame
  \par
   The read variables are the most involved these are broken into
       -# Frame Errors    -- these occur when receiving the WIB frame 
                             from the WIB
       -# State           -- counts state transitions
       -# Wib Errors      -- these are detected in the WIB header 
       -# ColdData Errors -- these are detected in the cold data streams
                                                                          */
/* ---------------------------------------------------------------------- */
class DataCompression : public Device {        
 public:
    DataCompression ( uint32_t  linkConfig, 
                      uint32_t baseAddress, 
                      uint32_t       index,
                      Device       *parent,
                      uint32_t  addrSize=1);
    ~DataCompression( ); 


    void command      ( string name, string arg );  
    void hardReset    ();
    void softReset    ();
    void countReset   ();    
    void readStatus   ();
    void readConfig   ();      
    void writeConfig  (bool force);
    void verifyConfig ();

    /* ------------------------------------------------------------------- *//*!
       \brief Enumerates the common and configuation registers
    \* ------------------------------------------------------------------- */  
    class Registers
    {
    public:
       Registers (Device       *device,
                  uint32_t baseAddress,
                  uint32_t    addrSize);
     ~Registers ();

    public:
     
     /* ------------------------------------------------------------------ *//*!
        \brief Enumerates the common and configuration registers
     \* ------------------------------------------------------------------ */
     enum Enum
       {
          Ca_Pattern  =  0, /*!< Hex pattern used to tag a promoted packet */
          Cfg_Mode    =  1, /*!< The DAQ mode (copy, compress, etc         */
          Cfg_NCfgs   =  2, /*!< The number of reconfigurations            */
          Cfg_Count   =  3, /*!< Count of common and configuration regs    */
       };

    private:
       RegisterLink *rl_[Cfg_Count];
    };
    /* ------------------------------------------------------------------- */  




    /* ------------------------------------------------------------------- *//*!
      \brief  Collection of all the variables that are monitored during 
              the HLS reading of the frame.
    \* ------------------------------------------------------------------- */
    class Read : public Device
    {
    public:
       Read (uint32_t  linkConfig, 
             uint32_t baseAddress, 
             uint32_t       index,
             Device       *parent,
             uint32_t    addrSize);
       
       ~Read  ();

       /* ---------------------------------------------------------------- *//*!
          \brief The global read registers
       \* ---------------------------------------------------------------- */
       class Registers
       {
       public:
          Registers (Device       *device,
                     uint32_t baseAddress,
                     uint32_t    addrSize);
          
          ~Registers ();

       public:
          /* ------------------------------------------------------------- *//*!
            \brief Enumerates the global read registers
          \* ------------------------------------------------------------- */
          enum Enum
          {
             Rd_Status           =  0, /*!< Nonlatching 32-bit status mask */
             Rd_NFrames          =  1, /*!< The number of input frames seen*/
             Rd_Count            =  2  /*!< Count of the global registers  */
          };
          
       private:
          RegisterLink *rl_[Rd_Count];
       };
       /* ---------------------------------------------------------------- */



       /* --------------------------------------------------------------- *//*!
          \class WibErrs
          \brief Monitors the Wib header frame  errors
       \* --------------------------------------------------------------- */
       class WibErrs : public Device
       {
       public:
          WibErrs (uint32_t linkConfig, 
                   uint32_t baseAddress, 
                   uint32_t       index,
                   Device       *parent,
                   uint32_t    addrSize);

          ~WibErrs ();
          
          /* ------------------------------------------------------------ *//*!
            \brief  Enumerates the registers associated with the WIB
                    header
          \* ------------------------------------------------------------ */
          class Registers
          {
          public:
             Registers (Device       *device,
                        uint32_t baseAddress,
                        uint32_t    addrSize);
             
             ~Registers ();

          public:
             /* -------------------------------------------------------- *//*!
               \brief Enumerats the WIB header registers
             \* -------------------------------------------------------- */
             enum Enum
             {
                Rd_NErrWibComma     =  0, /*!< Comma character not seen  */
                Rd_NErrWibVersion   =  1, /*!< Wib Frame version wrong   */
                Rd_NErrWibId        =  2, /*!< Wib Id inconsistent       */
                Rd_NErrWibRsvd      =  3, /*!< Wib reserved field != 0   */
                Rd_NErrWibErrors    =  4, /*!< Wib error field != 0      */
                Rd_NErrWibTimestamp =  5, /*!< Wib timestamp inconsistent*/
                Rd_NErrWibUnused6   =  6, /*!< Reserved                  */
                Rd_NErrWibUnused7   =  7, /*!< Reserved                  */
                Rd_Count            =  8  /*!< Count of WIB registers    */
             };

          private:
             RegisterLink *rl_[Rd_Count];
          };

       private:
          Registers sr_;
       };
       /* -------------------------------------------------------------- */
       /* END: WibErrs                                                   */
       /* -------------------------------------------------------------- */




       /* -------------------------------------------------------------- *//*!
          \class CdErrs
          \brief Monitors the cold data link errors
       \* -------------------------------------------------------------- */
       class CdErrs : public Device
       {
       public:
          CdErrs (uint32_t linkConfig, 
                  uint32_t baseAddress, 
                  uint32_t       index,
                  Device       *parent,
                  uint32_t    addrSize);
          
          ~CdErrs ();
          
          class Registers
          {
          public:
             Registers (Device       *device,
                        uint32_t baseAddress,
                        uint32_t    addrSize);
        
             ~Registers ();

          public:
             /* ----------------------------------------------------------- *//*!
                \brief Enumerates registers associated with the cold data
                       links
                \warning
                The \e Rd_NerrChkSum is a placeholder reserved until 
                it is decided what to do with. Because the information
                that went into computing the checksum is not promoted
                to the HLS module, this checksum cannot be verified.
                                                                           */
             /* ---------------------------------------------------------- */
             enum Enum
             {
                Rd_NErrCdStrErr1   =  0, /*!< Stream Err1 field != 0       */
                Rd_NErrCdStrErr2   =  1, /*!< Stream Err2 field != 0       */
                Rd_NErrCdRsvd0     =  2, /*!< First reserved field != 0    */
                Rd_NErrCdChkSum    =  3, /*!< Checksum error, see above    */
                Rd_NErrCdCvtCnt    =  4, /*!< Convert count inconsistent   */
                Rd_NErrCdErrReg    =  5, /*!< Error register != 0          */
                Rd_NErrCdRsvd1     =  6, /*!< Second reserved field != 0   */
                Rd_NErrCdHdrs      =  7, /*!< Error in one or more header  */
                Rd_Count           =  8, /*!< Count of cold data registers */
             };
             
          private:
             RegisterLink *rl_[Rd_Count];
          };

       private:
          Registers sr_;
       };
       /* -------------------------------------------------------------- */
       /* END: CdErrs                                                    */
       /* -------------------------------------------------------------- */




       /* -------------------------------------------------------------- *//*!
          \class States
          \brief Monitors the run state
       \* -------------------------------------------------------------- */
       class States : public Device
       {
       public:
          States (uint32_t  linkConfig, 
                  uint32_t baseAddress, 
                  uint32_t       index,
                  Device       *parent,
                  uint32_t    addrSize);
          
          ~States ();


          /* ----------------------------------------------------------- *//*!
             \class States
             \brief Monitors run state on each received frame
          \* ----------------------------------------------------------- */
          class Registers
          {
          public:
             Registers (Device       *device,
                        uint32_t baseAddress,
                        uint32_t    addrSize);
             
             ~Registers ();
             
          public:

             /* ----------------------------------------------------------- *//*!
                \brief Enumerates registers counting the run state on
                       each received frame.
             \* ----------------------------------------------------------- */
             enum Enum
             {
                Rd_NNormal          =  0, /*!< Normal running               */
                Rd_NDisabled        =  1, /*!< Run is disabled              */
                Rd_NFlush           =  2, /*!< Normal running frame flushed */
                Rd_NDisFlush        =  3, /*!< Run disabled, frame flushed  */ 
                Rd_Count            =  4  /*!< Count of state registers     */
             };
             
          private:
             RegisterLink *rl_[Rd_Count];
          };

       private:
          Registers sr_;
       };
       /* -------------------------------------------------------------- */
       /* END: State                                                     */
       /* -------------------------------------------------------------- */



       /* -------------------------------------------------------------- *//*!
          \class FrameErrs
          \brief Monitors the transport protocol errors that can occur 
                 when receiving the frame from the WIB
       \* ---------------------------------------------------------------- */
       class FrameErrs : public Device
       {
       public:
          FrameErrs (uint32_t  linkConfig, 
                     uint32_t baseAddress, 
                     uint32_t       index,
                     Device       *parent,
                     uint32_t    addrSize);
          ~FrameErrs ();

          /* ------------------------------------------------------------- *//*!
             \brief Enumerates registers counting errors that can occur
                    on the hand off of the frame from the deserialization
                    to the HLS module.
          \* ------------------------------------------------------------- */
          class Registers
          {
          public:
             Registers (Device       *device,
                        uint32_t baseAddress,
                        uint32_t    addrSize);
             
             ~Registers ();
             
          public:

             /* ----------------------------------------------------------- *//*!
                \brief Enumerates registers counting the errors 
             \* ----------------------------------------------------------- */
             enum Enum
             {
                Rd_NErrSofM         =  0, /*!< Missing Sof on first word    */
                Rd_NErrSofU         =  1, /*!< Unexpected Sof               */
                Rd_NErrEofM         =  2, /*!< Missing Eof on  last word    */
                Rd_NErrEofU         =  3, /*!< Unexpected Eof               */
                Rd_NErrEofE         =  4, /*!< Error reported on Eof word   */
                Rd_Count            =  5  /*!< Count of error registers     */
             };

          private:
             RegisterLink *rl_[Rd_Count];
          };

       private:
          Registers sr_;
       };
       /* ---------------------------------------------------------------- */
       /* END: FrameErr                                                    */
       /* ---------------------------------------------------------------- */
    
    private:
       Registers         sr_;  /*!< The global read error registers        */
       
       WibErrs     m_wibErrs;  /*!< The Wib Error monitoring device        */
       CdErrs      m_cd0Errs;  /*!< Cold Data Link 0 monitoring device     */
       CdErrs      m_cd1Errs;  /*!< Cold Data Link 1 monitoring device     */
       States       m_states;  /*!< Run state per frame counters           */
       FrameErrs m_frameErrs;  /*!< Reception frame error device           */
    };
    /* ------------------------------------------------------------------- */
    /* END: Read                                                           */
    /* ------------------------------------------------------------------- */



    /* ------------------------------------------------------------------- *//*!
      \brief Monitors the output writes
    \* ------------------------------------------------------------------- */
    class Write : public Device
    {
    public:
       Write (uint32_t linkConfig, 
              uint32_t baseAddress, 
              uint32_t       index,
              Device       *parent,
              uint32_t    addrSize);
       
       ~Write  ();

       /* -------------------------------------------------------------- *//*!
          \brief The registers monitoring the output writing
       \* -------------------------------------------------------------- */
       class Registers
       {
       public:
          Registers (Device       *device,
                     uint32_t baseAddress,
                     uint32_t    addrSize);

          ~Registers ();

       public:

          /* -------------------------------------------------------------- *//*!
             \brief Enumerates registers monitoring the output writing
          \* -------------------------------------------------------------- */
          enum Enum
          {
             Wr_NBytes           =  0, /*!< Total number of bytes written   */
             Wr_NPromoted        =  1, /*!< Number of frames -> packets     */
             Wr_NDropped         =  2, /*!< Number of frames dropped        */
             Wr_NPackets         =  3, /*!< Number of 1024 frame packets    */
             Wr_Count            =  4  /*!< Count of write registers        */
          };
          
       private:
          RegisterLink *rl_[Wr_Count];
       };

    private:
       Registers sr_;
    };
    /* ------------------------------------------------------------------ */
    /* END: Write                                                         */
    /* ------------------------------------------------------------------ */

private:
    void enableHLSmodule (uint32_t addrSize);

private:
    Registers      sr_; /*!< The DataCompression global monitor registers */
    Read        m_read; /*!< The DataCompression read   monitor registers */
    Write      m_write; /*!< The DataCompression write  monitor registers */
};
/* ---------------------------------------------------------------------- */

#endif
