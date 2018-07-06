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



#include "hls_stream.h"
#include "DuneDataCompressionCore.h"
#include "WibFrame-Read.h"
#include "Monitor.h"
#include "MonitorRead.h"



/* =-===================================================================== */
#if PROCESS_K_MODE == PROCESS_K_DATAFLOW
/* ----------------------------------------------------------------------- */
/* PROTOTYPES: Local                                                       */
/* ----------------------------------------------------------------------- */
/* NONE                                                                    */
/* ----------------------------------------------------------------------- */


#include "WibFrame-Write.h"


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

    // ------------------------------------------------------------------
    // Do not pack the monitoring variables
    // This makes the updating much faster (since all are independent
    // updating them will run in 1 cycle, but the cost is an explosion
    // in resources.  Since there are only on the order of 24-32 variables
    // and there are about 60 cycles available for updating, the design
    // choses the less resource intensive solution, i.e. do not pack.
    // ------------------------------------------------------------------
    #if 0
    #pragma HLS DATA_PACK variable=monitor.cfg
    #pragma HLS DATA_PACK variable=monitor.read.summary
    #pragma HLS DATA_PACK variable=monitor.read.errs
    #pragma HLS DATA_PACK variable=monitor.write
    #pragma HLS DATA_PACK variable=cfg
    #endif

   // ------------------------------------------
   // The frame data must be a ping-pong buffer
   // The status can be a FIFO
   // ------------------------------------------
   ReadFrame     frame;
   #pragma HLS RESOURCE variable=frame.m_dat.d  core=RAM_2P_LUTRAM
   #pragma HLS STREAM   variable=frame off


   #pragma HLS RESOURCE variable=lclMonitor.read core=RAM_2P_LUTRAM
   //#pragma HLS RESOURCE variable=lclMonitor.read.summary         core=RAM_2P_LUTRAM
   //#pragma HLS RESOURCE variable=lclMonitor.read.errs            core=RAM_2P_LUTRAM


   // Normally the chns array would be implemented as RAM_2P_LUTRAM
   // But in the straight copy, this array is unused and is optimized away
   ///#pragma HLS RESOURCE variable=lclCfg.chns  core=RAM_2P_LUTRAM
   ModuleConfig lclCfg;
   #pragma HLS STREAM   variable=lclCfg off

   #pragma HLS DATAFLOW


   lclCfg.copy              (cfg);
   lclMonitor.cfg.   update (lclCfg,     monitor.cfg);
   lclMonitor.common.update (lclCfg,  monitor.common);

   frame.read               (sAxis);
   lclMonitor.read.  update (lclCfg, monitor.read, frame.m_status);

   write_frame              (mAxis,  frame,  monitor.write, lclMonitor.write);


   return;
}
/* ----------------------------------------------------------------------- */
/* END: PROCESS_K_DATAFLOW                                                */
/* ====================================================================== */



/* ====================================================================== */
#elif PROCESS_K_MODE == PROCESS_K_COMPRESS
/* ====================================================================== */

#define PRAGMA_SUB(x) _Pragma (#x)
#define PRAGMA_HLS(x) PRAGMA_SUB(x)

#define MODULE_K_NPARALLEL 4
#define MODULE_K_NSERIAL  32

/*
 | This ensures that the correct number of parallel instances of Histogram:integrate_
 | and_size + APE_encode are instantiated
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
#if  MODULE_K_NCHANNELS == 128
#define NHISTPORTS    8
#define NSYMPORTS     4
#else
#error  "MODULE_K_NCHANNELS  must be 128 channels"
#endif


#include "WibFrame-Process.h"



/* ====================================================================== */
/* Local Prototypes                                                       */
/* ---------------------------------------------------------------------- */
static void
 handle_packet (AxisOut                                              &mAxis,
                AxisIn                                               &sAxis);


static void
 acquire_packet (AxisIn                                               &sAxis,
                 PacketContext                                       &pktCtx,
                 CompressionContext                                  &cmpCtx);


static void
 process_packet (AxisOut                                              &mAxis,
                 PacketContext                                       &pktCtx,
                 CompressionContext                                  &cmpCtx);


static void
 acquire_frame (AxisIn                                                &sAxis,
                PacketContext                                        &pktCtx,
                CompressionContext                                   &cmpCtx,
                int                                                   iframe);
/* ====================================================================== */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Top level entry to the data handling and compression
 *
 *  \param[ in]      sAxis The  input data stream
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
 *
\* ----------------------------------------------------------------------- */
void DuneDataCompressionCore (AxisIn            &sAxis,
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
    #pragma HLS STREAM variable=monitor off

   // Set the input and output ports as AXI4-StreaNm
   #pragma HLS INTERFACE axis port=sAxis  // Input Port
   #pragma HLS INTERFACE axis port=mAxis  // Output Port

   // Setup a AXI-Lite bus for status/configuration registers
   #pragma HLS INTERFACE s_axilite port=config           bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=monitor register bundle=BUS_A
   #pragma HLS INTERFACE s_axilite port=return           bundle=BUS_A

   //#pragma HLS DATA_PACK variable=config

   #pragma HLS DATAFLOW

   handle_packet (mAxis, sAxis);

   return;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Handles the reading and writing of each packet
 *
 *  \param[ in]       sAxis The  input data stream
 *  \param[out]       mAxis The output data stream
 *  \param[ in]   moduleIdx This refers to which group of channels are
 *                          being processed. Channels typically arrive
 *                          and are processed in groups of 128. An RCE
 *                          currently service 256 channels or 2 groups
 * \param[ in]      config Configuration information including a flag
 *                         indicating that the system needs to be
 *                         configured or reconfigured.
 * \param[out]  monitorRead Information that is used to monitor the
 *                          performance and reading of each frame,
 *
 * \param[out] monitorWrite Information that is used to monitor the
 *                          performance and writing of each packet,
 *
\* ----------------------------------------------------------------------- */
static void handle_packet (AxisOut                   &mAxis,
                           AxisIn                    &sAxis)
{
   #pragma HLS DATAFLOW


   PacketContext      pktCtx;
   CompressionContext cmpCtx;

   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.adcs.sg0  cyclic factor=NSYMPORTS  dim=1)
   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.adcs.sg1  cyclic factor=NSYMPORTS  dim=1)
   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.adcs.sg2  cyclic factor=NSYMPORTS  dim=1)
   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.adcs.sg3  cyclic factor=NSYMPORTS  dim=1)

   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.hists.sg0 cyclic factor=NHISTPORTS dim=1)
   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.hists.sg1 cyclic factor=NHISTPORTS dim=1)
   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.hists.sg2 cyclic factor=NHISTPORTS dim=1)
   PRAGMA_HLS(HLS ARRAY_PARTITION variable=cmpCtx.hists.sg3 cyclic factor=NHISTPORTS dim=1)

   // -------------------------------------------------
   // Set histogram identifiers, strictly for debugging
   // -------------------------------------------------
   HISTOGRAM_statement (cmpCtx.hists.set_ids (MODULE_K_NPARALLEL));

   acquire_packet (sAxis, pktCtx, cmpCtx);
   process_packet (mAxis, pktCtx, cmpCtx);

   return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Top level entry to the acquiring a packet.  A packet consists
 *         PACKET_K_NSAMPLES frames.
 *
 *  \param[ in]     sAxis  The  input data stream
 *  \param[out]    pktCtx  Context pertaining to the entire packet
 *  \param[out[      syms  The symbols that will be compressed. These
 *                         are derived from the ADCs by some reversible
 *                         process so that the original ADCs can be
 *                         recovered.
 *  \param[ in]      hists Histograms of \a syms. These form the basis
 *                         of the compression.
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
void acquire_packet (AxisIn                                        &sAxis,
                     PacketContext                                &pktCtx,
                     CompressionContext                           &cmpCtx )
{
   #pragma HLS INLINE off


   // -------------------------------------------
   // Accumulates the statistics and status
   // associated with the reading of a WIB frame.
   // -------------------------------------------

   /// STRIP --- if (config.init) lclMonitor.init ();

   ACQUIRE_LOOP:
   for (int idx = 0; idx < PACKET_K_NSAMPLES; idx++)
   {
      acquire_frame (sAxis,  pktCtx,      cmpCtx, idx);
   }

   //// monitor = lclMonitor;

   ///Histogram::print (hists, 128);
   return;
}
/* ----------------------------------------------------------------------- */




/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Top level entry to the acquiring one frame of the PACKET_K_NSAMPLES
 *         frames that compose a packet.
 *
 *  \param[ in]     sAxis  The  input data stream
 *  \param[out]    pktCtx  Context pertaining to the entire packet
 *  \param[out[      syms  The symbols that will be compressed. These
 *                         are derived from the ADCs by some reversible
 *                         process so that the original ADCs can be
 *                         recovered.
 *  \param[ in]      hists Histograms of \a syms. These form the basis
 *                         of the compression.
 *  \param[ in]  moduleIdx This refers to which group of channels are
 *                         being processed. Channels typically arrive
 *                         and are processed in groups of 128. An RCE
 *                         currently service 256 channels or 2 groups
 * \param[ in]     config  Configuration information including a flag
 *                         indicating that the system needs to be
 *                         configured or reconfigured.
 * \param[out] gblMonitor  Returned with information that monitors the
 *                         the reading of the frames.
 * \param[out] lclMonitor  Updated with information that monitors the
 *                         the reading of the frames.
 *
 * \par
 *  To some extent this method is unnecessary, it could have been
 *  incorporated into the aquire_packet method. Breaking it out as a
 *  separate method makes the contribution to the Vivado reports easier
 *  to read.  I've also had problems with implementing DATAFLOW within
 *  regions.
 *
\* ----------------------------------------------------------------------- */
static void acquire_frame (AxisIn                                        &sAxis,
                           PacketContext                                &pktCtx,
                           CompressionContext                           &cmpCtx,
                           int                                           iframe)
{
   #pragma HLS INLINE
   #pragma HLS DATAFLOW

   ReadFrame frame;
   #pragma HLS RESOURCE        variable=frame.m_dat.d          core=RAM_2P_LUTRAM
   #pragma HLS ARRAY_PARTITION variable=frame.m_dat.d cyclic factor=2

   frame.read        (sAxis);
   /// STRIP -- lclMonitor.update (config, gblMonitor, status);  ///// Hangs the RTL cosim if use config
   process_frame     (frame, iframe, pktCtx, cmpCtx);
}
/* ----------------------------------------------------------------------- */



#define WRITE_DATA 1

#if       WRITE_DATA
#include "PacketWrite.h"


/* ----------------------------------------------------------------------- */
static void process_packet
           (AxisOut                                                  &mAxis,
            PacketContext                                           &pktCtx,
            CompressionContext                                      &cmpCtx )
{
   #pragma HLS INLINE off

   write_packet  (mAxis,  pktCtx, cmpCtx);

   return;
}
/* ----------------------------------------------------------------------- */
#endif
/* ----------------------------------------------------------------------- */


#endif
