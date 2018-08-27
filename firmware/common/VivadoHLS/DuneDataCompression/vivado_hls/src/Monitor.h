// -*-Mode: C++;-*-

#ifndef _DUNE_DATA_COMPRESSION_MONITOR_H_
#define _DUNE_DATA_COMPRESSION_MONITOR_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     Monitor.h
 *  @brief    Defines classes that monitor the processing
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
 *  2018/04/18
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
   2018.04.18 jjr Created, split off from DuneDataCompressionTypes.h
   
\* ---------------------------------------------------------------------- */

#include "Config.h"
#include "WibFrame-Read.h"

#include "ap_int.h"
#include <stdint.h>


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorCommon
 *  \brief Track status and statistics common to the module
 *
\* ---------------------------------------------------------------------- */
struct MonitorCommon
{
public:
   void update (ModuleConfig const  cfg,
                MonitorCommon      &gbl);
   
   uint32_t   pattern; /*!< Recognizable bit pattern                      */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Updates the common/static status
 *
 *  \param[in]  cfg The active configuration
 *
 *  \param[   out]  gbl  The module's global configuration status.
 *                       This is updated with the local common monitor 
 *
\* ---------------------------------------------------------------------- */
inline void MonitorCommon::update(ModuleConfig const   cfg,
                                  MonitorCommon       &gbl)
{
     pattern = 0xabadcafe;
     gbl     = *this;

     return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorCfg
 *  \brief Track status and statistics configuration requests
 *  
 *  \param[in]  cfg The active configuration
 *  \param[out] gbl The module's global configuration status.
 *                  This is updated with the configuation 
 *                  monitor
 *  \param[out[
 *
\* ---------------------------------------------------------------------- */
struct MonitorCfg
{
public:
   void update (ModuleConfig const   cfg,
                MonitorCfg          &gbl);
   
   uint32_t    m_mode; /*!< Last configured acquisition mode              */
   uint32_t   m_ncfgs; /*!< Number of configuration requests              */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Updates the configuration monitor
 *
 *  \param[     in]     cfg  The configuration block
 *  \param[   out]      gbl  The module's global configuration status.
 *                           This is updated with the local read status
 *
\* ---------------------------------------------------------------------- */
inline void MonitorCfg::update (ModuleConfig const   cfg,
                                MonitorCfg          &gbl)
{
   static bool First = true;
   if (cfg.init)
   {
      if (First) m_ncfgs  = 1;
      else       m_ncfgs += 1;
      First = false;
      m_mode = cfg.mode;
   }

   gbl = *this;

   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct MonitorRead
 *  \brief Track status and statistics for read method
 *
\* ---------------------------------------------------------------------- */
struct MonitorRead
{
public:
   MonitorRead ()
   {
      //init ();
      return;
   }
   
public:
   void update (ModuleConfig const  cfg,
                MonitorRead        &gbl,
                ReadStatus      status);

   void update (ModuleConfig  const cfg,
                ReadStatus       status);


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

   typedef uint32_t  Counter;

   struct Summary
   {
   public:
      ap_uint<32>          mask; /* < Status mask                            */
      uint32_t          nframes; /*!< Total number frames, this must be the
                                      sum of the first 4                     */
      Counter        nStates[4]; /*!< Count of Frame states (Run/Enabled     */
   };

   struct Errors
    {
   public:
       Counter     nFrameErrs[5]; /*!< Count of framing errors (Sof/Eof etc)  */
       Counter      nWibErrs[24]; /*!< Count of WIB errors                    */
    };

   /* ---------------------------------------------------------------------- *//*!
    *
    *  brief  Conditionally increments the specified Wib Error counter
    *
    *  \param[in] increament If true, then increment the counter
    *  \param[in]    counter Which counter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   Counter count (bool increment, WibCounter counter)
   {
      #pragma HLS INLINE
      if (increment)
      {
         return errs.nWibErrs[static_cast<unsigned>(counter)] += 1;
      }
      else
      {
         return errs.nWibErrs[static_cast<unsigned>(counter)];
      }
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
    *
    *  brief  Conditionally increments the specified Wib Error counter
    *
    *  \param[in] increament If true, then increment the counter
    *  \param[in]    counter Which counter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   Counter count (bool increment, WibCounter counter, int off)
   {
      #pragma HLS INLINE
      if (increment)
      {
         return errs.nWibErrs[static_cast<unsigned>(counter) + off] += 1;
      }
      else
      {
         return errs.nWibErrs[static_cast<unsigned>(counter) + off];
      }
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
    *
    *  brief  Unconditionally increments the specified Wib Error counter
    *
    *  \param[in] counter Which counter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   void count (WibCounter counter)
   {
      #pragma HLS INLINE
       errs.nWibErrs[static_cast<unsigned>(counter)] += 1;
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
    *
    *  \brief  Convenience method to return the specfied WibErrorCounter,
    *          basically launders the enum WibCounter to be used as an index
    *
    *  \return The WibError counter value
    *
    *  \param[in] counter Which counter to return
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   Counter operator[] (WibCounter counter) const
   {
      #pragma HLS INLINE
      return errs.nWibErrs[static_cast<unsigned>(counter)];
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
    *
    *  brief  Conditionally increments the specified Frame Error counter
    *
    *  \param[in] increament If true, then increment the counter
    *  \param[in]        idx The index of the counter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   Counter count (bool increment, FrameCounter counter)
   {
      #pragma HLS INLINE
      if (increment)
      {
         return errs.nFrameErrs[static_cast<unsigned>(counter)] += 1;
      }
      else
      {
         return errs.nFrameErrs[static_cast<unsigned>(counter)];
      }
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
    *
    *  brief  Unconditionally increments the specified Frame Error counter
    *
    *  \param[in]  counter Which ounter to increment
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   void count (FrameCounter counter)
   {
      #pragma HLS INLINE
      errs.nFrameErrs[static_cast<unsigned>(counter)] += 1;
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
    *
    *  \brief  Convenience method to return the specfied FrameErr counter value
    *          basically launders the enum FrameCounter to be used as an index
    *
    *  \return The nStates counter value
    *
    *  \param[in] counter Which counter to return
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   Counter operator[] (FrameCounter counter) const
   {
      #pragma HLS INLINE
      return errs.nFrameErrs[static_cast<unsigned>(counter)];
   }
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *//*!
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
      summary.nStates[state] += 1;
   }
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
    *
    *  \brief  Convenience method to return the specfied StateCounter value
    *          basically launders the enum StateCounter to be used as an index
    *
    *  \return The nStates counter value
    *
    *  \param[in] counter Which counter to return
    *                                                                        */
   /* ---------------------------------------------------------------------- */
   Counter operator[] (StateCounter counter) const
   {
      #pragma HLS INLINE
      return summary.nStates[static_cast<unsigned>(counter)];
   }



   void init ()
   {
      #pragma HLS INLINE

      WIBERRS_CLEAR_LOOP:
      for (int idx = 0; idx < sizeof (errs.nWibErrs) / sizeof (errs.nWibErrs[0]); ++idx)
      {
         errs.nWibErrs[idx] = 0;
      }

      WIBSTATES_CLEAR_LOOP:
      for (int idx = 0; idx < sizeof (summary.nStates) / sizeof (summary.nStates[0]); ++idx)
      {
         summary.nStates[idx] = 0;
      }

      WIBFRAMEERRS_CLEAR_LOOP:
      for (int idx = 0; idx < sizeof (errs.nFrameErrs) / sizeof (errs.nFrameErrs[0]); ++idx)
      {
         errs.nFrameErrs[idx] = 0;
      }

      summary.nframes   = 0;
      summary.mask      = 0;
   }

   Summary           summary; /*!< Summary counters and variables         */
   Errors               errs; /*!< The various error counters             */
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
public:
   MonitorWrite () { return; }

   void init ()
   {
      nbytes    = 0;
      npromoted = 0;
      ndropped  = 0;
      npackets  = 0;
   }

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
#endif
