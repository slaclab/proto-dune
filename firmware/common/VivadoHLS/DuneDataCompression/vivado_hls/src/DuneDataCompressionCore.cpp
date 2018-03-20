// -*-Mode: C++;-*-


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionCore.cpp
 *  @brief    Core file for the DUNE compresssion
 *  @verbatim
 *                               Copyright 2013
 *                                    by
 *
 *                       The Board of Trustees of the
 *                    Leland Stanford Junior University.
 *                           All rights reserved.
 *
 *  @endverbatim
 *
 *  @par Facility:
 *  DUNE
 *
 *  @author
 *  russell@slac.stanford.edu
 *
 *  @par Date created:
 *  2016.06.14
 *
 *  @par Last commit:
 *  \$Date: $ by \$Author: $.
 *
 *  @par Revision number:
 *  \$Revision: $
 *
 *  @par Location in repository:
 *  \$HeadURL: $
 *
 * @par Credits:
 * SLAC
 *
\* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\

   HISTORY
   -------

   DATE       WHO WHAT
   ---------- --- ---------------------------------------------------------
   2016.11.16 jjr To get around the Vivado HLS crash when update_write was
                  implemented as a DATAFLOW method, it was merged with
                  write_frame.  This hurts performance a bit, but still
                  completes in the necessary time.  It is just esthically
                  not very pleasing.

   2016.11.01 jjr The dataflow copy now works.  Unknown why this is so. The
                  work of 2016.10.25 was just rearranging.  The one thing
                  that is different is that only the code relevant to the
                  simple copy is included. Previously, there was code that
                  that was part of compression, but it was unused.

                  Correct error that prevented the nwrote counter from
                  incrementing.

   2016.10.25 jjr Major rearrangement of the code. This code can now be
                  compiled in three different modes

                     -# A simple copy
                     -# A dataflow copy
                     -# Compression

                  The simply copy is a kludge in an attempt to give the
                  BU guys something to commission the WIB with.  It does
                  not handle the transition from Enabled -> Disabled nor
                  the flush correctly. The error is not too severe,
                  tacking on one invalid frame to the packet. More seriously
                  it means that the next packet is rejected.  Thus if one
                  had the sequence FLUSH, RUNENABLE, that first RUNENABLE
                  packet would be dropped.

                  The dataflow copy handles things correctly except, at
                  the time of this writing, we could not make it run at
                  the desired rate.

                  Compression is still a gleam in the eye.  The code
                  compiled at one time, but I seriously doubt that it
                  compiles now.

   2016.10.22 jjr Change AXI input streams to hls::stream
                  Change data type to unsigned 64-bit values
                  Eliminate return value
   2016.10.18 jjr Change to buffering input data to accomodate the flush
                  and RunEnable flags.
   2016.06.14 jjr Created

\* ---------------------------------------------------------------------- */


//////////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Data compression'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Data compression', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////


#include "DuneDataCompressionCore.h"
#include "DuneDataCompressionPacket.h"
#include "DuneDataCompressionHistogram.h"
#include "WibFrameV0_3.h"

/* ====================================================================== */
/*                                                                        */
/*    These enumerate the possible processing modes                       */
/*    Setting the #define PROCESS_K_TYPE will select that mode            */
/*                                                                        */
/* ---------------------------------------------------------------------- */
#define PROCESS_K_COPY        1 /* Simple copy, essentially *out++ = *in++*/
#define PROCESS_K_DATAFLOW    2 /* Buffered copy using DATAFLOW           */
#define PROCESS_K_COMPRESS    3 /* Compressed all                         */
/* ====================================================================== */



/* ====================================================================== */
/* !!!                                                                !!! */
/* !!! --- HERE BE THE #DEFINE USED TO SELECT THE PROCESSING MODE --- !!! */
/* !!!                                                                !!! */
/* ====================================================================== */
/*                                                                        */
/*  CRITICAL:                                                             */
/*  PROCESS_K_TYPE must be set to one of the above PROCESS_K_<xxxx> to    */
/*  select the desired processing mode.                                   */
/* ---------------------------------------------------------------------- */
#define PROCESS_K_MODE PROCESS_K_COMPRESS
/* ====================================================================== */




/* ====================================================================== */
/* Convenience routines common to all processing modes                    */
/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
static void print_header (uint64_t hdr)
{
   std::cout << "Header: " << std::setw(16) << std::hex << hdr << std::endl;
   return;
}

static void print_trailer (uint64_t tlr)
{
   std::cout << "Trailer: " << std::setw(16) << std::hex << tlr << std::endl;
   return;
}
#else

// --------------------------------------------------------------
// These must be #define away.
// While Vivado understands how to ignore std::cout, it does not
// know how to ignore things like std::setw and std::hex
// --------------------------------------------------------------
#define print_header(_hdr)
#define print_trailer(_hdr)

#endif


/* ----------------------------------------------------------------------- */
/* CLASSES: Local                                                          */
/* ----------------------------------------------------------------------- */


static void         commit (AxisOut  &axis, int &odx, bool write,
                            uint64_t  data, int user, int   last);


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Typedef for the read frame status
 *
 *  \par
 *   This is a bit mask of conditions that would invalidate the frame
 *   data.  Currently there are
 *      - Bit 0: RunDisable
 *      - Bit 1: Flush
 *      - Bit 2: Start of frame marker missing
 *      - Bit 3: End   of frame marker missing
 *      - Bit 4: K28_5 character not seen
 *      - Bit 5: WIB sequence error
 *
 *   If either of these 4 bits  are set
 *      -# The data for this frame is consider invalid
 *       # Any in-progress packet should be flushed.
\* ----------------------------------------------------------------------- */
typedef ap_uint<32> ReadStatus_t;

/* ----------------------------------------------------------------------- *//*!
 *
 *  \struct ReadFrame
 *  \brief  Structure to bind the read status with the frame's data
 *
\* ----------------------------------------------------------------------- */
struct ReadFrame
{
   ReadFrame () :
      status (0),
      write  (false),
      last   (false)
   {
      return;
   }

   enum class WibOffset
   {
      ErrWibBeg        = 0x00,

      // Errors from the WIB header words
      ErrWibBeg0      = 0x00,
      ErrWibComma     = 0x00, /*!< Comma character does not match         */
      ErrWibVersion   = 0x01, /*!< Version number incorrect               */
      ErrWibId        = 0x02, /*!< WibId (Crate.Slot.Fiber) incorrect     */
      ErrWibRsvd      = 0x03, /*!< Reserved header bits are not 0         */
      ErrWibErrors    = 0x04, /*!< Wib Errors field is not 0              */
      ErrWibEnd0      = 0x04,
      ErrWibCnt0      = ErrWibEnd0 - ErrWibBeg0 + 1,

      ErrWibBeg1      = 0x05,
      ErrWibTimestamp = 0x05, /*!< Unused bit                             */
      ErrWibUnused6   = 0x06, /*!< Unused bit                             */
      ErrWibUnused7   = 0x07, /*!< Unused bit                             */
      ErrWibEnd1      = 0x07,
      ErrWibCnt1      = ErrWibEnd1 - ErrWibBeg1 + 1,

      ErrWibEnd       = 0x07,
      ErrWibCnt       = ErrWibEnd  - ErrWibBeg  + 1
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  CdOffset
    *  \brief Maps out the bits of the cold data link status bits
    *
    */
   enum class CdOffset
   {
      // Errors from the Colddata stream 0 header words
      ErrCdBeg0      = 0x00,
      ErrCdStrErr1   = 0x00,  /*!< Colddata link, stream err1 is not 0    */
      ErrCdStrErr2   = 0x01,  /*!< Colddata link, stream err2 is not 0    */
      ErrCdRsvd0     = 0x02,  /*!< Colddata link, reserved field is not 0 */
      ErrCdChkSum    = 0x03,  /*!< Colddata link, checksum incorrect      */
      ErrCdCvtCnt    = 0x04,  /*!< Colddata link, convert count mismatch  */
      ErrCdEnd0      = 0x04,
      ErrCdCnt0      = ErrCdEnd0 - ErrCdBeg0 + 1,

      ErrCdBeg1      = 0x05,
      ErrCdErrReg    = 0x05,  /*!< Colddata link, error register is not 0 */
      ErrCdRsvd1     = 0x06,  /*!< Colddata link, reserved field is not 0 */
      ErrCdHdrs      = 0x07,  /*!< Colddata link, error in hdr words      */
      ErrCdEnd1      = 0x07,
      ErrCdCnt1      = ErrCdEnd1 - ErrCdBeg1 + 1,
      ErrCdEnd       = ErrCdEnd1,

      ErrCdCnt       = ErrCdEnd1 - ErrCdBeg1 + 1

   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  StateOffset
    *  \brief Maps out the bits of the frame state status bits
    *
    */
   enum class StateOffset
   {
      StateBeg        = 0x00,
      RunDisable      = 0x01, /*!< Run is disable                         */
      Flush           = 0x02, /*!< Flush the packet data                  */
      StateEnd        = 0x02,
      StateCnt        = StateEnd - StateBeg + 1
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  FrameSOffet
    *  \brief Maps out the bits of the frame state status bits
    *
    */
   enum class FrameOffset
   {
      ErrFrameBeg     = 0x00,
      ErrSofM         = 0x01, /*!< Start of frame error, error            */
      ErrSofU         = 0x02, /*!< Start of frame error, unexpected       */
      ErrEofM         = 0x03, /*!< End   of frame error, missing          */
      ErrEofU         = 0x04, /*!< End   of frame error, unexpected       */
      ErrEofE         = 0x05, /*!< End   of frame error, error bit set    */
      ErrFrameEnd     = 0x05,
      ErrFrameCnt     = ErrFrameEnd - ErrFrameBeg + 1
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum Offset
    *  \brief Maps out the bits in the status mask
    */
   enum Offset
   {
      // Errors from the WIB header words
      ErrWibBeg0      = 0x00,
      ErrWibComma     = 0x00, /*!< Comma character does not match         */
      ErrWibVersion   = 0x01, /*!< Version number incorrect               */
      ErrWibId        = 0x02, /*!< WibId (Crate.Slot.Fiber) incorrect     */
      ErrWibRsvd      = 0x03, /*!< Reserved header bits are not 0         */
      ErrWibErrors    = 0x04, /*!< Wib Errors field is not 0              */
      ErrWibEnd0      = 0x04,

      ErrWibBeg1      = 0x05,
      ErrWibTimestamp = 0x05, /*!< Unused bit                             */
      ErrWibUnused6   = 0x06, /*!< Unused bit                             */
      ErrWibUnused7   = 0x07, /*!< Unused bit                             */
      ErrWibEnd1      = 0x07,

      // Errors from the Colddata stream 0 header words
      ErrCd0Beg0      = 0x08,
      ErrCd0StrErr1   = 0x08,  /*!< Colddata link 0, stream err1 is not 0   */
      ErrCd0StrErr2   = 0x09,  /*!< Colddata link 0, stream err2 is not 0   */
      ErrCd0Rsvd0     = 0x0a,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd0ChkSum    = 0x0b,  /*!< Colddata link 0, checksum incorrect     */
      ErrCd0CvtCnt    = 0x0c,  /*!< Colddata link 0, convert count mismatch */
      ErrCd0End0      = 0x0c,

      ErrCd0Beg1      = 0x0d,
      ErrCd0ErrReg    = 0x0d,  /*!< Colddata link 0, error register is not 0*/
      ErrCd0Rsvd1     = 0x0e,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd0Hdrs      = 0x0f,  /*!< Colddata link 0, error in hdr words     */
      ErrCd0End1      = 0x0f,

      // Errors from the Colddata stream 1 header words
      ErrCd1Beg0      = 0x10,
      ErrCd1StrErr1   = 0x10,  /*!< Colddata link 0, stream err1 is not 0   */
      ErrCd1StrErr2   = 0x11,  /*!< Colddata link 0, stream err2 is not 0   */
      ErrCd1Rsvd0     = 0x12,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd1ChkSum    = 0x13,  /*!< Colddata link 0, checksum incorrect     */
      ErrCd1CvtCnt    = 0x14,  /*!< Colddata link 0, convert count mismatch */
      ErrCd1End0      = 0x14,

      ErrCd1Beg1      = 0x15,
      ErrCd1ErrReg    = 0x15,  /*!< Colddata link 0, error register is not 0*/
      ErrCd1Rsvd1     = 0x16,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd1Hdrs      = 0x17,  /*!< Colddata link 0, error in hdr words     */
      ErrCd1End1      = 0x17,

      // Frame state
      StateBeg        = 0x18,
      RunDisable      = 0x18, /*!< Run is disable                           */
      Flush           = 0x19, /*!< Flush the packet data                    */
      StateEnd        = 0x19,
      StateCnt        = StateEnd - StateBeg + 1,

      // Frame errors
      ErrFrameBeg     = 0x1a,
      ErrSofM         = 0x1a, /*!< Start of frame error, error              */
      ErrSofU         = 0x1b, /*!< Start of frame error, unexpected         */
      ErrEofM         = 0x1c, /*!< End   of frame error, missing            */
      ErrEofU         = 0x1d, /*!< End   of frame error, unexpected         */
      ErrEofE         = 0x1e, /*!< End   of frame error, error bit set      */
      ErrFrameEnd     = 0x1e
   };
   /* ---------------------------------------------------------------------- */






   /* ---------------------------------------------------------------------- *//*!
    *
    *  \brief  Extract the state bits from the status word
    *  \return The extracted state bits
    *
    *  \param[in]  status The target status word.
   \*----------------------------------------------------------------------- */
   static ap_uint<StateCnt> state (ReadStatus_t status)
   {
      #pragma HLS INLINE
      return status (Offset::StateEnd, Offset::StateEnd);
   }
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
     *
     *  \brief Tests whether the frame was correctly read
     *  \retval true, if it was correctly read
     *  \retval false, if not
     *
     *  \param[in] status The read status
     *                                                                        */
    /* ---------------------------------------------------------------------- */
    static bool isGoodFrame (ap_uint<32> status)
    {
       #pragma HLS INLINE
       int sb = status >> static_cast<unsigned>(Offset::ErrFrameBeg);
       return sb == 0;
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- *//*!
     *
     *  \brief Tests whether this frame is flushes the packet
     *  \retval true,  if so
     *  \retval false, if not
     *
     *  \param[in] status The read status
     *
     *  If either the run is disabled nor the frame flush is set, the
     *  frame is considered the last frame in the packet.
     *                                                                        */
    /* ---------------------------------------------------------------------- */
    static bool flushesPacket (ReadStatus_t status)
    {
       #pragma HLS INLINE
       bool wf = status.test (static_cast<unsigned>(Offset::RunDisable))
              |  status.test (static_cast<unsigned>(Offset::Flush));
       return wf;
    }

    static bool isWibHdr0Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrWibEnd0),
                      static_cast<unsigned>(Offset::ErrWibBeg0));
    }

    static bool isWibHdr1Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrWibEnd1),
                      static_cast<unsigned>(Offset::ErrWibBeg1));
    }

    static bool isCd0Hdr0Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd0End0),
                      static_cast<unsigned>(Offset::ErrCd0Beg0));
    }

    static bool isCd0Hdr1Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd0End1),
                      static_cast<unsigned>(Offset::ErrCd0Beg1));
    }

    static bool isCd1Hdr0Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd1End0),
                      static_cast<unsigned>(Offset::ErrCd1Beg0));
    }

    static bool isCd1Hdr1Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd1End1),
                      static_cast<unsigned>(Offset::ErrCd1Beg1));
    }


    typedef uint64_t Data;
    //typedef hls::stream<uint64_t> Data;

   ReadStatus_t             status;  /*!< The frame's read status          */
   bool                      write;  /*!< If true, output this frame       */
   bool                       last;  /*!< This is the last frame in packet */
   Data     d[MODULE_K_MAXSIZE_IB];  /*!< The frame's data. This data is
                                          valid iff \a status == 0         */
};
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 * \struct ReadContext
 * \brief  Structure to contain the read context.
 *
 *  The context are variables that maintained across WIB frames
 */
class ReadContext
{
public:
   ReadContext () :
      first (true),
      begPkt(true),
      mode     (0),
      wibId    (0),
      nframes  (0),
      cvtCnt0  (0),
      cvtCnt1  (0)
  {
      return;
  }

public:
   bool           first; /*!< First time flag                                */
   bool          begPkt; /*!< Beginning of a packet                          */
   ap_uint<2>      mode; /*!< The processing mode                            */
   ap_uint<11>    wibId; /*!< The crate, slot, fiber number                  */
   ap_uint<10>  nframes; /*!< The number of frames promoted                  */
   AxisWord_t   hdrs[6]; /*!< The 6 header words from the previous frame     */
   uint16_t     cvtCnt0; /*!< The previous convert count from stream 0       */
   uint16_t     cvtCnt1; /*!< The previous convert count from stream 1       */
};
/* ------------------------------------------------------------------------- */



/* ------------------------------------------------------------------------ *//*!
 *
 * \struct HdrContext
 * \brief  Buffers the compressed headers till ready to emit
 *
\* ------------------------------------------------------------------------- */
struct HdrContext
{
public:
   HdrContext () :
      hdx (0),
      edx (0)
{
      return;
}

   void reset ()
   {
      hdx = 0;
      edx = 0;
   }


   void addHeader (uint64_t hdr)
   {
      hdrsBuf[hdx++] = hdr;
   }

   void addException (uint16_t exc)
   {
      ap_int<2> shift = edx & 0x3;

      if (shift == 0) excBuf[edx] = 0;
      excBuf[edx] |= static_cast<uint64_t>(exc << (shift<<4));
      if (~shift) edx += 1;
   }

   void commit (AxisOut  &axis, int &odx, bool write, uint32_t status)
   {
      for (int idx = 0; idx < hdx; idx++)
      {
         ::commit (axis, odx, write, hdrsBuf[idx], 0, 0);
      }


      for (int idx = 0; idx < edx; idx++)
      {
         ::commit (axis, odx, write, excBuf[idx], 0, 0);
      }

      // Compose the summary word
      uint64_t summary = (static_cast<uint64_t>(status) << 32) | (hdx << 16) | (edx << 0);
      ::commit (axis, odx, write, summary, 0, 0);
   }

public:
   int         hdx;   /*!< Header buffer index                            */
   int         edx;   /*!< Exception buffer index                         */
   uint64_t hdrsBuf[1024 * 6];
   uint64_t excBuf[1024/4];
};
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \struct WriteContext
 *  \brief  Structure to bind the write context with the output stream
 *
\* ----------------------------------------------------------------------- */
struct WriteContext
{
public:
   WriteContext () :
      odx     (0),
      fdx     (0)
   {
      return;
   }

   void reset ()
   {
      odx = 0;
      fdx = 0;
      hdr.reset ();
      return;
   }

public:
    int         odx;   /*!< The output word index (count of output words)  */
    int         fdx;   /*!< Frame index (count of frames)                  */
    HdrContext  hdr;   /*!< The compressed header context                  */
};
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- */
/* PROTOTYPES: Local                                                       */
/* ----------------------------------------------------------------------- */
static void          copyCfg (ModuleConfig   &lcl,
                              ModuleConfig   &gbl) __attribute__ ((unused));

static void printReadResults (ap_uint<2>           status,
                              MonitorRead const  &monitor);

static void    update_common (MonitorCommon       &gbl,
                              MonitorCommon       &lcl,
                              ModuleConfig const  &cfg);

static void        configure (ModuleConfig const  &cfg,
                              MonitorCfg          &gbl,
                              MonitorCfg          &lcl,
                              bool               allow);

static void       read_frame (ReadFrame        &frame,
                              ReadContext        &ctx,
                              AxisIn           &sAxis,
                              ModuleConfig       &cfg,
                              MonitorRead        &gbl,
                              MonitorRead        &lcl);

static void       read_frame (ReadFrame        &frame,
                              ReadContext        &ctx,
                              AxisIn           &sAxis);

static void        read_wib0 (ReadStatus_t    &status,
                              ReadFrame::Data      *d,
                              AxisIn            &axis,
                              AxisWord_t        *hdrs,
                              bool             &first,
                              ap_uint<11>      &wibId);

static void       read_wib1 (ReadStatus_t    &status,
                             ReadFrame::Data      *d,
                             AxisIn            &axis,
                             AxisWord_t        *hdrs);

static void read_colddata_hdrs (ReadStatus_t &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                AxisWord_t     *hdrs,
                                uint16_t     &cvtCnt,
                                int              off);

static void read_colddata_data (ReadStatus_t &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                bool       checkLast);

static void      update_read (MonitorRead        &gbl,
                              MonitorRead        &lcl,
                              ModuleConfig const &cfg,
                              ReadStatus_t     status);

static void     write_frame (AxisOut           &mAxis,
                             WriteContext        &ctx,
                             ReadFrame         &frame,
                             MonitorWrite        &gbl,
                             MonitorWrite        &lcl);

static void         prologue (AxisOut          &mAxis,
                              int                &odx,
                              bool          writeFlag,
                              int           moduleIdx);

static void         epilogue (AxisOut          &mAxis,
                              uint64_t         status,
                              int               &odx);
/* ----------------------------------------------------------------------- */
/* END: Common/Convenience routines                                        */
/* ======================================================================= */




/* ======================================================================= */
#if PROCESS_K_MODE == PROCESS_K_COPY
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- */
/* PROTOTYPES: Local                                                       */
/* ----------------------------------------------------------------------- */
static void       copyFrame (AxisOut                        &mAxis,
                             int                              &odx,
                             AxisIn                         &sAxis,
                             ModuleIdx_t                 moduleIdx,
                             ModuleConfig                     &cfg,
                             MonitorModule                    &gbl,
                             MonitorModule                    &lcl);
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Top level entry to the data handling and compression
 *
 *  \param[ in]     sAxis  The  input data stream
 *  \param[out]      mAxis The output data stream
 *  \param[ in]  moduleIdx This refers to which group of channels are
 *                         being processed. Channels typically arrive
 *                         and are processed in groups of 128. An RCE
 *                         currently service 256 channels or 2 groups
 * \param[ in]     config  Configuration information including a flag
 *                         indicating that the system needs to be
 *                         configured or reconfigured.
 * \param[out]    monitor  Information that is used to monitor the
 *                         performance and internal workings of this
 *                         module.
\* ----------------------------------------------------------------------- */
void DuneDataCompressionCore(AxisIn            &sAxis,
                             AxisOut           &mAxis,
                             ModuleIdx_t    moduleIdx,
                             ModuleConfig      config,
                             MonitorModule   &monitor)
{

    #pragma HLS INTERFACE ap_none port=moduleIdx

    // This disables the FIFO interfaces for these variables
    //  -- Well, we tried, but this did not seem to do it
    //     The global parameters in the Solution Settting seem
    //     to take precedence.
    #pragma HLS STREAM variable=config off
    #pragma HLS STREAM variable=status off


   // Set the input and output ports as AXI4-StreaNm
   #pragma HLS INTERFACE axis port=sAxis  // Input Port
   #pragma HLS INTERFACE axis port=mAxis  // Output Port


   // Setup a AXI-Lite bus for status/configuration registers
   #pragma HLS INTERFACE s_axilite port=config          bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=status register bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=return          bundle=BUS_A


   #pragma HLS DATA_PACK variable=status.cfg
   //#pragma HLS DATA_PACK variable=status.read
   #pragma HLS DATA_PACK variable=config


   static   MonitorModule  lclMonitor;
   #pragma HLS RESOURCE variable=lclMonitor.read core=RAM_2P_LUTRAM
   //#pragma HLS STREAM   variable=lclMontior off


   static int  AxisOdx = 0;
   copyFrame (mAxis, AxisOdx, sAxis, moduleIdx, config, monitor, lclMonitor);

   return;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief     Copies the input stream to the output stream iff the
 *             \a processFlag is true
 *  \return    A flag indicating if the data was posted
 *
 *  \param[out]       mAxis  The output stream
 *  \param[ in]         odx  The output stream index
 *  \param[ in]       sAxis  The inpubt stream
 *  \param[ in]   moduleIdx  This module's index
 *  \param[ in]         cfg  The configuration
 *  \param[ in]         gbl  The module's global write status.  This
 *                           is receives a copy of the local write
 *                           status after the local write status has
 *                           been updated.
 *  \param[in:out[      lcl  The module's local write status
 *
 *
 *  \par
 *   The data is posted when
 *     -# The maximum number of frames in the output packet has been reached
 *     -# The \a processFlag is false and the output packet is not empty
 *
\* ----------------------------------------------------------------------- */
static void copyFrame (AxisOut          &mAxis,
                       int                &odx,
                       AxisIn           &sAxis,
                       ModuleIdx_t   moduleIdx,
                       ModuleConfig       &cfg,
                       MonitorModule      &gbl,
                       MonitorModule      &lcl)
{
   #pragma HLS INLINE

   static ap_uint<2> Inhibit = 0;

   // Keep it real simple for now
   bool  write = true;
   AxisIn_t in = sAxis.read ();
   ///bool write = !cfg.init && (cfg.mode == MODE_K_COPY);


   // Check run enabled flag
   bool runDisabled = (in.user & static_cast<uint8_t>(AxisUserFirst::RunEnable)) == 0;
   Inhibit.set (0, runDisabled);

   // If not inhibited, write the header (if odx == 0) and always write the data
   if (Inhibit == 0)
   {
      prologue (mAxis, odx, write, moduleIdx);
      commit   (mAxis, odx, write, in.data, 0, 0);
   }

   // -------------------------------------------------------------------
   // OUTPUT BODY
   // -----------
   // While the data is not copied to the output packet if Inhibit is
   // true, the copy loop always runs to ensure that the data stream is
   // drained.
   // -------------------------------------------------------------------
   COPYFRAME_LOOP:
   for (int idx = 1; idx < sizeof (WibFrame)/sizeof (uint64_t); idx++)
   {
      in = sAxis.read ();

      if (Inhibit == 0)
      {
         commit (mAxis, odx, write, in.data, 0, 0);

      }
   }

   // This is kind of a kludge, will inhibit the next frame
   bool flush = (in.user & static_cast<uint8_t>(AxisUserLast::Flush)) == 1;
   Inhibit.set (1, flush);

   if (cfg.init)
    {
       lcl.read.ntypes[0]  = 0;
       lcl.read.ntypes[1]  = 0;
       lcl.read.ntypes[2]  = 0;
       lcl.read.ntypes[3]  = 0;
       lcl.read.ntypes[4]  = 0;
       lcl.read.ntypes[5]  = 0;
       lcl.read.ntypes[6]  = 0;
       lcl.read.ntypes[7]  = 0;
       lcl.write.npromoted = 0;
       lcl.write.nwrote    = 0;
    }


    if      (  !write) lcl.write.ndropped  += 1;
    else if (!Inhibit) lcl.write.npromoted += 1;

    lcl.read.ntypes[Inhibit] += 1;

    // !!! KLUDGE !!!
    uint64_t status = 0;
    bool output = epilogue (mAxis, status, odx, write, Inhibit);
    if (output) lcl.write.nwrote += 1;

   // -----------------------------------------
    // Update the global write status monitoring
    // -----------------------------------------
    gbl = lcl;

   return;
}
/* ----------------------------------------------------------------------- */
/* END: PROCESS_K_COPY                                                     */
/* =-===================================================================== */




/* =-===================================================================== */
#elif PROCESS_K_MODE == PROCESS_K_DATAFLOW
/* ----------------------------------------------------------------------- */






/* ----------------------------------------------------------------------- */
/* PROTOTYPES: Local                                                       */
/* ----------------------------------------------------------------------- */
#if 0
static void    copy        (MonitorRead        &dst,
                            MonitorRead        &src);
#endif
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Top level entry to the data handling and compression
 *
 *  \param[ in]     sAxis  The  input data stream
 *  \param[out]      mAxis The output data stream
 *  \param[ in]  moduleIdx This refers to which group of channels are
 *                         being processed. Channels typically arrive
 *                         and are processed in groups of 128. An RCE
 *                         currently service 256 channels or 2 groups
 * \param[ in]        cfg  Configuration information including a flag
 *                         indicating that the system needs to be
 *                         configured or reconfigured.
 * \param[out]    monitor  Information that is used to monitor the
 *                         performance and internal workings of this
 *                         module.
\* ----------------------------------------------------------------------- */
void DuneDataCompressionCore(AxisIn            &sAxis,
                             AxisOut           &mAxis,
                             ModuleIdx_t    moduleIdx,
                             ModuleConfig         cfg,
                             MonitorModule   &monitor)
{
    #pragma HLS INTERFACE ap_none port=moduleIdx

    // This disables the FIFO interfaces for these variables
    //  -- Well, we tried, but this did not seem to do it
    //     The global parameters in the Solution Settting seem
    //     to take precedence.
    #pragma HLS STREAM variable=cfg     off
    #pragma HLS STREAM variable=monitor off


   // Set the input and output ports as AXI4-StreaNm
   #pragma HLS INTERFACE axis port=sAxis  // Input Port
   #pragma HLS INTERFACE axis port=mAxis  // Output Port


   // Setup a AXI-Lite bus for status/configuration registers
   #pragma HLS INTERFACE s_axilite port=cfg              bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=monitor register bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=return           bundle=BUS_A


   #pragma HLS DATA_PACK variable=monitor.cfg
   #pragma HLS DATA_PACK variable=monitor.read
   #pragma HLS DATA_PACK variable=monitor.write
   #pragma HLS DATA_PACK variable=cfg

   static ReadFrame     frame;
   #pragma HLS STREAM   variable=frame off

   static ReadContext   readCtx;
   #pragma HLS STREAM variable=readCtx off
   //#pragma HLS RESOURCE variable=readCtx core=RAM_2P_LUTRAM


   static   MonitorModule  lclMonitor;
   #pragma HLS STREAM   variable=lclMonitor off
   #pragma HLS RESOURCE variable=lclMonitor.read.nWibErrs   core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=lclMonitor.read.nStates    core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=lclMonitor.read.nFrameErrs core=RAM_2P_LUTRAM


   // Normally the chns array would be implemented as RAM_2P_LUTRAM
   // But in the straight copy, this array is unused and is optimized away
   ///#pragma HLS RESOURCE variable=lclCfg.chns  core=RAM_2P_LUTRAM
   ModuleConfig lclCfg;
   #pragma HLS STREAM   variable=lclCfg off


   static WriteContext writeCtx;

   #pragma HLS DATAFLOW

   // Can only allow change modes on 1024 packet boundaries
#if 0
   bool allow = readCtx.nframes == 0;
   if (allow)
   {
      readCtx.mode = lclCfg.mode;
   }
#endif
   bool allow = true;

   copyCfg       (lclCfg, cfg);
   configure     (lclCfg, monitor.cfg, lclMonitor.cfg, allow);
   update_common (monitor.common,      lclMonitor.common, lclCfg);

   read_frame    (frame,  readCtx,  sAxis);
   update_read   (monitor.read,     lclMonitor.read, lclCfg, frame.status);

   write_frame   (mAxis,  writeCtx, frame,  monitor.write, lclMonitor.write);


   return;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Copies the MonitorRead.
 *
 *  \param[ in] dst  The destination MonitorRead class
 *  \param[out] src  The source MonitorRead class
 *
 *   While totally trivial, this makes implementing this as a DATAFLOW
 *   method easier.
\* ----------------------------------------------------------------------- */
void copy (MonitorRead &dst, MonitorRead &src)
{
     #pragma HLS INLINE off
     #pragma HLS PIPELINE
     dst = src;
}
/* ----------------------------------------------------------------------- */





// TEMPLATE FOR WRITING COMPRESSED HEADERS
#if 0
static void write_frame (AxisOut           &axis,
                         WriteContext       &ctx,
                         ReadFrame        &frame,
                         MonitorWrite       &gbl,
                         MonitorWrite       &lcl)
{
   ///#pragma HLS INLINE
   if (ctx.odx == 0)
   {
      ctx.hdr.reset ();
   }



   // Keep it real simple for now
   //bool write = !cfg.init && (cfg.mode == MODE_K_COPY);
   bool write = true;
   bool valid = ReadFrame::isFrameWriteable (frame.status);

   unsigned hdrs = frame.status;
   // -------------------------------------------------
   // OUTPUT HEADER
   // -------------
   // Add output header to the output frame iff this is
   //   1. the first output word of this packet ..and..
   //   2. the processFlag is true.
   // --------------------------------------------------
   if (valid)
   {
       prologue (axis, ctx.odx, write, 0);
   }

   ReadStatus_t status = frame.status;

   // Check if in compress header mode
   //       If so, hdrExc and hdrExcMsk and a bit mask of those headers that must be saved to the exception block
   //       If not, set these masks to 0, which will cause all the headers to be output in sequence
   bool   compressHdrMode;
   ap_uint<6>     hdrExc (0);  // Dense  map i,e.,  6 bits, one for each header word
   ap_uint<30> hdrExcMsk (0);  // Sparse map i.e., 30 bits, one for each frame  word


   if ((compressHdrMode = (cfg.mode == MODE_K_COMPRESSHDR)))
   {
      if (ReadFrame::isWibHdr0Bad (status)) { hdrExc.set (0); hdrExcMsk.set ( 0); }
      if (ReadFrame::isWibHdr1Bad (status)) { hdrExc.set (1); hdrExcMsk.set ( 1); }
      if (ReadFrame::isCd0Hdr0Bad (status)) { hdrExc.set (2); hdrExcMsk.set ( 2); }
      if (ReadFrame::isCd0Hdr1Bad (status)) { hdrExc.set (3); hdrExcMsk.set ( 3); }
      if (ReadFrame::isCd1Hdr0Bad (status)) { hdrExc.set (4); hdrExcMsk.set (16); }
      if (ReadFrame::isCd1Hdr1Bad (status)) { hdrExc.set (5); hdrExcMsk.set (17); }
   }

   // -------------------------------------------------------------------
   // OUTPUT BODY
   // -----------
   // While the data is not copied to the output packet is procesFlag is
   // false, the copy loop always runs to ensure that the data stream is
   // drained.
   // -------------------------------------------------------------------
   COPYFRAME_LOOP:
   for (int idx = 0; idx < sizeof (WibFrame)/sizeof (uint64_t); idx++)
   {
      #pragma HLS PIPELINE
      uint64_t      d;

      d = frame.d.read ();

      // Save to the HdrContext block or push to the output
      if (hdrExcMsk.test (idx))
      {
         ctx.hdr.addHeader(d);
      }
      else
      {
         // --------------------------------------------------------------
         // Only output the frame if valid and below the 64-bit word limit
         //  -------------------------------------------------------------
         if (valid && ctx.odx < cfg.limit)
         {
            commit (axis, ctx.odx, write, d, 0, 0);
          }
      }
   }

   // If any, add this frame's exception
   if (hdrExcMsk) ctx.hdr.addException ((hdrExc << 10) | ctx.fdx);


   // --------------------------------------------
   // OUTPUT TRAILER
   // --------------
   // Add the trailer and flush packet if either
   //    1. processFlag is not set and have non-empty packet
   //    2. reach the maximum
   // --------------------------------------------
   bool output;
   if (valid && (ctx.fdx == 1023))
   {
      if (compressHdrMode && valid && ctx.odx < cfg.limit)
      {
         ctx.hdr.commit (axis, ctx.odx, write, frame.status);
      }

      epilogue (axis, ctx.odx, write, !valid, output);
   }

   {
      WriteStatus status;
      status.nbytes = output ? ctx.odx * 8 : 0;
      status.status = output;

      update_write  (gbl,  lcl,  cfg,  status);
   }

   return;
}
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */
/* END: PROCESS_K_DATAFLOW                                                */
/* ====================================================================== */



/* ====================================================================== */
#elif PROCESS_K_MODE == PROCESS_K_COMPRESS
/* ---------------------------------------------------------------------- */

#include "BTE.h"
typedef Histogram::Entry_t APE_Table_t;


#include "AP-Common.h"
#include "AP-Encode.h"
#include "BitStream64.h"

// Define which channels to process, debug only
#define   NCHANS MODULE_K_NCHANNELS
#define BEG_CHAN  0
#define END_CHAN (BEG_CHAN + NCHANS)

#define WRITE_DATA 1


#define PRAGMA_SUB(x) _Pragma (#x)
#define PRAGMA_HLS(x) PRAGMA_SUB(x)


/*
 | This ensures that the correct number of parallel instances of Histogram:integrate_and_size +
 | APE_encode are instantiated
 |
 | Note -- NPARALLEL is used in the PRAGMA_HLS, so it must be a simple integer, ruling out
 |         doing the obvious thing of defining it as MODULES_K_NPARALLEL/8
 |
 | The '8' is they maximum number of non-parallel group of APE_encode that can be executed
 | and still meet the timing.  As of this writing it was taking about 220 cycles to execute
 | the above pair of instructions. Multiplying this by 8 means it takes ~17,600 cycles to
 | complete, safely below the allotment of around ~64K.  This should leave ample time to
 | serially write the results to the output AXI stream.
 |
 |  If the timing changes, then this '8' should be adjusted accordingly
*/
#if     MODULE_K_NCHANNELS <= 8
#define NPARALLEL  1
#define NHISTPORTS 1
#define NADCPORTS  1
#define NPRVDEPTH  2
#elif   MODULE_K_NCHANNELS <= 16
#define NPARALLEL  2
#define NHISTPORTS 2
#define NADCPORTS  2
#define NPRVDEPTH  4
#elif   MODULE_K_NCHANNELS <= 32
#define NPARALLEL  4
#define NHISTPORTS 4
#define NADCPORTS  4
#define NPRVDEPTH  8
#elif   MODULE_K_NCHANNELS <= 64
#define NPARALLEL  8
#define NHISTPORTS 8
#define NADCPORTS  8
#define NPRVDEPTH 16
#elif   MODULE_K_NCHANNELS <= 128
#define NPARALLEL   5
#define NHISTPORTS  8
#define NADCPORTS   16
//#define NHISTPORTS  16
//#define NADCPORTS   16
#define NPRVDEPTH  32
#elif   MODULE_K_NCHANNELS <= 256
#define NPARALLEL  32
#define NHISTPORTS 32
#define NADCPORTS  32
#define NPRVDEPTH  64
#elif   MODULE_K_NCHANNELS <= 320
#define NPARALLEL  40
#define NHISTPORTS 40
#define NADCPORTS  40
#define NPRVDEPTH  80
#elif   MODULE_K_NCHANNELS <= 384
#define NPARALLEL  48
#define NHISTPORTS 48
#define NADCPORTS  48
#define NPRVDEPTH  96
#elif   MODULE_K_NCHANNELS <= 512
#define NPARALLEL  64
#define NHISTPORTS 64
#define NADCPORTS  64
#define NPRVDEPTH 128
#else
#error  "MODULE_K_NCHANNELS  > 512 not supported"
#endif


/* ---------------------------------------------------------------------- *//*!
 *
 *  \class   GlobalContext
 *  \brief The Global Context
 *
\* ---------------------------------------------------------------------- */
class GlobalContext
{
public:
   GlobalContext ();

public:
   void           init ();
   WibTimestamp_t timestamp (Packets::Pidx_t pidx) const;
   WibId_t        wibId     (Packets::Pidx_t pidx) const;
   WibTimestamp_t bof       (WibTimestamp_t timestamp);

public:
   Packets                          m_packets;/*!< Circular buffer context*/
   WibId_t        m_wibId    [Packets::Count];/*!< Crate.Slot.Fiber of WIB*/
   WibTimestamp_t m_timestamp[Packets::Count];/*!< timestamp of the frame */
};
/* ---------------------------------------------------------------------- */



static ReadStatus_t
            process_frame (AxisIn                                              &axis,
                           ModuleConfig                                          cfg,
                           ReadContext                                          &ctx,
                           AdcIn_t     (*adcs)[MODULE_K_NCHANNELS][Packets::NSamples],
                           Histogram  (*hists)[MODULE_K_NCHANNELS],
                           MonitorRead                                     &monitor);

static int    reconfigure (GlobalContext &gctx);

static void        clear  (Histogram (*hists)[ Packets::Count][MODULE_K_NCHANNELS]);

static void commit_adcs_4 (AxisOut &mAxis, AdcIn_t const *adcs, int cur);

static void  write_packet (AxisOut                                            &mAxis,
                           WibId_t                                             wibId,
                           WibTimestamp_t                                  timestamp,
                           AdcIn_t const adcs[MODULE_K_NCHANNELS][Packets::NSamples],
                           Histogram                       hists[MODULE_K_NCHANNELS],
                           ModuleIdx_t                                    moduleIdx);

static void    write_adcs (AxisOut                                            &mAxis,
                           int                                                  &mdx,
                           uint64_t                                            &data,
                           int                                                  &odx,
                           AdcIn_t const adcs[MODULE_K_NCHANNELS][Packets::NSamples],
                           Histogram                       hists[MODULE_K_NCHANNELS]);

static void  write_adcs_1 (AxisOut                                  &mAxis,
                           int                                        &mdx,
                           uint64_t                                  &data,
                           int                                        &odx,
                           AdcIn_t         const   adcs[Packets::NSamples],
                           Histogram                                 &hist,
                           uint8_t                                  ichan);



// DEBUGGING -- The list of channels to dump
static const int List[] = { 0}; /// 8, 16, 24, 64, 72};


#ifndef __SYNTHESIS__
static void print_caption (int last)
{
   for (int idx = 0; idx <= last; idx++)
   {
      std::cout << std::setw(1) << std::hex << ' ' << (idx >> 4);
   }
   std::cout << std::endl;


   for (int idx = 0; idx <= last; idx++)
   {
      std::cout << std::setw(1) << std::hex << ' ' << (idx & 0xf);
   }
   std::cout << std::endl;

   return;
}
#endif



#ifndef __SYNTHESIS__
#define HIST_ENCODE_DEBUG 0
#endif

static void copy_frame    (AxisOut &mAxis, WriteContext &writeCtx,
                           AxisIn  &sAxis,  ReadContext  &readCtx,
                           ModuleConfig &lclCfg,
                           MonitorModule &monitor, MonitorModule &lclMonitor)
{
   ReadFrame frame;
   read_frame    (frame,  readCtx,  sAxis, lclCfg, monitor.read, lclMonitor.read);
   write_frame   (mAxis,  writeCtx, frame,  monitor.write, lclMonitor.write);
}


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Top level entry to the data handling and compression
 *
 *  \param[ in]     sAxis  The  input data stream
 *  \param[out]      mAxis The output data stream
 *  \param[ in]  moduleIdx This refers to which group of channels are
 *                         being processed. Channels typically arrive
 *                         and are processed in groups of 128. An RCE
 *                         currently service 256 channels or 2 groups
 * \param[ in]     config  Configuration information including a flag
 *                         indicating that the system needs to be
 *                         configured or reconfigured.
 * \param[out]    monitor  Information that is used to monitor the
 *                         performance and internal workings of this
 *                         module.
\* ----------------------------------------------------------------------- */
void DuneDataCompressionCore(AxisIn            &sAxis,
                             AxisOut           &mAxis,
                             ModuleIdx_t    moduleIdx,
                             ModuleConfig         cfg,
                             MonitorModule   &monitor)
{
    #pragma HLS INTERFACE ap_none port=moduleIdx

    // This disables the FIFO interfaces for these variables
    //  -- Well, we tried, but this did not seem to do it
    //     The global parameters in the Solution Settting seem
    //     to take precedence.
    #pragma HLS STREAM variable=cfg     off
    #pragma HLS STREAM variable=monitor off


   // Set the input and output ports as AXI4-StreaNm
   #pragma HLS INTERFACE axis port=sAxis  // Input Port
   #pragma HLS INTERFACE axis port=mAxis  // Output Port


   // Setup a AXI-Lite bus for status/configuration registers
   #pragma HLS INTERFACE s_axilite port=cfg              bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=monitor register bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=return           bundle=BUS_A


   #pragma HLS DATA_PACK variable=monitor.cfg
   #pragma HLS DATA_PACK variable=monitor.read
   #pragma HLS DATA_PACK variable=monitor.write
   #pragma HLS DATA_PACK variable=cfg


    static ReadFrame     frame;
    #pragma HLS STREAM   variable=frame off

    static ReadContext   readCtx;
    #pragma HLS STREAM variable=readCtx off
    //#pragma HLS RESOURCE variable=readCtx core=RAM_2P_LUTRAM


    static Mode_t      Mode = MODE_K_COPY;
    bool outputFlag;

    static int AxisOdx = 0;

    static   MonitorModule  lclMonitor;
    #pragma HLS STREAM   variable=lclMonitor off
    #pragma HLS RESOURCE variable=lclMonitor.read.nWibErrs   core=RAM_2P_LUTRAM
    #pragma HLS RESOURCE variable=lclMonitor.read.nStates    core=RAM_2P_LUTRAM
    #pragma HLS RESOURCE variable=lclMonitor.read.nFrameErrs core=RAM_2P_LUTRAM


    // Normally the chns array would be implemented as RAM_2P_LUTRAM
    // But in the straight copy, this array is unused and is optimized away
    ///#pragma HLS RESOURCE variable=lclCfg.chns  core=RAM_2P_LUTRAM
    ModuleConfig lclCfg;
    #pragma HLS STREAM   variable=lclCfg off
    #pragma HLS RESOURCE variable=lclCfg.chns core=RAM_2P_LUTRAM

   // Can only change modes on 1024 packet boundaries

#if 1
   if (readCtx.begPkt)
    {
       readCtx.mode = lclCfg.mode;
    }
#endif



   PROCESS:
   switch (Mode)
   {
      case MODE_K_DUMP:
      case MODE_K_TRANSPOSE:
      {
         break;
      }

      case MODE_K_COPY:
      {
#pragma HLS DATAFLOW
         static WriteContext writeCtx;


         copyCfg       (lclCfg, cfg);
         configure     (lclCfg, monitor.cfg, lclMonitor.cfg, readCtx.begPkt);
         update_common (monitor.common,      lclMonitor.common, lclCfg);

         read_frame    (frame,  readCtx,  sAxis);
         update_read   (monitor.read,     lclMonitor.read, lclCfg, frame.status);

         write_frame   (mAxis,  writeCtx, frame,  monitor.write, lclMonitor.write);
         /// copy_frame    (mAxis,  writeCtx, sAxis, readCtx, lclCfg, monitor, lclMonitor);

         break;
      }

      case MODE_K_COMPRESS:
      {
         static GlobalContext GCtx;
          #pragma HLS RESET variable=GCtx.m_timestamp off

         #pragma HLS DATAFLOW
         static Histogram                Histograms[MODULE_K_NCHANNELS];
         #pragma     HLS RESET           variable=Histograms off
         #pragma     HLS STREAM          variable=Histograms off

         //#pragma    HLS RESOURCE         variable=Histograms core=RAM_2P_LUTRAM   latency=1
         //#pragma    HLS ARRAY_PARTITION  variable=Histograms complete                 dim=1
         PRAGMA_HLS(HLS ARRAY_PARTITION  variable=Histograms cyclic factor=NHISTPORTS dim=1)


         static AdcIn_t  Adcs[MODULE_K_NCHANNELS][Packets::NSamples];
         #pragma     HLS RESET          variable=Adcs off
         #pragma     HLS STREAM         variable=Adcs off
         #pragma    HLS RESOURCE        variable=Adcs core=RAM_2P_BRAM
         PRAGMA_HLS(HLS ARRAY_PARTITION variable=Adcs cyclic  factor=NADCPORTS  dim=1)
         //#pragma    HLS ARRAY_PARTITION variable=Adcs complete                  dim=1


         // -------------------------
         // Set histogram identifiers
         // -------------------------
         HISTOGRAM_statement
         (
            static bool HistInit = true;
            if (HistInit)
            {
               Histogram::set_id (Histograms, sizeof (Histograms) / sizeof (Histograms[1]));
               HistInit = false;
            }
         )

         ReadStatus_t status = process_frame (sAxis, cfg,         readCtx,
                                              &Adcs, &Histograms, lclMonitor.read);
         update_read   (monitor.read,     lclMonitor.read, lclCfg, status);


         if (readCtx.nframes & 0x3ff)
         {
            WibTimestamp_t timestamp = 0;
            write_packet (mAxis,
                          0,
                          0,
                          Adcs,
                          Histograms,
                          moduleIdx);
         }
      }
   }

   return;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  Configures the compression context
 *  \return The index into the 1024 (nominal) when the previous buffer
 *          is safe to write.  This is typically the largest value of
 *          the leading count (NL).
 * *                                                                         */
/* ----------------------------------------------------------------------- */
static int reconfigure (GlobalContext &gctx)
{
  #pragma HLS OCCURRENCE cycle=1024

   int write_idx = 0;

   gctx.init ();

   return write_idx;
}
/* ----------------------------------------------------------------------- */





typedef ap_uint<48>           Adc48_t;

static void process_colddata (Histogram (*hists)[MODULE_K_NCHANNELS],
                               AdcIn_t   (*adcs )[MODULE_K_NCHANNELS][Packets::NSamples],
                               ReadFrame::Data             *d,
                               AdcIn_t                   *prv,
                               Packets::Oidx_t         &oidx,
                               int                     ichan);

static inline void process4 (Histogram                 hists[4],
                             AdcIn_t adcs[4][Packets::NSamples],
                             AdcIn_t                     prv[4],
                             int                           oidx,
                             Adc48_t                     adcs48,
                             int                          ichan);


static ReadStatus_t process_frame
           (AxisIn                                                   &axis,
            ModuleConfig                                            config,
            ReadContext                                               &ctx,
            AdcIn_t     (*adcs_buf)[MODULE_K_NCHANNELS][Packets::NSamples],
            Histogram                    (*histograms)[MODULE_K_NCHANNELS],
            MonitorRead                                          &monitor)
{
   static AdcIn_t Prv[128];
   #pragma HLS RESET           variable=Prv off
   #pragma HLS ARRAY_PARTITION variable=Prv cyclic factor=16 dim=1


   // /* Check if have any data */
   // if (axis[0].user != 2) return false;

   ReadStatus_t status = 0;

   ReadFrame::Data hdrs[6];


   // Wib Header Words 0 & 1
    read_wib0 (status, hdrs + 0, axis, ctx.hdrs, ctx.first, ctx.wibId);
    read_wib1 (status, hdrs + 1, axis, ctx.hdrs);

    ctx.nframes += 1;

   std::cout << "W0 = " << std::hex << hdrs[0] << std::endl;

   //WibTimestamp_t timestamp = (w0.data >> 8);
   //Packets::Pidx_t     pidx = gctx.m_packets.m_pidx;
   //Packets::Oidx_t     oidx = (timestamp & 0x3ff);
   //gctx.m_timestamp[pidx]   =  timestamp;

   Packets::Oidx_t oidx = 0;

   Histogram (*hists)[MODULE_K_NCHANNELS]                    = histograms;
   AdcIn_t   (*adcs )[MODULE_K_NCHANNELS][Packets::NSamples] =   adcs_buf;

   //gctx.m_wibId[pidx] = w1.data (gctx.m_wibId[pidx].length()-1, 0);
   int      idx = 2;

   ReadFrame::Data d[12];
   ReadFrame::Data h[ 2];
   read_colddata_hdrs (status, h, axis, ctx.hdrs + 2, ctx.cvtCnt0, 0);
   process_colddata   (hists, adcs, d, Prv, oidx, 0);

   read_colddata_hdrs (status, h, axis, ctx.hdrs + 4, ctx.cvtCnt0, 0);
   process_colddata   (hists, adcs, d, Prv, oidx, 64);

   return status;
}
/* ---------------------------------------------------------------------- */


static void process_colddata (Histogram (*hists)[MODULE_K_NCHANNELS],
                               AdcIn_t   (*adcs )[MODULE_K_NCHANNELS][Packets::NSamples],
                               ReadFrame::Data    *d,
                               AdcIn_t          *prv,
                               Packets::Oidx_t &oidx,
                               int             ichan)
 {

   // -----------------------------------------------------------
   // This loops extracts 16 x 12 bit ADCs from 3 x 64-bit values
   // ------------------------------------------------------------
   SAMPLE_EXTRACT_LOOP:
   for (int idx = 0; idx < 12/3; idx++)
   {
      // ------------------------------------------------------------
      // The reading of the 5 axi stream limits the II to 5 and is
      // is the best one can do. This rids the output of carried
      // dependency warnings.
      // ------------------------------------------------------------
      #pragma HLS PIPELINE II=5

      HISTOGRAM_statement ((*hists)[ichan].printit = ichan == 0 ? true : false;)


      // --------
      // ADCs 0-3
      // --------
      ap_uint<64>   a0 = *d++;
      Adc48_t    adcs4 = a0;
      process4 (&(*hists)[ichan], &(*adcs)[ichan], &prv[ichan], oidx, adcs4, ichan);
      ichan += 4;


      // --------
      // ADCs 4-7
      // --------
      ap_uint<64> a1 = *d++;
      adcs4 = (a1(31,0), a0(63,48));
      process4 (&(*hists)[ichan], &(*adcs)[ichan], &prv[ichan], oidx, adcs4, ichan);
      ichan += 4;

      // ---------
      // ADCs 8-11
      // ---------
      ap_uint<64> a2 = *d++;
      adcs4 = (a2 (15,0), a1(63,32));
      process4 (&(*hists)[ichan], &(*adcs)[ichan], &prv[ichan], oidx, adcs4, ichan);
      ichan += 4;

      // ----------
      // ADCs 12-15
      // ----------
      adcs4 = a2 (63,16);
      process4 (&(*hists)[ichan], &(*adcs)[ichan], &prv[ichan], oidx, adcs4, ichan);
      ichan += 4;
      }
 }



#if   !defined(PROCESS_PRINT) && !defined(__SYNTHESIS__)
#define PROCESS_PRINT 0
#endif

#if PROCESS_PRINT
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Diagnostic print out of the 4 ADCs being processed
 *
 *  \param[in]  oidx  Effectively the time sample index, although
 *                    technically the index into the output packet.
 *  \param[in] ichan  The identifying channel number of the first sample
 *  \param[in]  adc0  The ADC of \a ichan + 0
 *  \param[in]  adc1  The ADC of \a ichan + 1
 *  \param[in]  adc2  The ADC of \a ichan + 2
 *  \param[in]  adc3  The ADC of \a ichan + 3
 *
\* ---------------------------------------------------------------------- */
static void process4_adcs_print (int     oidx,
                                 int    ichan,
                                 AdcIn_t adc0,
                                 AdcIn_t adc1,
                                 AdcIn_t adc2,
                                 AdcIn_t adc3)
{
   std::cout << "Process4[" << std::setw(5) << std::hex
            <<  oidx << ':' << ichan << ']'
            << ' ' << adc0
            << ' ' << adc1
            << ' ' << adc2
            << ' ' << adc3 << std::endl;
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Diagnostic print-out of the ADCs after processing
 *
 *  \param[in]  oidx  Effectively the time sample index, although
 *                    technically the index into the output packet.
 *  \param[in] ichan  The identifying channel number of the first sample
 *
 *  \param[in]  adc0  The current  ADC of \a ichan + 0
 *  \param[in]  prv0  The previous ADC of \a ichan + 0
 *  \param[in]  sym0  The symbol to be encoded for \a ichan + 0
 *
 *  \param[in]  adc1  The current  ADC of \a ichan + 1
 *  \param[in]  prv1  The previous ADC of \a ichan + 1
 *  \param[in]  sym1  The symbol to be encoded for \a ichan + 1
 *
 *  \param[in]  adc2  The current  ADC of \a ichan + 2
 *  \param[in]  prv2  The previous ADC of \a ichan + 2
 *  \param[in]  sym2  The symbol to be encoded for \a ichan + 2
 *
 *  \param[in]  adc3  The current  ADC of \a ichan + 3
 *  \param[in]  prv3  The previous ADC of \a ichan + 3
 *  \param[in]  sym3  The symbol to be encoded for \a ichan + 3
 *
\* ---------------------------------------------------------------------- */
static void process4_adcs_prv_sym_print
           (int     oidx, int    ichan,
            AdcIn_t adc0, AdcIn_t prv0, Histogram::Symbol_t sym0,
            AdcIn_t adc1, AdcIn_t prv1, Histogram::Symbol_t sym1,
            AdcIn_t adc2, AdcIn_t prv2, Histogram::Symbol_t sym2,
            AdcIn_t adc3, AdcIn_t prv3, Histogram::Symbol_t sym3)
{
   std::cout << "cur:prv->sym["
   << std::setw(3) << std::noshowbase << std::hex << ichan << ']'
   << "  " << std::setw(5) << adc0 << ':' << std::setw(5) << prv0 << "->" << std::setw(5) << sym0
   << "  " << std::setw(5) << adc1 << ':' << std::setw(5) << prv1 << "->" << std::setw(5) << sym1
   << "  " << std::setw(5) << adc2 << ':' << std::setw(5) << prv2 << "->" << std::setw(5) << sym2
   << "  " << std::setw(5) << adc3 << ':' << std::setw(5) << prv3 << "->" << std::setw(5) << sym3
   << std::endl;

   return;
}
/* ---------------------------------------------------------------------- */


#else

/* ---------------------------------------------------------------------- */
#define process4_adcs_print(_oidx, _ichan, _adc0, _adc1, _adc2, _adc3)
#define process4_adcs_prv_sym_print(_oidx, _ichan,        \
                                    _adc0,  _prv0, _sym0, \
                                    _adc1,  _prv1, _sym1, \
                                    _adc2,  _prv2, _sym2, \
                                    _adc3,  _prv3, _sym3)
/* ---------------------------------------------------------------------- */
#endif  /* PROCESS_PRINT                                                  */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Extracts, saves and histograms 4 ADCs. The histogram is of
 *          the difference between the current and previous ADC values
 *
 *   \param[in:out]  hists The 4 histograms
 *   \param[out]      adcs  The storage for the ADCs
 *   \param[in:out]    prv  The previous values of the 4 ADCs. After
 *                          being used to form the differences they
 *                          are replaced by the current ADC values.
 *  \param[in]        oidx  The output index
 *  \param[in]      adcs48  The 4 x 12 input ADCs
 *  \param[in]       ichan  For diagnostic purposes only (printf's...)
 *
\* ---------------------------------------------------------------------- */
static void process4 (Histogram                 hists[4],
                      AdcIn_t adcs[4][Packets::NSamples],
                      AdcIn_t                     prv[4],
                      int                           oidx,
                      Adc48_t                     adcs48,
                      int                          ichan)
{
   #pragma HLS PIPELINE
   #pragma HLS   INLINE

   // ---------------------------------------------
   // Extract the 4 adcs packet into the input word
   // --------------------------------------------------
   AdcIn_t adc0 = adcs48;
   AdcIn_t adc1 = adcs48 >>= PACKET_B_ADC;
   AdcIn_t adc2 = adcs48 >>= PACKET_B_ADC;
   AdcIn_t adc3 = adcs48 >>= PACKET_B_ADC;

   // ----------------------------------------
   // Check if the frist sample in this packet
   // ----------------------------------------
   if (oidx == 0)
   {
      // --------------------------------------------------
      // The first ADC is the seed value is not compressed
      // --------------------------------------------------
      hists[0].clear ();
      hists[1].clear ();
      hists[2].clear ();
      hists[3].clear ();

      // -------------------------
      // Diagnostic only print-out
      // -------------------------
      process4_adcs_print (oidx, ichan, adc0, adc1, adc2, adc3);
   }
   else
   {
      // -----------------------------------------
      // Retrieve the adc from the previous sample
      // -----------------------------------------
      AdcIn_t prv0 = prv[0];
      AdcIn_t prv1 = prv[1];
      AdcIn_t prv2 = prv[2];
      AdcIn_t prv3 = prv[3];

      // ------------------------------------------------------------
      // Use the difference of the current ADC and its previous value
      // as the symbol to be encoded/compressed.
      // --------------------------------------------------
      Histogram::Symbol_t sym0 = Histogram::symbol (adc0, prv0);
      Histogram::Symbol_t sym1 = Histogram::symbol (adc1, prv1);
      Histogram::Symbol_t sym2 = Histogram::symbol (adc2, prv2);
      Histogram::Symbol_t sym3 = Histogram::symbol (adc3, prv3);

      // -----------------------------
      // Update the encoding histogram
      // -----------------------------
      hists[0].bump (sym0);
      hists[1].bump (sym1);
      hists[2].bump (sym2);
      hists[3].bump (sym3);

      // ------------------------
      // Diagnostic only printout
      // ------------------------
      process4_adcs_prv_sym_print (oidx, ichan,
                                   adc0, prv0, sym0,
                                   adc1, prv1, sym1,
                                   adc2, prv2, sym2,
                                   adc3, prv3, sym3);
   }

   // -------------------------------------------------------
   // Replace the previous ADC value with its current value
   // Note that the values from the last sample in the packet
   // are never used, but doing the check costs extra logic
   // so the values are just stored even though they are used.
   // -------------------------------------------------------
   prv[0] = adc0;
   prv[1] = adc1;
   prv[2] = adc2;
   prv[3] = adc3;

   // --------------------
   // Save the current adc
   // --------------------
   adcs[0][oidx] = adc0;
   adcs[1][oidx] = adc1;
   adcs[2][oidx] = adc2;
   adcs[3][oidx] = adc3;

   return;
}
/* ---------------------------------------------------------------------- */






/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Dummy GlobalContext constructor
 *
\* ---------------------------------------------------------------------- */
inline GlobalContext::GlobalContext ()
{
   #pragma HLS inline


   init ();
   return;
}
/* ---------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  Initializes the global context
 *
\* ---------------------------------------------------------------------- */
inline void GlobalContext::init ()
{
   #pragma HLS inline

   m_packets.init ();
   return;
}
/* ---------------------------------------------------------------------- */



/* --------------------------------------------------------------------- *//*!
 *
 * \brief Return the timetamp associated with the specified packet
 *        index/number, \a pidx
 *
 * \param[in]  pidx  The target packet index/number
 *
\* -------------------------------------------------------------------- */
WibTimestamp_t inline GlobalContext::timestamp (Packets::Pidx_t pidx) const
{
   #pragma HLS inline

   return m_timestamp[pidx];
}
/* -------------------------------------------------------------------- */



/* --------------------------------------------------------------------- *//*!
 *
 * \brief Return the WIB id associated with the specified packet
 *        index/number, \a pidx
 *
 * \param[in]  pidx  The target packet index/number
 *
 * \note
 *  While the timestamp can and must vary packet to packet, the WIB
 *  must stay constant in every frame.
 *
\* -------------------------------------------------------------------- */
WibId_t inline GlobalContext::wibId (Packets::Pidx_t pidx) const
{
   #pragma HLS inline

   return m_wibId[pidx];
}
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define WRITE_ADCS_PRINT 1
#endif
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct ChannelSize
 *  \brief  Captures the isze, in bits of the encoded data and overflow
 *
\* ---------------------------------------------------------------------- */
struct ChannelSize
{
    ap_uint<22>  data; /*!< Size, in bits, of the encoded data            */
    ap_uint<22>  over; /*!< Size, in bits, of the overflow ADCs           */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if WRITE_ADCS_PRINT
/* ---------------------------------------------------------------------- */

static void
  write_adcs_print (AdcIn_t const adcs[MODULE_K_NCHANNELS][Packets::NSamples])
{
   for (int ilist = 0; ilist < sizeof (List) / sizeof (List[0]); ilist++)
   {
      int ichan = List[ilist];

      std::cout << "Dumping Adc Channel: " << std::hex << std::setw (3)
       << ichan << std::endl;

      AdcIn_t const *a = adcs[ichan];

      for (int idx = 0; idx < Packets::NSamples; idx++)
      {
          #define COLUMNS 0x20
          int col = idx % COLUMNS;
          if (col == 0x0) std::cout << std::hex << std::setw(3) << idx;
          std::cout << ' ' << std::hex << std::setw(3) << a[idx];
          if (col == (COLUMNS -1)) std::cout << std::endl;
          #undef COLUMNS
      }
   }

    return;
}


static void write_sizes_print (int ichan, ChannelSize &size)
{
   std::cout << "Bit Size[0x" << std::setw (2) << std::hex << ichan << "] = 0x"
             << size.data << ":ox" << size.over << std::endl;
}


static void write_adcs_encode_chan (int ichan)
 {
 std::cout << "Encoding chan: " << std::hex << ichan << std::endl;
 }

/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */

#define write_adcs_print(_adcs)
#define write_adcs_bit_size_print(_ichan, _ebit_size)
#define write_adcs_encode_chan(_ichan)
#define write_sizes_print(_ichan, _size)

/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



static inline void
       write_adcs (AxisOut                                           &mAxis,
                   int                                                 &mdx,
                   uint64_t                                           &data,
                   int                                                 &odx,
                   AdcIn_t const adcs[MODULE_K_NCHANNELS][Packets::NSamples],
                   Histogram                       hists[MODULE_K_NCHANNELS]);

static __inline void   encodeN (APE_etx                              *etx,
                                ChannelSize      size[MODULE_K_NCHANNELS],
                                Histogram                          *hists,
                                AdcIn_t const   adcs[][Packets::NSamples],
                                int                                 nadcs,
                                int                              nstreams,
                                int                                nchans,
                                int                                 ichan);

static void             writeN (AxisOut                            &mAxis,
                                uint64_t                             &buf,
                                int                                  &mbx,
                                ap_uint<64>                       babuf[],
                                ap_uint<6>                        bavld[],
                                ap_uint<64>                       oxbuf[],
                                ap_uint<6>                        oabuf[],
                                APE_etx                             etx[],
                                ChannelSize      size[MODULE_K_NCHANNELS],
                                int                              nstreams,
                                int                                nchans,
                                int                                ichan);

static void               pack (AxisOut                           &mAxis,
                                uint64_t                            &buf,
                                int                                 &bdx,
                                ap_uint<6>                         &bodx,
                                ap_uint<64>                       &bsbuf,
                                ap_uint<6>                        &bsvld,
                                BitStream64                          &bs,
                                int                                bleft);


static void write_encoded_data (AxisOut      &mAxis,
                                int            &mdx,
                                BitStream64     &bs);

static void    write_overflow  (AxisOut      &mAxis,
                                int            &mdx,
                                BitStream64     &bs);

static void        commit_last (AxisOut      &mAxis,
                                int             mdx);


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Writes one packets worth of data to the output stream
 *
 *   \param[out]     mAxis  The output data stream
 *   \param[ in]     wibId  The crate, slot, fiber # of the WIB fiber
 *   \param[ in] timestamp  The timestamp of the packet to be written
 *   \param[ in]      adcs  The ADCs to be compressed
 *   \param[ in]     hists  The histograms used to compose the compression
 *                          tables
 *   \param[ in] moduleIdx  The module idx of this group of channels
 *
 *   \par
 *   Packets are only written when a full 1024 time samples have been
 *   accumulated and only when the timestamp % 1024 = 1023. This keeps
 *   the packets from all RCEs synchronized.
 *
 *   \note
 *    The frame index/number and timestamp are that of the frame
 *    to be written. This is typically the frame previous to the frame
 *    being processed. It is the caller's responsibility to ensure that
 *    this is true.
 *
\* ---------------------------------------------------------------------- */
static void
   write_packet (AxisOut                                            &mAxis,
                 WibId_t                                             wibId,
                 WibTimestamp_t                                  timestamp,
                 AdcIn_t const adcs[MODULE_K_NCHANNELS][Packets::NSamples],
                 Histogram                       hists[MODULE_K_NCHANNELS],
                 ModuleIdx_t                                     moduleIdx)
{
   #define COMPRESS_VERSION 0x10000000

#if 0
   if (Packets::is_last (timestamp))
   {
      timestamp  = Packets::bof (timestamp);
   }
   else
   {
      return;
   }
#endif

   int   mdx = 0;
   uint64_t data;

   uint32_t frameId  = Frame::V0::identifier (Frame::V0::DataType::COMPRESSED);
   uint64_t hdr = Frame::V0::header (frameId);
   commit (mAxis, mdx, true, hdr,       0x2, 0);
   commit (mAxis, mdx, true, wibId,     0x0, 0);
   commit (mAxis, mdx, true, timestamp, 0x0, 0);

/*
   WRITE_PACKET_CHANNEL_LOOP:
   for (int ichan = 0; ichan < MODULE_K_NCHANNELS; ichan++)
   {
      write_adcs_1 (mAxis,
                    mdx,
                    data,
                    odx,
                    adcs[ichan],
                    hists[ichan],
                    ichan);
   }

   /// !!! KLUDGE !!!
   return;
   */

   write_adcs (mAxis, mdx, data, mdx, adcs, hists);


   /*
    *  Set end-of-frame marker
    *
    *  !!! WARNING !!!
    *  The receiver must 'backfill' the length in the
    *  header because mAxis is only sequentially accessible
    */
   uint64_t tlr = Frame::V0::trailer (frameId, mdx << 3);
   commit (mAxis, mdx, true,  tlr, 0x0, 0x1);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the adcs for all channels to the output stream
 *
 *  \param[in]     mAxis  The output stream
 *  \param[in:out]   mdx  The index into the ouput stream, mAxis
 *  \param[in:out]  data  The 64-bit data accumulation buffer
 *  \param[in:out]   odx  The 16-bit index into \a data
 *  \param[in]      adcs  The adcs to encode
 *  \param[in:out] hists  The histograms of the ADCs. These are transformed
 *                        into the cumulative distributions used to do
 *                        the actual encoding
 *
\* ---------------------------------------------------------------------- */
static inline void
       write_adcs (AxisOut                                            &mAxis,
                   int                                                  &mdx,
                   uint64_t                                            &data,
                   int                                                  &odx,
                   AdcIn_t const adcs[MODULE_K_NCHANNELS][Packets::NSamples],
                   Histogram                       hists[MODULE_K_NCHANNELS])
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   write_adcs_print (adcs);


   #define NSERIAL ((NCHANS + NPARALLEL -1)/NPARALLEL)

   APE_etx                  etx[NPARALLEL];
   ChannelSize   sizes[MODULE_K_NCHANNELS];

   #pragma     HLS ARRAY_PARTITION variable=etx         complete dim=1
   PRAGMA_HLS (HLS ARRAY_PARTITION variable=sizes dim=1 factor=NPARALLEL)

   // ------------------------------------
   // Transform 64-bit index to bit index
   // ------------------------------------
   int bdx = mdx << 6;
   uint64_t buf;
   ap_uint<64> babuf[NPARALLEL];
   ap_uint<6>  bavld[NPARALLEL] = {0};
   ap_uint<64> oabuf[NPARALLEL];
   ap_uint<6>  oavld[NPARALLEL] = {0};


   WRITE_ADCS_CHANNEL_LOOP:
   for (int ichan = 0; ichan < NCHANS; ichan += NPARALLEL)
   {
      #pragma HLS UNROLL
      #pragma DATAFLOW

      write_adcs_encode_chan (ichan);
      encodeN (etx, sizes, hists,  adcs, PACKET_K_NSAMPLES, NPARALLEL, NSERIAL, ichan);
      //writeN  (mAxis, buf, bdx,   babuf, bavld,
      //                            oabuf, oavld, etx, sizes, NPARALLEL, NSERIAL, ichan);
   }

   return;
}
/* ---------------------------------------------------------------------- */




/* -------------------------------------------------------------------- *//*!
 *
 *  \brief Encode the \a ichan channels in parallel
 *
 *  \param[in:out]  etx  The vector of encoding contexts
 *  \param[in:out] hist  The vector of histograms to use generate
 *                       compression tables
 *  \param[in]     adcs  The vector of adc samples
 *  \param[in]    nadcs  The number of adcs to encode for each channel
 *  \param[in] nstreams  The number of parallel streams
 *  \param[in]   nchans  The number of channels per stream
 *  \param[in]    ichan  The channel number of the first channel to
 *                       be encoded
 *
\* -------------------------------------------------------------------- */
static __inline void encodeN (APE_etx                              *etx,
                              ChannelSize     sizes[MODULE_K_NCHANNELS],
                              Histogram                          *hists,
                              AdcIn_t const   adcs[][Packets::NSamples],
                              int                                 nadcs,
                              int                              nstreams,
                              int                                nchans,
                              int                                 ichan)
{
   #pragma HLS PIPELINE
   #pragma HLS INLINE


   ENCODE_N_ENCODELOOP:
   for (int idx = 0; idx < nstreams; idx++)
   {
      #pragma HLS UNROLL

      HISTOGRAM_statement (hists[ichan].print ());

      // ---------------------------------------
      // Capture bit indices before the encoding
      // ---------------------------------------
      BitStream64::Idx_t ba_prv_idx = etx[idx].ba.m_idx;
      BitStream64::Idx_t oa_prv_idx = etx[idx].oa.m_idx;


      // ------------------------------------------------
      // Encode the histogram, adcs and the overflow adcs
      // ------------------------------------------------
      APE_encode (etx[idx], hists[ichan], adcs[ichan], Packets::NSamples);


      // --------------------------------------------------------
      // Compute the bit size of this channel's data and overflow
      // --------------------------------------------------------
      sizes[ichan].data = etx[idx].ba.m_idx - ba_prv_idx;
      sizes[ichan].over = etx[idx].oa.m_idx - oa_prv_idx;
      write_sizes_print (ichan, sizes[ichan]);


      // -------------------------------------------------------
      // Increment the channels by the number of channels
      // and check if done with all the channels.  This check is
      // necessary since the number of channels may not be an
      // integer multiple of the number of streams.
      // -------------------------------------------------------
      ichan     += 1;
      if (ichan >= NCHANS)
      {
         break;
      }
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if WRITE_DATA
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the \a ichan channels in parallel
 *
 *  \param[in:out] mAxis The output stream
 *  \param[in:out]   buf The temporary staging buffer
 *  \param[in:out]   mdx The bit index of the output array
 *  \param[in]       etx The vector of encoding contexts
 *  \param[in]       end The last bit of each channel
 *  \param[in]  nstreams The number of parallel streams
 *  \param[in]    nchans The number of channels per stream
 *  \param[in]     ichan The channel number of the first channel to
 *                       be encoded
 *
\* -------------------------------------------------------------------- */
static void writeN (AxisOut                        &mAxis,
                    uint64_t                         &buf,
                    int                              &bdx,
                    ap_uint<64>                   babuf[],
                    ap_uint<6>                    bavld[],
                    ap_uint<64>                   oabuf[],
                    ap_uint<6>                    oavld[],
                    APE_etx                         etx[],
                    ChannelSize sizes[MODULE_K_NCHANNELS],
                    int                          nstreams,
                    int                            nchans,
                    int                             ichan)
{
   WRITE_CHANNEL_LOOP:

   ap_uint<6>   bodx = bdx;


   for (int idx = 0;  idx < nstreams; idx++)
   {
      #pragma HLS PIPELINE

      pack (mAxis, buf, bdx, bodx, babuf[idx], bavld[idx], etx->ba, sizes[ichan].data);
      pack (mAxis, buf, bdx, bodx, oabuf[idx], oavld[idx], etx->oa, sizes[ichan].over);


      if (ichan > NCHANS)
      {
         return;
      }
   }

   return;
}
/* -------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define PACK_DATA_PRINT 1
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if  PACK_DATA_PRINT

static void pack_data_print (int        bleft,
                             ap_uint<64> data,
                             int       rshift,
                             int       lshift)
{
std::cout << "Pack data read("
          << std::hex << bleft << " = " << std::setw(16) << data
          << "l:rshift" << std::setw(2) << lshift << ':'
          << rshift << std::endl;
}

/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */

#define pack_data_print(_bleft, _data, _rshift, _lshift)

#endif
/* -------------------------------------------------------------------- */


static void pack (AxisOut                           &mAxis,
                  uint64_t                            &buf,
                  int                                 &bdx,
                  ap_uint<6>                         &bodx,
                  ap_uint<64>                       &bsbuf,
                  ap_uint<6>                        &bsvld,
                  BitStream64                          &bs,
                  int                                bleft)
 {
   // Get the index of the first bit in this stream
   ap_uint<6> bbeg = bdx;
   int         mdx = bdx >> 6;
   int rshift =  0;
   int lshift = 64;

   // ----------------------------------------------------------------
   // Need to merge the shards of
   //     a) the buffered output stream --        buf
   // and b) the leftovers from the input stream  bsbuf of bsvld bits
   // ---------------------------------------------------------------


   // ----------------------------------
   // Count the bits that will be output
   // when the loop is completed.
   // -----------------------------------
   ap_uint<64> data;
   bdx   += bleft;
   rshift = 64 - lshift;
   while (bleft > 0)
   {
      buf  <<= lshift;
      data   = bs.m_out.read ();
      pack_data_print (bleft, data, rshift, lshift);
      buf   |= data >> rshift;
      commit (mAxis, mdx, true, buf, 0, 0);
      bleft -= 64;
   }

   // If this 64-bit index is not 0, then have a left over output piece
    if (bsvld)
    {
       // Is there room for these bits
       int over = 64 - (bodx + bsvld);
       if (over < 0)
       {
          // The shard fits into the current buffer
          buf  <<=  bsvld;
          buf   |=  bsbuf;
          lshift = -over;
          bodx  += bsvld;
       }
       else
       {
          // The shard does not fit into the current buffer
          // Put in what can be, and hold onto the rest
          buf  <<=  bsvld - over;
          buf   |=  bsbuf >> over;
          commit (mAxis, mdx, true, buf, 0, 0);
          buf    = bsbuf;
          lshift = 64 - over;
          bodx  += bsvld - over;
      }
    }


   // Bits leftover from the input buffer
   bsvld =  -bleft;
   bsbuf =  data & ((1 << bleft) - 1);


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes out the compressed data
 *
 *  \param[in:out]  mAxis The output data stream
 *  \param[in:out]    mdx The output index
 *  \param[in:out]     bs  The bit stream holding the compressed data
 *
\* ---------------------------------------------------------------------- */
static inline void write_encoded_data (AxisOut      &mAxis,
                                       int            &mdx,
                                       BitStream64     &bs)
{
   #pragma HLS INLINE

   int n64 = (bs.flush () + 63) >> 6;
   mdx += n64;


   WRITE_ENCODED_CHANNEL_LOOP:
   for (int idx = 0; idx < n64; idx++)
   {
      #pragma HLS PIPELINE
      #pragma HLS LOOP_TRIPCOUNT min=40 max=192 avg=96
      uint64_t w64;
      bs.m_out.read_nb (w64);
      commit (mAxis, mdx, true, w64, 0, 0);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes out the encoded statistics histogram and the values
 *         to restore the overflow ADCs
 *
 *  \param[in:out]  mAxis The output data stream
 *  \param[in:out]    mdx The output index
 *  \param[in:out]     bs  The bit stream holding the histogram and
 *                         overflow bins
 *
\* ---------------------------------------------------------------------- */
static void write_overflow (AxisOut  &mAxis,
                            int        &mdx,
                            BitStream64 &bs)
{
   int n64 = (bs.flush () + 63) >> 6;
   mdx += n64;

   WRITE_OVERFLOW_LOOP:
   for (int idx = 0; idx < n64; idx++)
   {
       #pragma HLS PIPELINE
       #pragma HLS LOOP_TRIPCOUNT min=1 max=192 avg=8

       uint64_t w64;
       bs.m_out.read_nb (w64);
       commit (mAxis, mdx, true, w64, 0, 0);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Writes the Adc block to the CPU for one channel to the output
 *          stream.
 *
 *  \param[out]    mAxis  The output AXI-stream
 *  \param[in]      adcs  The buffer of adcs
 *  \param[in]      dscs  The array of waveform descriptors to use
 *                        when filling the output buffer
 *  \param[ in]    ndscs  The number of waveform descriptors to write
 *  \param[ in]    ichan  The channel being promoted
 *
\* ---------------------------------------------------------------------- */
static void write_adcs_1 (AxisOut                                  &mAxis,
                          int                                        &mdx,
                          uint64_t                                  &data,
                          int                                        &odx,
                          AdcIn_t         const   adcs[Packets::NSamples],
                          Histogram                                 &hist,
                          uint8_t                                  ichan)
{
   #pragma HLS inline

   // Loop over the ADCs to write
   WRITE_ADCS_1:
   for (int idx = 0; idx < PACKET_K_NSAMPLES; idx += 4)
   {
      ///#pragma HLS PIPELINE
      commit_adcs_4 (mAxis, &adcs[idx], idx & 0x3ff);
   }

   mdx += PACKET_K_NSAMPLES / 4;
   odx = 0;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Commits 4 ADCs to the output stream
 *
 *  \param[out]  mAxis  The output stream
 *  \param[ in]   adcs  The array of 4 adcs to commit to the output stream
 *  \param[ in]    cur  Index of the first adc
 *
\* ---------------------------------------------------------------------- */
static void commit_adcs_4 (AxisOut &mAxis, AdcIn_t const *adcs, int cur)
{
   #pragma HLS inline
   #pragma HLS PIPELINE

   uint64_t   data = ((uint64_t)adcs[0] <<  0)
                   | ((uint64_t)adcs[1] << 16)
                   | ((uint64_t)adcs[2] << 32)
                   | ((uint64_t)adcs[3] << 48);

   int odx = 0;
   commit (mAxis, odx, true, data, 0, 0);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Commits the last word to the output stream
 *
 *  \param[in]  axis The output stream
 *  \param[in]   tag A 32-bit value that is placed in the lower 32-bits
 *                   to the last 64-bit word. This is typically the
 *                   number of valid bits written.
 *
\* ---------------------------------------------------------------------- */
static void commit_last (AxisOut &axis,  int mdx)
{
#pragma HLS inline

   AxisOut_t out;

   out.data = ((uint64_t)0xFFFFFFFF << 32) | mdx;
   out.keep = 0xFF;
   out.strb = 0x0;
   out.user = 0x0;
   out.last = 0x1;
   out.id   = 0x0;
   out.dest = 0x0;

   axis.write (out);
   mdx += 1;
}
/* ---------------------------------------------------------------------- */

#endif /* WRITE_DATA                                                      */
/* ---------------------------------------------------------------------- */




#else

#error ERROR: PROCESS_K_MODE must be set to one of PROCESS_K_COPY, PROCESS_K_DATAFLOW or PROCESS_K_COMPRESS

#endif

/* ====================================================================== */
/* IMPLEMENtATION: Common Routines                                        */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Make a local copy of the configuraion block
 *
 *   \param[out]  lcl The local  copy of the configuration block
 *   \param[ in]  gbl The global copy of the configuration block.
 *
\* ---------------------------------------------------------------------- */
static void copyCfg (ModuleConfig &lcl, ModuleConfig &gbl)
{
    #pragma HLS PIPELINE
    lcl      = gbl;
    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define WRITE_DATA_PRINT 1
#endif
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#if WRITE_DATA_PRINT
/* ---------------------------------------------------------------------- */
static void print_write_data (int odx, uint64_t data)
{
   std::cout << "Writing[" << std::setw(5) << odx << "] = " << std::setw(16) << std::hex << data << std::endl;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define print_write_data(_odx, _data)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Commits 1 64-bit value to the output stream
 *
 *   \param[   out]  axis  The output AXI stream
 *   \param[in:out]   odx  The output index
 *   \param[    in] write  Flag to do the write (if false, no write)
 *   \param[    in]  data  The 64-bit data to commit
 *   \param[    in[  user  The value of the user field
 *
\* ---------------------------------------------------------------------- */
static void commit (AxisOut &axis,
                    int      &odx,
                    bool    write,
                    uint64_t data,
                    int      user,
                    int      last)
{
   #pragma HLS inline
   #pragma HLS PIPELINE

   if (write)
   {
      AxisOut_t out;
      out.data = data;
      out.keep = 0xFF;
      out.strb = 0x0;
      out.user = user;
      out.last = last;
      out.id   = 0x0;
      out.dest = 0x0;

      print_write_data (odx, data);


      axis.write (out);
      odx         += 1;
   }

   return;
}
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Updates the common/static status
 *
 *  \param[    in] init  Is this an initialization request *
 *  \param[   out]  gbl  The module's global configuration status.
 *                       This is updated with the local read status
 *  \param[in:out]  lcl  The module's local  configuration status
 *                    .  This is copied into the global configuration
 *                       status after it has been updated.
 *
\* ---------------------------------------------------------------------- */
static void update_common (MonitorCommon       &gbl,
                           MonitorCommon       &lcl,
                           ModuleConfig const  &cfg)
{
     lcl.pattern = 0xabadcafe;
     gbl         = lcl;

     return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Configures the module
 *
 *  \param[     in]     cfg  The configuration block
 *  \param[   out]      gbl  The module's global configuration status.
 *                           This is updated with the local read status
 *  \param[in:out]      lcl  The module's local  configuration status
 *                        .  This is copied into the global configuration
 *                           status after it has been updated.
 *
\* ---------------------------------------------------------------------- */
static void configure (ModuleConfig const  &cfg,
                       MonitorCfg          &gbl,
                       MonitorCfg          &lcl,
                       bool               allow)
{
   // Only honor configuration requests if allowed and requested on allowable
   if (cfg.init && allow)
   {
      lcl.ncfgs  += 1;
   }

   gbl = lcl;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Convenience method to launder a ReadFrame::Offset to an
 *          unsigned int for use in the bit set method.
 *
 *   \param[in:out] status  The read status bit mask
 *   \param[in]        val  Flag to set the bit to a 0 or a 1
 *   \param[in]        bit  The bit number to modify
 *
\* ---------------------------------------------------------------------- */
static void update (ReadStatus_t        &status,
                    bool                    val,
                    enum ReadFrame::Offset  bit)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   status.set (static_cast<unsigned>(bit), val);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Convenience method to clear of set the appropriate state bit.
 *
 *   \param[in:out] status  The read status bit mask
 *   \param[in]        val  Flag to set the bit to a 0 or a 1
 *   \param[in]        bit  The beginning state bitb
 *   \param[in]        off  The offset (0-3) to the appropriate state bit.
 *
 *    The four states are
 *       -0.  Normal, run in progress
 *        1.  Run disabled
 *        2.  Flush, the frame should be read and disposed of (not written
 *        3.  Disabled and Flush, the run is disabled and the frame should
 *            be flushed.
 *
\* ---------------------------------------------------------------------- */
static void update (ReadStatus_t        &status,
                    bool                    val,
                    enum ReadFrame::Offset  bit,
                    int                     off)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   status |= val << static_cast<unsigned>(bit) + off;

   /////  This was tried, but the synthesizer issues a warning that this
   ////// construct may result in non-optimal results.
   /////  status.set (static_cast<unsigned>(bit) + off, val);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the first WIB header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The first WIB header word
 *  \param[in]      wibId  The WIB identity, \e i.e. the Crate.Slot.Fiber
 *                         number.  This is compared with the identity in
 *                         the \a hdr.
 *
\* ---------------------------------------------------------------------- */
static void eval_wib0 (ReadStatus_t &status, uint64_t hdr, uint16_t wibId)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   // Check that the first word is as expected
   //    The Wib Comma    field must be WibFrame:K28_5
   //    The Wib Version  field must be WibFrame::VersionNumber
   //    The Wib Id       field must be wibId
   //    The Wib Reserved field must be 0
   //    The Wib Error field must be 0
   bool wibErrCommaChar = WibFrame::commaChar(hdr) != WibFrame::K28_5;
   bool wibErrVersion   = WibFrame::version  (hdr) != WibFrame::VersionNumber;
   bool wibErrId        = WibFrame::id       (hdr) != wibId;
   bool wibErrRsvd      = WibFrame::rsvd     (hdr) != 0;
   bool wibErrErrors    = WibFrame::wibErrors(hdr) != 0;

   update (status, wibErrCommaChar, ReadFrame::ErrWibComma);
   update (status, wibErrVersion,   ReadFrame::ErrWibVersion);
   update (status, wibErrId,        ReadFrame::ErrWibId);
   update (status, wibErrRsvd,      ReadFrame::ErrWibRsvd);
   update (status, wibErrErrors,    ReadFrame::ErrWibErrors);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the second WIB header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The second WIB header word
 *  \param[in]        exp  The expected header word value
 *
 *
 *   Except for one mode 1, the header contains the timestamp.  The
 *   expected value, \a exp, has had the timestamp updated by the
 *   number of ticks between timesamples.  With this update, this it
 *   must match \a hdr.
 *
\* ---------------------------------------------------------------------- */
static void eval_wib1 (ReadStatus_t &status, uint64_t hdr, uint64_t prv)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   bool wibErrTimestamp = hdr != prv;
   update (status, wibErrTimestamp, ReadFrame::Offset::ErrWibTimestamp);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the first cold data header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The first WIB cold data link header word
 *  \param[in]  gotCvtCnt  The convert count that was previously extracted
 *                         from the header
 *  \param[in]  expCvtCnt  The expected convert count. This is based on
 *                         incrementing the previous convert count by 1.
 *  \param[in]        off  This is the bit offset to the base of the
 *                         appropriate cold data link; basically selects
 *                         the target cold data link.
 *
\* ---------------------------------------------------------------------- */
static void eval_colddata0 (ReadStatus_t &status,
                            uint64_t         hdr,
                            uint16_t   gotCvtCnt,
                            uint16_t   expCvtCnt,
                            int              off)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   // Check that the word is as expected
   //       The Stream Err1 field must be 0
   //       The Stream Err2 field must be 0
   //       The Reserved    field must be 0
   //       The Convert Count field must be expect convert count
   //
   // Note: This word also contains the checksums. Because the data that
   //       went into forming the check sums is no longer present, this
   //       field cannot be checked.
   bool cdErrStreamErr1 = WibFrame::ColdData::streamErr1 (hdr) != 0;
   bool cdErrStreamErr2 = WibFrame::ColdData::streamErr2 (hdr) != 0;
   bool cdErrRsvd0      = WibFrame::ColdData::rsvd0      (hdr) != 0;
   bool cdErrCvtCnt     = gotCvtCnt                            != expCvtCnt;

   update (status, cdErrStreamErr1, ReadFrame::ErrCd0StrErr1, off);
   update (status, cdErrStreamErr2, ReadFrame::ErrCd0StrErr2, off);
   update (status, cdErrRsvd0,      ReadFrame::ErrCd0Rsvd0,   off);
   update (status, cdErrCvtCnt,     ReadFrame::ErrCd0CvtCnt,  off);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the second cold data header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The second WIB cold data link header word
 *  \param[in]        prv  The previous header word.
 *  \param[in]        off  This is the bit offset to the base of the
 *                         appropriate cold data link; basically selects
 *                         the target cold data link.
 *
\* ---------------------------------------------------------------------- */
static void eval_colddata1 (ReadStatus_t &status, uint64_t hdr, uint64_t prv, int off)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   bool cdErrErrReg = WibFrame::ColdData::errReg (hdr) != 0;
   bool cdErrHdrs   = WibFrame::ColdData::hdrs   (hdr) != WibFrame::ColdData::hdrs (prv);
   bool cdErrRsvd1  = WibFrame::ColdData::rsvd1  (hdr) != 0;

   update (status, cdErrErrReg, ReadFrame::ErrCd0ErrReg, off);
   update (status, cdErrRsvd1,  ReadFrame::ErrCd0Rsvd1,  off);
   update (status, cdErrHdrs,   ReadFrame::ErrCd0Hdrs,   off);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads and evaluates the first WIB header word
 *
 *  \param[in:out] status The read status word to be updated by an abnormal
 *                        conditions
 *  \param[out]         d Array to capture the header word
 *  \param[in]       axis The AXI stream to read the data from
 *  \param[in:out]   hdrs On input, this is the expected header value based
 *                        on the previous set. THis value is updated
 *                        based on the read header word
 * \param[in:out]   first A first time flag used to capture the WIB identity
 * \param[in:out]   wibId The WIB identity.  This is set on the first frame
 *                        and used as comparison value from then on.
 *
\* ---------------------------------------------------------------------- */
static void read_wib0 (ReadStatus_t &status,
                       ReadFrame::Data   *d,
                       AxisIn         &axis,
                       AxisWord_t     *hdrs,
                       bool          &first,
                       ap_uint<11>   &wibId)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   AxisIn_t in = axis.read ();

   // Check the state of Start and End Of Frame bits
   bool    sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   bool    eof = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));

   // Check
   //  1) Start Of Frame is not missing and
   //  2) End   Of Frame is not present
   if (!sof) status.set (ReadFrame::ErrSofM);
   if ( eof) status.set (ReadFrame::ErrEofU);


   // Check the run enable bit
   // Note: The flush bit is only present on the last word in the frame
   bool  runEnable = in.user.test (static_cast<uint8_t>(AxisUserFirst::RunEnable));
   if  (!runEnable) status.set (ReadFrame::RunDisable);

   // !!! KLUDGE !!!
   // Seed the Wib Id with the first scene
   // This is not a great idea
   if (first)
   {
      first   = false;
      wibId   = WibFrame::id (in.data);
   }

   //  Check that all the fields are as expected
   eval_wib0 (status, in.data, wibId);

   // Capture the header and use as the seed for the next frame
   hdrs[0]   = in.data;
   d   [0]   = in.data;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads and evaluates the second WIB header word, this is
 *          the timestamp.
 *
 *  \param[in:out] status The read status word to be updated by an abnormal
 *                        conditions
 *  \param[out]         d Array to capture the header word
 *  \param[in]       axis The AXI stream to read the data from
 *  \param[in:out]   hdrs On input, this is the expected header value based
 *                        on the previous set. THis value is updated
 *                        based on the read header word
 *
\* ---------------------------------------------------------------------- */
static void read_wib1 (ReadStatus_t &status,
                       ReadFrame::Data   *d,
                       AxisIn         &axis,
                       AxisWord_t     *hdrs)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   // This is the timestamp
   AxisIn_t in = axis.read ();

   // Check the state of Start and End Of Frame bits
   bool    sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   bool    eof = in.last.test (static_cast<uint8_t>(AxisLast     ::Eof));


   // Should not see the SOF or EOF marker
   if (sof) status.set (ReadFrame::ErrSofU);
   if (eof) status.set (ReadFrame::ErrEofU);


   //  Check that all the fields are as expected
   eval_wib1 (status, in.data, hdrs[1]);

   // Capture the header and update for use as the seed for the next frame
   hdrs[1] = in.data + 25;
   d   [1] = in.data;
   //d.write_nb (in.data);

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
static void read_colddata_hdrs (ReadStatus_t &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                AxisWord_t     *hdrs,
                                uint16_t  &expCvtCnt,
                                int              off)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE II=2

   // ---------------------------------------------------------------------
   // Colddata link, word 0
   //
   AxisIn_t in = axis.read ();


   // Check the state of Start and End Of Frame bits
   bool    sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   bool    eof = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));


   // Should not see the SOF or EOF marker
   if (sof) status.set (ReadFrame::ErrSofU);
   if (eof) status.set (ReadFrame::ErrEofU);


   // Extract the convert count,
   // Check that all the fields are as expected
   // Update the convert count to the new value
   uint16_t gotCvtCnt = WibFrame::ColdData::cvtCnt (in.data);
   eval_colddata0 (status, in.data, gotCvtCnt, expCvtCnt, off);
   expCvtCnt  = gotCvtCnt + 1;


   // Capture the header and update for use as the seed for the next frame
   hdrs[0] = in.data;
   d   [0] = in.data;
   //
   // ---------------------------------------------------------------------



   // ---------------------------------------------------------------------
   // Colddata link, word 1
   //
   in   = axis.read ();


   // Check the state of Start and End Of Frame bits
   sof  = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   eof  = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));


   // Should not see the SOF or EOF marker
   if (sof) status.set (ReadFrame::ErrSofU);
   if (eof) status.set (ReadFrame::ErrEofU);


   // Check that all the fields are as expected
   eval_colddata1 (status, in.data, hdrs[1], off);


   // Capture the header and update for use as the seed for the next frame
   hdrs[1] = in.data;
   d   [1] = in.data;
   // ---------------------------------------------------------------------
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void read_colddata_data (ReadStatus_t &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                bool       checkLast)
{
    #pragma HLS INLINE

    AxisIn_t in;

   // Capture the data
   READCOLDDATA_LOOP:
   for (int idx = 0; idx < 12; idx++)
   {
      // Get the input word, always copy the data
      in     = axis.read ();
      d[idx] = in.data;


      // Check the state of Start and End Of Frame bits
      bool sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
      bool eof = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));


      // Should not see the SOF marker
      if (sof) status.set (ReadFrame::ErrSofU);


      // Check if this is not the last word
      if (idx != 11 || !checkLast)
      {
         // Not the last word, should not see EOF marker
         if (eof) status.set (ReadFrame::ErrEofU);
      }
      else
      {
         // Handle the very last word
         // Check the presence of flush and eof on the last input word
         bool flush  = in.user.test (static_cast<unsigned>(AxisUserLast::Flush));
         bool eoferr = in.user.test (static_cast<unsigned>(AxisUserLast::EofErr));

         if (flush ) status.set (ReadFrame::Flush);
         if (!eof  ) status.set (ReadFrame::ErrEofM);
         if (eoferr) status.set (ReadFrame::ErrEofE);
         break;
      }
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads and evaluates the two WIB cold data header words,
 *
 *  \param[in:out] status The read status word to be updated by an abnormal
 *                        conditions
 *  \param[out]         d Array to capture the header word
 *  \param[in]       axis The AXI stream to read the data from
 *  \param[in:out]   hdrs On input, this is the expected header value based
 *                        on the previous set. This value is updated
 *                        based on the read header word.
 *  \param[in:out] cvtCnt On input, the expect convert count, on output,
 *                        the updated convert count
 *  \param[in]        off  This is the bit offset to the base of the
 *                         appropriate cold data link; basically selects
 *                         the target cold data link.
 *
\* ---------------------------------------------------------------------- */
static void read_colddata (ReadStatus_t &status,
                           ReadFrame::Data   *d,
                           AxisIn         &axis,
                           AxisWord_t     *hdrs,
                           uint16_t     &cvtCnt,
                           int              off)
{
   #pragma HLS INLINE off
   read_colddata_hdrs (status, d,     axis, hdrs, cvtCnt, off);
   read_colddata_data (status, d + 2, axis, off == 8);
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define READ_FRAME_PRINT 1
#endif
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#if WRITE_DATA_PRINT
/* ---------------------------------------------------------------------- */
static void print_read_frame (ReadFrame const &frame)
 {
   for (int i = 0; i < 30; i++)
   {
      std::cout << "Frame.d[" << std::setw(5) << i << "] = " << std::setw(16) << std::hex << frame.d[i] << std::endl;
   }
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define print_read_frame(_frame)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Read and buffer the Wib Frame data
 *
 *  \param[   out] frame  A copy of the WIB data + status
 *  \param[    in]  axis  The input WIB data
 *  \param[    in]  init  Is this an initialization request
 *  \param[   out]   gbl  The module's global read status. This is updated
 *                        with the local read status
 *  \param[in:out]   lcl  The module's local  read status.  This is copied
 *                        into the global read status after it has been
 *                        updated.
 *
 *  \par
 *   The data is always copied but is valid iff the run enable bit in
 *   the user field of the first word is set and the flush flag in
 *   the user field of the last word is not set
 *
\* ---------------------------------------------------------------------- */
static void read_frame (ReadFrame     &frame,
                        ReadContext     &ctx,
                        AxisIn         &axis)
{
   ReadStatus_t status = 0;

   // Wib Header Words 0 & 1
   read_wib0     (status, frame.d, axis, ctx.hdrs, ctx.first, ctx.wibId);
   read_wib1     (status, frame.d, axis, ctx.hdrs);

   // Colddata links 0 & 1
   read_colddata (status, frame.d +  2, axis, ctx.hdrs + 2, ctx.cvtCnt0, 0);
   read_colddata (status, frame.d + 16, axis, ctx.hdrs + 4, ctx.cvtCnt1, 8);


   frame.status = status;

   print_read_frame (frame);


   // Count the number of frames to be written
   // A good frame is one with SOF and EOF set only on the proper words and no Eof Err
   // Flush frames with nothing in the packet are not promoted.
   // Flush frames in active packets are.  It is out to the receiver to dispose of these.
   bool writeOutput = ReadFrame::isGoodFrame (status) &&
                    ! ((ctx.nframes == 0) && status.test (static_cast<unsigned>(ReadFrame::Flush)));

   if  (writeOutput)
   {
      ctx.nframes += 1;
      frame.last   = ctx.begPkt = ctx.nframes == 0;

      int nframes = ctx.nframes;
      printf ("Frames.last = %d (%d)\n", frame.last, nframes);
   }

   frame.write = writeOutput;

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Read and buffer the Wib Frame data
 *
 *  \param[   out] frame  A copy of the WIB data + status
 *  \param[    in]  axis  The input WIB data
 *  \param[    in]  init  Is this an initialization request
 *  \param[   out]   gbl  The module's global read status. This is updated
 *                        with the local read status
 *  \param[in:out]   lcl  The module's local  read status.  This is copied
 *                        into the global read status after it has been
 *                        updated.
 *
 *  \par
 *   The data is always copied but is valid iff the run enable bit in
 *   the user field of the first word is set and the flush flag in
 *   the user field of the last word is not set
 *
\* ---------------------------------------------------------------------- */
static void read_frame (ReadFrame     &frame,
                        ReadContext     &ctx,
                        AxisIn         &axis,
                        ModuleConfig    &cfg,
                        MonitorRead     &gbl,
                        MonitorRead     &lcl)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   ReadStatus_t status = 0;

   // Wib Header Words 0 & 1
   read_wib0     (status, frame.d, axis, ctx.hdrs, ctx.first, ctx.wibId);
   read_wib1     (status, frame.d, axis, ctx.hdrs);

   // Colddata links 0 & 1
   read_colddata (status, frame.d +  2, axis, ctx.hdrs + 2, ctx.cvtCnt0, 0);
   read_colddata (status, frame.d + 16, axis, ctx.hdrs + 4, ctx.cvtCnt1, 8);

   frame.status = status;

    // Count the number of frames to be written
    // A good frame is one with SOF and EOF set only on the proper words and no Eof Err
    bool writeOutput = ReadFrame::isGoodFrame (status);
    frame.write      = writeOutput;

    if  (writeOutput)
    {
       ctx.nframes += 1;
       frame.last   = ctx.nframes == 0
                    | ReadFrame::flushesPacket (status);

       // Don't write flushed frames
       if (status.test (static_cast<unsigned>(ReadFrame::Flush))) frame.write = false;
    }

    update_read (gbl,  lcl, cfg, status);


   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
/* Local methods used by the update read method                           */
/* ---------------------------------------------------------------------- */
static void update_frame    (MonitorRead    &lcl,
                             ReadStatus_t status);

static void update_wib      (MonitorRead    &lcl,
                             ReadStatus_t status);

static void update_colddata (MonitorRead    &lcl,
                             ReadStatus_t status,
                             int            sOff,
                             int            cOff);
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Update the read portion of the status block
 *
 *  \param[   out]    gbl This will receive a copy of the locally
 *                        maintained read status block after it has
 *                        been updated
 *  \param[in:out]    lcl The locally maintained copy of the read status
 *                        block
 * \param[in]         cfg The configuration block
 * \param[in]      output Flag indicating whether the packet was written
 * \param[in]      status The read status flags.  This is used to increment
 *                        a counter corresponding to this status.
 *
\* ---------------------------------------------------------------------- */
static void update_read (MonitorRead        &gbl,
                         MonitorRead        &lcl,
                         ModuleConfig const &cfg,
                         ReadStatus_t     status)
{
#if PROCESS_K_MODE == PROCESS_K_COMPRESS
   #pragma HLS INLINE  off
#else
    #pragma HLS INLINE off
#endif

   //std::cout << "status(flush): " << status << " user:" << in.user << std::endl;

   if (cfg.init)
   {
      lcl.init ();
   }
   else
   {
      // Always update the status mask and count the total number of frames seen
      lcl.mask     = status;
      lcl.nframes += 1;

      if (status.test (ReadFrame::ErrSofM))
      {
         // Count frames missing SOF
         lcl.count(MonitorRead::FrameCounter::ErrSofM);
      }
      else
      {
         // SOF   was set
         update_frame (lcl, status);

         // A good frame is one with SOF and EOF set only on the proper words and no Eof Err
         bool goodFrame = ReadFrame::isGoodFrame (status);
         if  (goodFrame)
         {
            // Only count if both SOF and EOF were properly seen
            lcl.count (ReadFrame::state (status));

            // Only increment the counters associated with checking the integrity
            // of the frame contents if this is not a FLUSH frame.
            if (!status.test (static_cast<unsigned>(ReadFrame::Offset::Flush)))
            {
               // Wib Header word
               update_wib (lcl, status);

               // ColdData link 0 and 1
               update_colddata (lcl,status, 0, 0);
               update_colddata (lcl,status, 8, 8);
            }
         }
      }
   }

   printReadResults (status, lcl);

   gbl = lcl;

   return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Update the counters that monitor the status of the reading
 *           of the frame. By contrast, these are counters that have nothing
 *           to do with the data contents, only the integrity of the data
 *           transfer.
 *
 *  \param[in:out]    mon  The monitor class to update
 *  \param[in]     status  The status mask
 *
\* ----------------------------------------------------------------------- */
static void update_frame (MonitorRead &mon, ReadStatus_t status)
{
   //#pragma HLS PIPELINE
   #pragma HLS INLINE

   // Unexpected SOF seen?
   mon.count (status.test (ReadFrame::  Offset::      ErrSofU),
                           MonitorRead::FrameCounter::ErrSofU);

   // Unexpected EOF seen?
   mon.count (status.test (ReadFrame::  Offset::      ErrEofU),
                           MonitorRead::FrameCounter::ErrEofU);

   // EOF missing on last word?
   mon.count (status.test (ReadFrame::  Offset::      ErrEofM),
                           MonitorRead::FrameCounter::ErrEofM);

   // EOF err set in the frame?
   mon.count (status.test (ReadFrame::  Offset::      ErrEofE),
                           MonitorRead::FrameCounter::ErrEofE);

   return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Update the counters that monitor the status of the reading
 *           of the cold data by the WIB
 *
 *  \param[in:out]    mon  The monitor class to update
 *  \param[in]     status  The status mask
 *
\* ----------------------------------------------------------------------- */
static void update_wib (MonitorRead &mon, ReadStatus_t status)
{
   //#pragma HLS PIPELINE
   #pragma HLS INLINE

   // Wib Header word 0
   mon.count (status.test (ReadFrame::  Offset::    ErrWibComma),
                           MonitorRead::WibCounter::ErrWibComma);

   mon.count (status.test (ReadFrame::  Offset::    ErrWibId),
                           MonitorRead::WibCounter::ErrWibId);

   mon.count (status.test (ReadFrame::  Offset::    ErrWibVersion),
                           MonitorRead::WibCounter::ErrWibVersion);

   mon.count (status.test (ReadFrame::  Offset::    ErrWibRsvd),
                           MonitorRead::WibCounter::ErrWibRsvd);

   mon.count (status.test (ReadFrame::  Offset::    ErrWibErrors),
                           MonitorRead::WibCounter::ErrWibErrors);


   // Wib Header word 1
   mon.count (status.test (ReadFrame::  Offset::    ErrWibTimestamp),
                           MonitorRead::WibCounter::ErrWibTimestamp);

   return;
}
/* ----------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Writes the frame's data to the output packet. The packet will
 *           be flushed if
 *              -# Full
 *              -# The run was disabled with output pending
 *              -# A flush was requested
 *
 *   \param[   out]       axis  The output AXI stream
 *   \param[in:out]        ctx  The output context
 *   \param[in    ]      frame  The frame's data + status
 *   \param[in:out]        gbl  The global write monitor
 *   \param[in:out]        lcl  The local  write nonitor
 *
\* ---------------------------------------------------------------------- */
static void write_frame (AxisOut           &axis,
                         WriteContext       &ctx,
                         ReadFrame        &frame,
                         MonitorWrite       &gbl,
                         MonitorWrite       &lcl)
{
   // -------------------------------------------------
   // OUTPUT HEADER
   // -------------
   // Add output header to the output frame iff this is
   //   1. the first output word of this packet ..and..
   //   2. the processFlag is true.
   // --------------------------------------------------
   if (ctx.odx == 0)
   {
       prologue (axis, ctx.odx, frame.write, 0);
   }


   // -------------------------------------------------------------------
   // OUTPUT BODY
   // -----------
   // While the data is not copied to the output packet is procesFlag is
   // false, the copy loop always runs to ensure that the data stream is
   // drained.
   // -------------------------------------------------------------------
   COPYFRAME_LOOP:
   for (int idx = 0; idx < sizeof (WibFrame)/sizeof (uint64_t); idx++)
   {
      #pragma HLS PIPELINE
      uint64_t          d;

      d = frame.d[idx];
      //d = frame.d.read ();
      commit (axis, ctx.odx, frame.write, d, 0, 0);
   }

   if (frame.write) lcl.npromoted += 1;
   else             lcl.ndropped  += 1;


   // --------------------------------------------
   // OUTPUT TRAILER
   // --------------
   // Add the trailer and flush packet if both
   //   1) Have some data to flush
   //   2) Instructed that this is the last packet
   // --------------------------------------------
   bool output;
   if (ctx.odx && frame.last)
   {
      epilogue (axis, frame.status, ctx.odx);

      lcl.nbytes   += ctx.odx * 8;
      lcl.npackets += 1;
      ctx.odx       = 0;
   }

   gbl = lcl;

   return;
}
/* ---------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Update the counters that monitor the status of the reading
 *           of the front end electronics
 *
 *  \param[in:out]    mon  The monitor class to update
 *  \param[in]     status  The status mask
 *
\* ----------------------------------------------------------------------- */
static void  update_colddata (MonitorRead    &mon,
                              ReadStatus_t status,
                              int            sOff,
                              int            cOff)
{
   //#pragma HLS PIPELINE
   #pragma HLS INLINE off

   // ------------------------------------------------------------
   // ColdData Header Word 0
   //
   mon.count (status.test (ReadFrame::  Offset::    ErrCd0StrErr1),
                           MonitorRead::WibCounter::ErrCd0StrErr1);

   mon.count (status.test (ReadFrame::  Offset::    ErrCd0StrErr2),
                           MonitorRead::WibCounter::ErrCd0StrErr2);

   mon.count (status.test (ReadFrame  ::Offset::    ErrCd0Rsvd0),
                           MonitorRead::WibCounter::ErrCd0Rsvd0);

   mon.count (status.test (ReadFrame::  Offset::    ErrCd0ChkSum),
                           MonitorRead::WibCounter::ErrCd0ChkSum);

   mon.count (status.test (ReadFrame::  Offset::    ErrCd0CvtCnt),
                           MonitorRead::WibCounter::ErrCd0CvtCnt);
   //
   // ------------------------------------------------------------


   // ------------------------------------------------------------
   // ColdData Header Word 1
   //
   mon.count (status.test (ReadFrame::  Offset::    ErrCd0ErrReg),
                           MonitorRead::WibCounter::ErrCd0ErrReg);

   mon.count (status.test (ReadFrame::  Offset::    ErrCd0Rsvd1),
                           MonitorRead::WibCounter::ErrCd0Rsvd1);

   mon.count (status.test (ReadFrame::  Offset::    ErrCd0Hdrs),
                           MonitorRead::WibCounter::ErrCd0Hdrs);
   //
   // ------------------------------------------------------------

   return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  If this is the first word, add the packet header
 *
 *  \param[   out]     mAxis  The output AXI stream
 *  \param[in:out]       odx  The 64-bit packet index
 *  \param[in    ] writeFlag  Write enable flags
 *  \param[in    ] moduleIdx  Which module, \e i.e. which group of
 *                            128 channels being service
 *
\* ----------------------------------------------------------------------- */
static void prologue (AxisOut &mAxis, int &odx, bool writeFlag, int moduleIdx)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   #define WIB_VERSION VERSION_COMPOSE (0, 0, 0, 1)

   if (odx == 0)
   {
      // --------------------------------------------------------------
      // Because the output packet maybe terminated at anytime, the
      // size of the packet cannot be known at this time. This field
      // in the record header will have to be filled in by the receiving
      // software.
      //
      // For integrity checking, the module index will be stuffed
      // into this unused field. Its kind of a kludge, but maybe useful.
      // --------------------------------------------------------------
      uint32_t  id = Identifier::identifier (Identifier::FrameType::DATA,
                                             Identifier::DataType::WIB,
                                             moduleIdx);

      uint64_t      hdr = Header::header (id, WIB_VERSION);
      print_header (hdr);
      commit       (mAxis, odx, writeFlag, hdr, 2, 0);
   }
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Does the processing after a full frame has been readin
 *
 *  \param[   out]     mAxis  The output AXI stream
 *  \param[    in]    status  The summeary status
 *  \param[in:out]       odx  The 64-bit packet index
 *
\* ----------------------------------------------------------------------- */
static void epilogue (AxisOut    &mAxis,
                      uint64_t   status,
                      int          &odx)
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   bool wrote;


   std::cout << "Outputting @ odx = " << odx << std::endl;

   int   nbytes = odx * sizeof (uint64_t) + sizeof (Trailer);
   uint32_t  id = Identifier::identifier (Identifier::FrameType::DATA,
                                          Identifier::DataType::WIB,
                                          nbytes);
   uint64_t tlr = Trailer::trailer (id);
   print_trailer (tlr);

   commit (mAxis, odx, true, status, 0, 0);
   commit (mAxis, odx, true, tlr,    0, 1);

   return;
}
/* ----------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Print the status results of the readFrame method
 *
 *  \param[in]  status  The 2-bit readFrame status word
 *  \param[in] monitor  The read monitor record
\* ---------------------------------------------------------------------- */
static void printReadResults (ap_uint<2> status, MonitorRead const &monitor)
{
   // If the readStatus was not successsful
   if (status)
   {
      std::cout << "ReadStatus: " << status << std::endl
                << "Types:      " << monitor[MonitorRead::StateCounter::Normal]
                <<            ' ' << monitor[MonitorRead::StateCounter::RunDisabled]
                <<            ' ' << monitor[MonitorRead::StateCounter::Flush]
                <<            ' ' << monitor[MonitorRead::StateCounter::DisFlush]
                <<       "Err SofM     " << monitor[MonitorRead::FrameCounter::ErrSofM]
                <<       "Err SofU     " << monitor[MonitorRead::FrameCounter::ErrSofU]
                <<       "Err EofM     " << monitor[MonitorRead::FrameCounter::ErrEofM]
                <<       "Err EofU     " << monitor[MonitorRead::FrameCounter::ErrEofU]
                <<       "Err EofE     " << monitor[MonitorRead::FrameCounter::ErrEofE]
                <<   "Err_WibComma     " << monitor[MonitorRead::WibCounter  ::ErrWibComma]
                <<   "Err WibTimestamp " << monitor[MonitorRead::WibCounter  ::ErrWibTimestamp]
                << std::endl;
  }

  return;
}
