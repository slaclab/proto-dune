// -*-Mode: C++;-*-


#ifndef _DUNE_DATA_COMPRESSION_TYPES_H_
#define _DUNE_DATA_COMPRESSION_TYPES_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionTypes.h
 *  @brief    Interface file for the Dune data compression
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
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  2015/12/02>
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
   2018.03.06 jjr Changed the symbolic definitions of the Axis control
                  bits from masks to bit numbers.
   2016.05.03 jjr Added output limiting register to the configuration
   2015.11.28 jjr Added error counters for Start of Frame, End of Frame on
                  other than the expected words, i.e. first word for SOF
                  and last for EOF

   2015.11.18 jjr Added error counters for no Start of Frame, End of Frame
                  and K28_5 character not seen to the read status block

                  Redefined the bits in user first and user last so that aren't
                  overloaded. Bits 0 & 2 are now only set in user last and
                  bits 1 & 3 are not only set in user first.  This will
                  allow us to resynchronize the frames if need be.

                  To accomodate this the user field had to be expanded to
                  4 bits

   2016.10.22 jjr Change Axi streams to hls::streams.  Change data types
                  to unsigned 64-bit data.
   2016.10.20 jjr Change ModuleConfig init and mode to 32-bit integers
                  just to make it easier to manipulation from the
                  processor.
   2016.10.12 jjr Added Common Status to the status block.  This will
                  count statistics common to all channels
   2016.01.05 jjr Added the status word to the raw frame. Previously this
                  was absent, meaning that this simulation did not match
                  the frames as delivered by the FPGA code.
   2015.12.02 jjr Made public

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


#include <stdint.h>

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "WibFrame.h"


/* ---------------------------------------------------------------------- *//*!
 *
 * \def    MODULE_B_NCHANNELS
 * \brief  The number of channels in a module expressed as a bit count.
 *         This is typically 5, giving 32 channels in a module, but for
 *         testing purposes may be can any number resulting in a multiple
 *         of 4 channels.
 *
 * \def    MODULE_K_NCHANNELS
 * \brief  The number of channels in a module as a integer count. This is
 *         typically 32, but, for testing can be any multiple 4. This
 *         must be consistent with MODULE_B_NCHANNELS, but is expressed
 *         literally for readability.
 *
 * \def    RCE_B_NCHANNELS
 * \brief  The number of channels serviced by an RCE expressed as a bit
 *         count.
 * \par
 *         Since the number of channels serviced by an RCE is only constrained
 *         to be a multiple of 32, this value must berounded up.  For example
 *         if an RCE service 384 channels, then this value must be set to 9.
 *
 * \def    RCE_K_NCHANNELS
 * \brief  The total number of channels processed in one RCE. This value
 *         is typically a multiple of 32.
 *
 * \def    RCE_B_NMODULES
 * \brief  The number of modules serviced by a module expressed as a bit count.
 *
 * \note
 *         If the number of modules is not a power of 2, then this value must
 *         be rounded up.
 *
 * \def    RCE_K_NMODULES
 * \brief  The number of modules in one RCE. This is formally
 *         RCE_K_NCHANNELS / MODULE_K_NCHANNELS. With 256 channels and
 *         32 channels per module, this gives 8 modules.
 *
\* ---------------------------------------------------------------------- */
#define MODULE_B_NCHANNELS             7
#define MODULE_K_NCHANNELS           128//// !!!KLUDGE 32

#define RCE_B_NCHANNELS                8
#define RCE_K_NCHANNELS              256

#define RCE_B_NMODULES              (RCE_B_NCHANNELS - MODULE_B_NCHANNELS)
#define RCE_K_NMODULES              (RCE_K_NCHANNELS / MODULE_K_NCHANNELS)
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \def    PACKET_B_NSAMPLES
 *  \brief  The number of ADCs per packet, expressed in bits
 *
 *  \def    PACKET_K_NSAMPLES
 *  \brief  The number of ADCs per packet
 *
 *  \def   PACKET_M_NSAMPLES
 *  \brief The number of ADCs per packet, expressed as a bit mask
 *
\* ---------------------------------------------------------------------- */
#define PACKET_B_NSAMPLES   10
#define PACKET_K_NSAMPLES 1024
#define PACKET_M_NSAMPLES (PACKET_K_NSAMPLES - 1)
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \def    MODULE_K_MAXSIZE_IB
 *  \brief  Maximum size of an inbound frame
 *
 *  \def    MODULE_K_MAXSIZE_OB
 *  \brief  Maximum size of an outbound packet
 *
\* ---------------------------------------------------------------------- */
#define MODULE_K_MAXSIZE_IB  (sizeof (WibFrame) / sizeof (uint64_t))
#define MODULE_K_MAXSIZE_OB  (MODULE_K_NCHANNELS * PACKET_K_NSAMPLES/4 + 100)
/* ---------------------------------------------------------------------- */



// --------------------------------------------------------------------
// Configure the AXIS bus to be the same as 
// RCEG3_AXIS_DMA_CONFIG_C (RceG3Pkg.vhd)
//   constant RCEG3_AXIS_DMA_CONFIG_C : AxiStreamConfigType := (
//      TSTRB_EN_C    => false,
//      TDATA_BYTES_C => 8,
//      TDEST_BITS_C  => 4,
//      TID_BITS_C    => 0,
//      TKEEP_MODE_C  => TKEEP_COMP_C,
//      TUSER_BITS_C  => 4,
//      TUSER_MODE_C  => TUSER_FIRST_LAST_C);
// --------------------------------------------------------------------


///////////////////////////////////////////
//       AXIS_STREAM Definition          //
///////////////////////////////////////////
typedef     ap_axiu<64,4,1,1>  AxisIn_t;
typedef     ap_axiu<64,4,1,1>  AxisOut_t;
typedef hls::stream<AxisIn_t>  AxisIn;
typedef hls::stream<AxisOut_t> AxisOut;
typedef     ap_uint<64>        AxisWord_t;
// tdata[063:00] = DMA Data
// tKeep[007:00] = DMA Byte Enable
// tStrb[007:00] = Not Used
// tUser[003:03] = Run enable                (first)
// tUser[002:02] = Flush                     (tlast)
// tUser[001:01] = Start of Frame            (first)
// tUser[000:00] = End of Frame with Errors  (tlast)
// tLast[000:00] = End of Frame
//   tId[000:00] = Not Used
// tDest[000:00] = Not Used
// Note:
///////////////////////////////////////////
//   template<int D, int U, int TI, int TD>
//   struct ap_axis{
//      ap_int<D>    data;
//      ap_uint<D/8> keep;
//      ap_uint<D/8> strb;
//      ap_uint<U>   user;
//      ap_uint<1>   last;
//      ap_uint<TI>  id;
//      ap_uint<TD>  dest;
//   };

/* ---------------------------------------------------------------------- *//*!
 *
 * \enum   class AxisUserFirst
 * \brief  Enumeates the meaning of the bits in AxisIn user field when
 *         it is in the first word of the frame.
 *
\* ---------------------------------------------------------------------- */
enum class AxisUserFirst
{
   Sof       = 1,  /*!< Always set on the first word of the frame         */
   RunEnable = 3   /*!< Run enable, effect means keep the data            */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \enum  class AxisUserLast
 * \brief Enumerates the meaning of the bits in AxisIn user field when
 *        it is in the last word the frame.
\* ---------------------------------------------------------------------- */
enum class AxisUserLast
{
   EofErr   = 0,  /*!< End of frame with errors                           */
   Flush    = 2   /*!< Flush the output frame                             */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \enum  class AxisLast
 * \brief Enumerates the meaing of the ibits in Axis last field.
 *
\* ---------------------------------------------------------------------- */
enum class AxisLast
{
   Eof     = 0 /*!< End of frame word                                     */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef ModuleIdx_t
 *  \brief   An integer sized to contain the module indices.
 *
 *  \par
 *   The incoming channels are processed in groups of channels called
 *   modules. The number of modules on an RCE is RCE_K_NMODULES, with
 *   RCE_B_NMODULES giving the number of bits in an integer needed
 *   to specify a module index.
 *
 *  \note
 *   Must guard against having only 1 module. In this case RCE_B_NMODULES
 *   will be 0, and, without the check, an ap_uint<0> will try to be
 *   instantiated; clearly a non-sensical idea.
 *
\* ---------------------------------------------------------------------- */
#if RCE_B_NMODULES > 0
typedef ap_uint<RCE_B_NMODULES> ModuleIdx_t;
#else
typedef ap_uint<1> ModuleIdx_t;
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef WibTimestamp_t
 *  \brief   The timestamp information in the WIB frame
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint<64> WibTimestamp_t;
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef WibCrate_t
 *  \brief   The WIB crate number
 *
 *  \typedef WibSlot_t
 *  \brief   The WIB slot number
 *
 *  \typedef WibFiber_t
 *  \brief   The WIB fiber number
 *
 *  \typedef WibId_t
 *  \brief   The WIB fiber slot and crate #
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint< 5> WibCrate_t;
typedef ap_uint< 3> WibSlot_t;
typedef ap_uint< 3> WibFiber_t;
typedef ap_uint<13> WibId_t;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct ChannelConfig
 *  \brief  The per channel configuration information
 *
\* ---------------------------------------------------------------------- */
struct ChannelConfig {
   bool disabled;        /*!< Is this channel disabled                    */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *   \enum  Mode_t
 *   \brief Enumeration of the data processing modes
 *
\* ---------------------------------------------------------------------- */
enum MODE_K
{
   MODE_K_DUMP       = 0,  /*!< Read and dispose of the data              */
   MODE_K_COPY       = 1,  /*!< Read and promote as is                    */
   MODE_K_TRANSPOSE  = 2,  /*!< Transpose channel and time order          */
   MODE_K_COMPRESS   = 3,  /*!< Compress the data                         */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef Mode_t
 *  \brief   The data processing mode
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint<2> Mode_t;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct ConfigModule
 *  \brief  The per module configuation information
 *
\* ---------------------------------------------------------------------- */
struct ModuleConfig {
   uint32_t                            init;  /*!< Initialization flag    */
   uint32_t                            mode;  /*!< Data processing mode   */
   uint32_t                           limit;  /*!< Limits size of output
                                                   packet, units = 64bit
                                                   words                  */
   ChannelConfig    chns[MODULE_K_NCHANNELS]; /*!< Per channel config     */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorCommon
 *  \brief Track status and statistics common to the module
 *
\* ---------------------------------------------------------------------- */
struct MonitorCommon
{
   uint32_t   pattern; /*!< Recognizable bit pattern                      */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorCfg
 *  \brief Track status and statistics configuration requests
 *
\* ---------------------------------------------------------------------- */
struct MonitorCfg
{
   uint32_t      mode; /*!< Last configured acquisition mode              */
   uint32_t     ncfgs; /*!< Number of configuration requests              */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorRead
 *  \brief Track status and statistics for read method
 *
\* ---------------------------------------------------------------------- */
struct MonitorRead
{
   explicit MonitorRead ()
   {
//      #pragma    HLS ARRAY_PARTITION variable=ntypes factor=4 dim=1
   }

   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Counter
    *  \brief The index of the counters of ntypes
    *
    */
   enum class WibCounter
   {
      // Errors from the WIB header words
      ErrWibComma     = 0x00, /*!< Comma character does not match         */
      ErrWibVersion   = 0x01, /*!< Version number incorrect               */
      ErrWibId        = 0x02, /*!< WibId (Crate.Slot.Fiber) incorrect     */
      ErrWibRsvd      = 0x03, /*!< Reserved header bits are not 0         */
      ErrWibErrors    = 0x04, /*!< Wib Errors field is not 0              */
      ErrWibTimestamp = 0x05, /*!< Unused bit                             */
      ErrWibUnused6   = 0x06, /*!< Unused bit                             */
      ErrWibUnused7   = 0x07, /*!< Unused bit                             */

      // Errors from the Colddata stream 0 header words
      ErrCd0StrErr1   = 0x08,  /*!< Colddata link 0, stream err1 is not 0   */
      ErrCd0StrErr2   = 0x09,  /*!< Colddata link 0, stream err2 is not 0   */
      ErrCd0Rsvd0     = 0x0a,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd0ChkSum    = 0x0b,  /*!< Colddata link 0, checksum incorrect     */
      ErrCd0CvtCnt    = 0x0c,  /*!< Colddata link 0, convert count mismatch */
      ErrCd0ErrReg    = 0x0d,  /*!< Colddata link 0, error register is not 0*/
      ErrCd0Rsvd1     = 0x0e,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd0Hdrs      = 0x0f,  /*!< Colddata link 0, error in hdr words     */

       // Errors from the Colddata stream 1 header words
      ErrCd1StrErr1   = 0x10,  /*!< Colddata link 0, stream err1 is not 0   */
      ErrCd1StrErr2   = 0x11,  /*!< Colddata link 0, stream err2 is not 0   */
      ErrCd1Rsvd0     = 0x12,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd1ChkSum    = 0x13,  /*!< Colddata link 0, checksum incorrect     */
      ErrCd1CvtCnt    = 0x14,  /*!< Colddata link 0, convert count mismatch */
      ErrCd1ErrReg    = 0x15,  /*!< Colddata link 0, error register is not 0*/
      ErrCd1Rsvd1     = 0x16,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd1Hdrs      = 0x17,   /*!< Colddata link 0, error in hdr words    */
   };
   typedef ap_uint<5> WibCounterIdx;


   enum class StateCounter
   {
      // These bits are not promoted to the host
      Normal          =  0x0, /*!< Frames with Flush = 0, RunDisable = 0    */
      RunDisabled     =  0x1, /*!, Frames with Flush = 0, RunDisable = 1    */
      Flush           =  0x2, /*!< Frames with Flush = 1, RunDisable = 0    */
      DisFlush        =  0x3, /*!< Frames with Flush = 1, RunDisable = 1    */
   };
   typedef ap_uint<2> StateCounterIdx;

   enum class FrameCounter
   {
      ErrSofM         =  0x0, /*!< Frames missing Sof marker on first user word */
      ErrSofU         =  0x1, /*!< Frames with Sof marker on other than first   */
      ErrEofM         =  0x2, /*!< Frames missing Eof marker on last word       */
      ErrEofU         =  0x3, /*!< Frames with Eof marker on other than last    */
      ErrEofE         =  0x4, /*!< Frames with Eof marker with error            */
   };
   typedef ap_uint<3> FrameCounterIdx;

   /* ---------------------------------------------------------------------- *//*!<
    *
    *  brief  Conditionally increments the specified Wib Error counter
    *
    *  \param[in] increament If true, then increment the counter
    *  \param[in]    counter Which counter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   uint32_t count (bool increment, WibCounter counter)
   {
      #pragma HLS INLINE
      if (increment)
      {
         return nWibErrs[static_cast<unsigned>(counter)] += 1;
      }
      else
      {
         return nWibErrs[static_cast<unsigned>(counter)];
      }
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!<
    *
    *  brief  Unconditionally increments the specified Wib Error counter
    *
    *  \param[in] counter Which counter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
  void count (WibCounter counter)
   {
      #pragma HLS INLINE
       nWibErrs[static_cast<unsigned>(counter)] += 1;
   }
   /* ---------------------------------------------------------------------- */



  /* ---------------------------------------------------------------------- *//*!<
   *
   *  brief  Conditionally increments the specified Frame Error counter
   *
   *  \param[in] increament If true, then increment the counter
   *  \param[in]        idx The index of the counter to increment
   *                                                                        */
  /* ---------------------------------------------------------------------- */
  uint32_t count (bool increment, FrameCounter counter)
  {
     #pragma HLS INLINE
     if (increment)
     {
        return nWibErrs[static_cast<unsigned>(counter)] += 1;
     }
     else
     {
        return nWibErrs[static_cast<unsigned>(counter)];
     }
  }
  /* ---------------------------------------------------------------------- */


  /* ---------------------------------------------------------------------- *//*!<
   *
   *  brief  Unconditionally increments the specified Frame Error counter
   *
   *  \param[in]  counter Which ounter to increment
   *                                                                        */
  /* ---------------------------------------------------------------------- */
 void count (FrameCounter counter)
  {
     #pragma HLS INLINE
      nWibErrs[static_cast<unsigned>(counter)] += 1;
  }
  /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!<
     *
     *  brief  Unconditionally increments the frame state counters
     *
     *  \param[in]  state  The index of the state counter to increment
     *
     *  There are 4 possible frame states
     *     -0  Normal,               run  enabled, frame flush disabled
     *     -1  Run disabled,         run disabled, frame flush disabled
     *     -2  Flush,                run  enabled, frame flush enabled
     *     -3  Run Disabled & Flush, run  disable, frame flush enabled
     *
     *                                                                        */
    /* ---------------------------------------------------------------------- */
   void count (StateCounterIdx state)
   {
      #pragma HLS INLINE
      nStates[state] += 1;
   }

   unsigned operator[] (StateCounter counter) const
   {
      return nStates[static_cast<unsigned>(counter)];
   }

   unsigned operator[] (FrameCounter counter) const
   {
      return nFrameErrs[static_cast<unsigned>(counter)];
   }

   unsigned operator[] (WibCounter counter) const
   {
      return nWibErrs[static_cast<unsigned>(counter)];
   }

   void init ()
   {
      #pragma HLS INLINE

      // Don't processs frames during a configuration request
      WIBERRS_CLEAR_LOOP:
      for (int idx = 0; idx < sizeof (nWibErrs) / sizeof (nWibErrs[0]); ++idx)
      {
         nWibErrs[idx] = 0;
      }

      for (int idx = 0; idx < sizeof (nStates) / sizeof (nStates[0]); ++idx)
      {
         nStates[idx] = 0;
      }

      for (int idx = 0; idx < sizeof (nFrameErrs) / sizeof (nFrameErrs[0]); ++idx)
      {
         nFrameErrs[idx] = 0;
      }

      nframes   = 0;
      mask      = 0;
   }

   typedef ap_uint<32> Counter;
   ap_uint<32>          mask; /* < Status mask                            */
   uint32_t          nframes; /*!< Total number frames, this must be the
                                   sum of the first 4                     */
   Counter      nWibErrs[24]; /*!< Count of WIB errors                    */
   Counter        nStates[4]; /*!< Count of Frame states (Run/Enabled     */
   Counter     nFrameErrs[5]; /*!< Count of framing errors (Sof/Eof etc)  */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorWrite
 *  \brief Track status and statistics for write method
 *
\* ---------------------------------------------------------------------- */
struct MonitorWrite
{
   uint32_t    nbytes; /*!< Number of bytes written                       */
   uint32_t npromoted; /*!< Number WIB frame promoted to the output packet*/
   uint32_t  ndropped; /*!< Number WIB frames dropped                     */
   uint32_t  npackets; /*!< Number of packets written                     */
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorModule
 *  \brief  The per module monitoring information
 *
\* ---------------------------------------------------------------------- */
struct MonitorModule
{
   MonitorCommon  common;   /*!< Status common to the module              */
   MonitorCfg        cfg;   /*!< Configuration request status             */
   MonitorRead      read;   /*!< Read frame status                        */
   MonitorWrite    write;   /*!< Write packet status                      */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \namespace Frame
 *  \brief     Defines the layout and construction of the framing word
 *
 *  \par
 *   This layout is generic and fixed.  All versions of the header and
 *   trailer will use this layout, although certainly with different
 *   version words (by definition) and potentially different pattern
 *   words.
 *
 *   This done as a namespace so that new versions can be added to it.
 *   This takes advantage of the fact the namespaces are open, while
 *   classes are not.
 *
\* ---------------------------------------------------------------------- */
namespace Frame
{
   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Size
    *  \brief Defines the sizes of bit fields of the framing word.
    *
   \* ------------------------------------------------------------------- */
   enum class Size
   {
      VERSION =  4, /*!< Size, in bits, of the version field              */
      PATTERN = 28, /*!< Size, in bits, of the pattern field              */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Offset
    *  \brief Defines the right justified bit offset for the bit fields
    *         of the framing word.
    *
   \* ------------------------------------------------------------------- */
   enum class Offset
   {
      VERSION =  0, /*!< Right justified offset of the version  field     */
      PATTERN =  4  /*!< Right justified offset of the pattern field      */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Mask
    *  \brief Defines the right justified masks for the bit fields of
    *         the framing word.
    *
   \* ------------------------------------------------------------------- */
   enum class Mask
   {
      VERSION = 0xf,       /*!< Right justified mask of the version field */
      PATTERN = 0xfffffff, /*!< Right justified mask of the pattern field */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief  Constructs a framing word from the frame's version number
    *          and pattern word.
    *  \return The constructed framing word
    *
    *  \param[in] version The frame's format version number
    *  \param[in] pattern The frame's pattern word.
    *
   \* ------------------------------------------------------------------- */
   static inline uint32_t framer (uint32_t version, uint32_t pattern)
   {
      uint32_t w = ((pattern & static_cast<uint32_t>(Mask  ::PATTERN))
                            << static_cast<uint32_t>(Offset::PATTERN))
                 | ((version & static_cast<uint32_t>(Mask  ::VERSION))
                            << static_cast<uint32_t>(Offset::VERSION));
      return w;
   }
   /* ------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
} /* END: namespace FRAME                                                 */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \namespace Frame::V0
 *  \brief     Defines the layout and construction of V0 frame header
 *             and trailer.
 *
\* ---------------------------------------------------------------------- */
namespace Frame { namespace V0
{
   /* ------------------------------------------------------------------- *//*!
    *
    *  \var   Version
    *  \brief Defines this as version 0
    *
    *  \var   Pattern
    *  \brief Defines the pattern word for the version 0 frame
    *
    *  \var   Framer
    *  \brief Defines the framing word for the version 0 frame
    *
   \* ------------------------------------------------------------------- */
   const uint32_t Version =         0;
   const uint32_t Pattern = 0x8b309e2;
   const uint32_t Framer  =
                ((Pattern & static_cast<uint32_t>(Frame::Mask  ::PATTERN))
                         << static_cast<uint32_t>(Frame::Offset::PATTERN))

              | ((Version & static_cast<uint32_t>(Frame::Mask  ::VERSION))
                         << static_cast<uint32_t>(Frame::Offset::VERSION));
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class FrameType
    *  \brief Defines the various types of frames
    *
   \* ------------------------------------------------------------------- */
   enum class FrameType
   {
      DISCARD = 0,  /*!< Discard frame                                    */
      DATA    = 1   /*!< Data frame                                       */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Size
    *  \brief Defines the size, in bits, for the bit fields of the
    *         identifier.
    *
   \* ------------------------------------------------------------------- */
   enum class Size
   {
      NBYTES     = 24,  /*!< Size, in bits, of nbytes field               */
      FRAMETYPE  =  4,  /*!< Size, in bits, of frame type field           */
      RECORDTYPE =  4,  /*!< Size, in bits, of record type field          */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Offset
    *  \brief Defines the right justified bit offset for the bit fields
    *         of the identifier.
    *
   \* ------------------------------------------------------------------- */
   enum class Offset
   {
      NBYTES     =  0, /*!< Right justified offset of      nbytes field   */
      FRAMETYPE  = 24, /*!< Right justified offset of  frame type field   */
      RECORDTYPE = 28, /*!< Right justified offset of record type field   */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Mask
    *  \brief Defines the right justified masks for the bit fields of
    *         the identifier.
    *
   \* ------------------------------------------------------------------- */
   enum class Mask
   {
      NBYTES     = 0xffffff,/*!< Right justified mask, record type field  */
      FRAMETYPE  = 0xf,     /*!< Right justified mask,  frame type field  */
      RECORDTYPE = 0xf,     /*!< Right justified mask,  data type field   */

   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- */
   /* Generic, header, trailer and identifier construction routines       */
   /* ------------------------------------------------------------------- */
   template<typename RecordType>
   static uint32_t identifier (FrameType   frameType,
                               RecordType recordType,
                               uint32_t       nbytes);

   template<typename RecordType>
   static uint32_t identifier (FrameType    frameType,
                               RecordType  recordType);

   static uint64_t header     (uint32_t identifier);
   static uint64_t trailer    (uint32_t identifier);
   static uint64_t trailer    (uint32_t identifier, uint32_t nbytes);
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class DataType
    *  \brief Enumerates the valid record types when the frame type = DATA
   \* ------------------------------------------------------------------- */
   enum class DataType
   {
      RESERVED   = 0,  /*!< Unused, reserved                              */
      WIB        = 1,  /*!< Asis WIB format,unprocessed                   */
      TRANSPOSED = 2,  /*!< Transposed channel <-> time order             */
      COMPRESSED = 3,  /*!< Compressed                                    */
   };
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- */
   /* FrameType = DATA construction routines                              */
   /* ------------------------------------------------------------------- */
   static uint32_t identifier (DataType dataType);
   static uint32_t identifier (DataType dataType, uint32_t nbytes);
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for the specified frame and
    *         record type.
    *
    *  \param[in]  frameType The frame type
    *  \param[in] recordType The record type
    *  \param[in[     nbytes The number of bytes in this frame
    *
    *  \par
    *   This can only be used when the number of bytes is known. For
    *   example, When the frame is serially filled with a variable number
    *   of bytes, the number of bytes is generally not known until the last
    *   data has been written.  In that case, one should use the call
    *   that omits this parameter.
    *
    *  \warning
    *   It is up to the caller to ensure that the \a recordType is drawn
    *   from the set associated with the specified \a frameType. In
    *   general,these routines should not be called directly by the user,
    *   but by used as the basis of routines that onstruction frame type
    *   specific identifiers. Such routine can ensure that the \a recordType
    *   is consistent with the \a frameType
    *
   \* ------------------------------------------------------------------- */
   template<typename RecordType>
   static inline uint32_t identifier (FrameType   frameType,
                                      RecordType recordType,
                                      uint32_t       nbytes)
   {
      uint32_t id = ((uint32_t)( frameType) << (int)Offset::FRAMETYPE)
                  | ((uint32_t)(recordType) << (int)Offset::RECORDTYPE)
                  | ((uint32_t)(    nbytes) << (int)Offset::NBYTES);
      return id;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for the specified frame and
    *         record type.
    *
    *  \param[in]  frameType The frame type
    *  \param[in] recordType The record type
    *
    *  \warning
    *   It is up to the caller to ensure that the \a recordType is drawn
    *   from the set associated with the specified \a frameType. In
    *   general,these routines should not be called directly by the user,
    *   but by used as the basis of routines that onstruction frame type
    *   specific identifiers. Such routine can ensure that the \a recordType
    *   is consistent with the \a frameType
    *
   \* ------------------------------------------------------------------- */
   template<typename RecordType>
   static inline uint32_t identifier (FrameType    frameType,
                                      RecordType  recordType)
   {
      uint32_t id = ((uint32_t)( frameType) << (int)Offset::FRAMETYPE)
                  | ((uint32_t)(recordType) << (int)Offset::RECORDTYPE)
                  | ((uint32_t)(         0) << (int)Offset::NBYTES);
      return id;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief  Constructs a verion = 0 frame header
    *  \return The V0 frame header
    *
    *  \param[in] identifier  The previously construct V0 identifier
    *
   \* ------------------------------------------------------------------- */
   static inline uint64_t header  (uint32_t identifier)
   {
      uint64_t hdr = ((uint64_t)identifier << 32) | Framer;
      return   hdr;
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief  Constructs a version = 0 frame trailer
    *  \return The V0 frame trailer
    *
    *  \param[in] identifier  The previously construct V0 identifier.
    *
    *  \warning
    *   It is the user's responsibility to ensure that the \a identifier
    *   is consistent with that used to construct the header.
    *
   \* ------------------------------------------------------------------- */
   static inline uint64_t trailer (uint32_t identifier)
   {
      uint64_t tlr = ~(((uint64_t)Framer << 32) | identifier);
      return   tlr;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
     *
     *  \brief  Constructs a version = 0 frame trailer with the length
     *          field.
     *  \return The V0 frame trailer
     *
     *  \param[in] identifier  The previously construct V0 identifier.
     *  \param[in]     nbytes  The length, in bytes, of the framee
     *
     *  \warning
     *   It is the user's responsibility to ensure that the \a identifier
     *   is consistent with that used to construct the header and that
     *   the NBYTES field is clear.
     *
    \* ------------------------------------------------------------------- */
   static inline uint64_t trailer (uint32_t identifier, uint32_t nbytes)
   {
      uint64_t tlr =  trailer (identifier | (nbytes << (int)Offset::NBYTES));
      return   tlr;
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for a frame type of DATA
    *
    *  \param[in] dataType The type of data
    *
    *  \par
    *   This call should only be used when the number of bytes is not
    *   known when constructing the identifier.
    *
   \* ------------------------------------------------------------------- */
   static inline uint32_t identifier (DataType dataType)
   {
      uint32_t id = identifier<DataType> (FrameType::DATA, dataType);
      return id;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for a frame type of DATA
    *
    *  \param[in] dataType The type of data
    *  \param[in[   nbytes The number of bytes in this frame
    *
    *  \par
    *   This can only be used when the number of bytes is known. For
    *   example, When the frame is serially filled with a variable number
    *   of bytes, the number of bytes is generally not known until the last
    *   data has been written.  In that case, one should use the call
    *   that omits this parameter.
    *   not
    *
   \* ------------------------------------------------------------------- */
   static uint32_t identifier (DataType dataType, uint32_t nbytes)
   {
      uint32_t id = identifier<DataType> (FrameType::DATA, dataType, nbytes);
      return id;
   }
   /* ------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
}}  /* END: namespace Frame::V0                                           */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 * \class   Identifier
 *  \brief  Identifies the contents of the record
 *
\* ---------------------------------------------------------------------- */
class Identifier
{
public:
   enum class FrameType
   {
      DISCARD = 0,  /*!< Discard frame                                   */
      DATA    = 1   /*!< Data frame                                      */
   };

public:
   Identifier () { return; }

   enum class DataType
   {
      RESERVED   = 0,  /*!< Unused, reserved                              */
      WIB        = 1,  /*!< Asis WIB format,unprocessed                   */
      TRANSPOSED = 2,  /*!< Transposed channel <-> time order             */
      COMPRESSED = 3,  /*!< Compressed                                    */
   };

   enum class Size
   {
      NBYTES     = 24,  /*!< Size, in bits, of nbytes field               */
      RECORDTYPE =  4,  /*!< Size, in bits, of record type field          */
      FRAMETYPE  =  4   /*!< Size, in bits, of frame type field           */
   };

   enum class Offset
   {
      NBYTES     =  0, /*!< Right justified offset of      nbytes field   */
      RECORDTYPE = 24, /*!< Right justified offset of   data type field   */
      FRAMETYPE  = 28  /*!< Right justified offset of record type field   */
   };

   enum class Mask
   {
      NBYTES     = 0xf,     /*!< Right justified mask, nbytes field       */
      RECORDTYPE = 0xf,     /*!< Right justified mask, data type field    */
      FRAMETYPE  = 0xffffff,/*!< Right justified mask, record type field  */
   };

   static uint32_t identifier (FrameType   frameType,
                               DataType     dataType,
                               uint32_t       nbytes);

   static uint32_t identifier (FrameType   frameType,
                               DataType     dataType);
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Completely generic initializer
 *
 *  \param[in] frameType  The frame type
 *  \param[in] recordType The record type.  This must be consistent
 *                        with the types allowed by the \a frameType,
 *  \param[in] nbytes     The number of bytes in this frame.
 *
\* ---------------------------------------------------------------------- */
inline uint32_t Identifier::identifier (Identifier::FrameType frameType,
                                        DataType               dataType,
                                        uint32_t                 nbytes)
{
   uint32_t id = ((uint32_t)(frameType) << (int)Identifier::Offset::FRAMETYPE)
               | ((uint32_t)( dataType) << (int)Identifier::Offset::RECORDTYPE)
               | ((uint32_t)(   nbytes) << (int)Identifier::Offset::NBYTES);
   return id;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Completely generic initializer, no length
 *
 *  \param[in] frameType  The frame type
 *  \param[in] recordType The record type.  This must be consistent
 *                        with the types allowed by the \a frameType,
 *
\* ---------------------------------------------------------------------- */
inline uint32_t Identifier::identifier (Identifier::FrameType frameType,
                                        DataType               dataType)
{
   uint32_t id = ((uint32_t)(frameType) << (int)Identifier::Offset::FRAMETYPE)
               | ((uint32_t)( dataType) << (int)Identifier::Offset::RECORDTYPE);
   return id;
}
/* ---------------------------------------------------------------------- */
class Header
{
private:
   Header (uint32_t identifier, uint32_t version);

public:
   class Version
   {
   public:
   /* ----------------------------------------------------------------------- *//*!
    *
    *  \enum  WF_VERSION_S
    *  \brief Bit size of the version number fields
    *
   \* ----------------------------------------------------------------------- */
   enum class Size
   {
      RELEASE = 8,  /*!< Size of the version release field, ==0 for production,
                         other values indicate a development version         */
      PATCH   = 8,  /*!< Size of the version   patch field                   */
      MINOR   = 8,  /*!< Size of the version   minor field                   */
      MAJOR   = 8,  /*!< Size of the version   major field                   */
   };
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
    *
    *   \enum  Offset
    *   \brief Bit offsets of the version number fields
    *
   \* ---------------------------------------------------------------------- */
   enum class Offset
   {
      RELEASE =  0, /*!< Bit offset of the version release field             */
      PATCH   =  8, /*!< Bit offset of the version   patch field             */
      MINOR   = 16, /*!< Bit offset of the version   minor field             */
      MAJOR   = 24, /*!< Bit offset of the version   major field             */
   };
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
    *
    *   \enum  Mask
    *   \brief Right justified bit mask of the version number fields
    *
   \* ---------------------------------------------------------------------- */
   enum class Mask
   {
      RELEASE = 0xff, /*!< Bit offset of the version release field           */
      PATCH   = 0xff, /*!< Bit offset of the version   patch field           */
      MINOR   = 0xff, /*!< Bit offset of the version   minor field           */
      MAJOR   = 0xff, /*!< Bit offset of the version   major field           */
   };
   /* ---------------------------------------------------------------------- */


   public:
      static uint32_t version (uint8_t   major,
                               uint8_t   minor,
                               uint8_t   patch,
                               uint8_t release);
   };
   /* ---------------------------------------------------------------------- */


   static uint64_t header  (enum Identifier::FrameType frametype,
                            enum Identifier::DataType   dataType,
                            uint32_t                      nbytes,
                            uint32_t                     version);

   static uint64_t header  (enum Identifier::DataType   datatype,
                            uint32_t                      nbytes,
                            uint32_t                     version);

   static uint64_t header  (uint32_t identifier,
                            uint32_t    version);

   uint64_t      m_ui64;
};


class Trailer
{
public:
   Trailer (int nbytes);

   static const uint32_t Pattern = 0x708b309e;

   static uint64_t trailer (uint32_t identifier);

public:
   uint64_t m_trailer;
};

/* ---------------------------------------------------------------------- *//*!
 *
 *   \def   VERSION_COMPOSE
 *   \brief Composes a full version number from its components
 *
 *   \param[in] _major  The major version number.  This should be bumped
 *                      when an incompatible change is made. Generally
 *                      this means that the receiving software must be
 *                      modified.
 *  \param[in]  _ninor  The minor version number.  This should be bumped
 *                      when a backwardly compatiable change is made. This
 *                      happens when a new feature is added, for example,
 *                      a new record type. Existing software can ignore
 *                      this new record type, and can continue to operate
 *                      as before. Naturally, it cannot take advantage or
 *                      use this new record type until that receiving
 *                      software has been upgraded.
 *  \param[in] _patch   The patch number. This should be used to indicate
 *                      a bug fix. This change does not effective the
 *                      interface but by bumping the patch number, a record
 *                      is of the change is maintained
 * \param[in] _release  This should be set to 0 for all production releases
 *                       and non 0 for development releases
 *
 * \par
 *  This is provided as a macro because the version number is a static
 *  concept as composed to a run time concept.
 *
\* ---------------------------------------------------------------------- */
#define VERSION_COMPOSE(_major, _minor, _patch, _release)                  \
                  ( ((  _major) << (int)Header::Version::Offset::MAJOR  )  \
                  | ((  _minor) << (int)Header::Version::Offset::MINOR  )  \
                  | ((  _patch) << (int)Header::Version::Offset::PATCH  )  \
                  | ((_release) << (int)Header::Version::Offset::RELEASE))
/* ---------------------------------------------------------------------- */




inline uint32_t Header::Version::version (uint8_t   major,
                                          uint8_t   minor,
                                          uint8_t   patch,
                                          uint8_t release)
{
   #pragma HLS INLINE

   uint32_t w = ((uint32_t)major   << (int)Header::Version::Offset::MAJOR)
              | ((uint32_t)minor   << (int)Header::Version::Offset::MINOR)
              | ((uint32_t)patch   << (int)Header::Version::Offset::PATCH)
              | ((uint32_t)release << (int)Header::Version::Offset::RELEASE);

   return w;
};


inline Header::Header (uint32_t identifier,
                       uint32_t    version) :
   m_ui64 (header (identifier, version))
{
   #pragma HLS INLINE

   return;
}

inline uint64_t Header::header (uint32_t identifier,
                                uint32_t    version)
{
   #pragma HLS INLINE

   uint64_t w = ((uint64_t)version << 32) | identifier;
   return w;
}

inline uint64_t Header::header  (enum Identifier::DataType    dataType,
                                 uint32_t                       nbytes,
                                 uint32_t                      version)
{
   #pragma HLS INLINE

   uint32_t id = Identifier::identifier (Identifier::FrameType::DATA,
                                         dataType,
                                         nbytes);
   uint64_t w  = header (id, version);
   return w;

}

inline uint64_t Header::header  (enum Identifier::FrameType  frameType,
                                 enum Identifier::DataType    dataType,
                                 uint32_t                       nbytes,
                                 uint32_t                      version)
{
   #pragma HLS INLINE

   uint32_t id = Identifier::identifier (frameType, dataType, nbytes);
   uint64_t w = header (id, version);
   return w;
}

inline uint64_t Trailer::trailer (uint32_t identifier)
{
   #pragma HLS INLINE

   uint64_t w = ((uint64_t)(Trailer::Pattern) << 32) | identifier;
   return w;
}


#endif

