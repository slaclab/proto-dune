// -*-Mode: C++;-*-


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionCore_test.cpp
 *  @brief    Test harness for the DUNE compresssion
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


#define __STDC_FORMAT_MACROS

#include "DuneDataCompressionCore.h"
#include "DuneDataCompressionTypes.h"
#include "WibFrame.h"
#include "AxisIO_test.h"

#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errno.h>



/* ---------------------------------------------------------------------- */
/* Local Prototypes                                                       */
/* ---------------------------------------------------------------------- */
static int      open_file (char const           *filename);

static ssize_t  read_adcs (int                          fd,
                           char const            *filename,
                           uint16_t                  *adcs,
                           int                       nadcs);

static int  compress_test (int                          fd,
                           char const            *filename,
                           uint32_t               headerId,
                           uint32_t                version);

static int      copy_test (int                          fd,
                           char const            *filename,
                           uint32_t               headerId,
                           uint32_t                version);

static void configure_hls (Source                     &src,
                           MyStreamOut              &mAxis,
                           ModuleIdx_t           moduleIdx,
                           uint32_t               headerId,
                           uint32_t                version,
                           uint64_t              timestamp,
                           ModuleConfig            &config,
                           MonitorModule          &monitor);

static int    fill_packet (Source                     &src,
                           uint32_t               headerId,
                           uint32_t                version,
                           uint64_t              timestamp,
                           uint16_t adcs[PACKET_K_NSAMPLES]
                                       [MODULE_K_NCHANNELS],
                           int                    runEnable,
                           int                        flush);

static void        update (MonitorModule          &monitor,
                           bool              output_packet);

static bool        differ (MonitorModule const         &s0,
                           MonitorModule const         &s1);

static int    check_empty (MyStreamOut              &mAxis,
                           MyStreamIn                 &src);

static void print_monitor (MonitorModule const         &s0,
                           MonitorModule const         &s1,
                           int                     ipacket,
                           int                     isample);

static void decode        (AxisOut                  &mAxis);

/* ---------------------------------------------------------------------- */


#define NPACKETS 2

#if PROCESS_K_MODE == PROCESS_K_DATAFLOW
#define MODE MODE_K_COPY
#else
#define MODE MODE_K_COMPRESS
#endif

/* ---------------------------------------------------------------------- *//*!
  \process Processes all the data from the specified file

  \param[out]   slice  The array of time slices to fill
  \param [in]     dir  The root directory of the data files
  \param [in]     run  The run number
  \param [in]   event  The event number
  \param [in] channel  The channel to process
  \param [in]   print  Print flags, all adcs will be printed to the
                       terminal.
                                                                          */
/* ---------------------------------------------------------------------- */
int main (int argc, char const *argv[])
{

   uint32_t version  = VERSION_COMPOSE(1, 0, 0, 0);
   uint32_t headerId = Identifier::identifier (Identifier::FrameType::DATA,
                                               Identifier:: DataType:: WIB,
                                               0);

   unsigned int                     run =                       15660;
   static char const DefaultDirectory[] =       "/u/eb/convery/ForJJ";
   static char const               *dir =            DefaultDirectory;
   static const char     FileTemplate[] = "%s/run%u_binary/run%u.bin";
   char filename[256];


   // --------------------------------------
   // Compose the test file name and open it
   // --------------------------------------
   sprintf (filename, FileTemplate, dir, run, run);
   int fd = open_file (filename);



   // -----------------------
   // Set the processing mode
   // -----------------------
   int   mode = MODE;
   int status = (mode == MODE_K_COPY)
              ?     copy_test (fd, filename, headerId, version)
              : compress_test (fd, filename, headerId, version);


   if (status == 0)
   {
      printf ("Passed\n");
   }

   return 0;
   return status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int copy_test (int               fd,
                      char const *filename,
                      uint32_t    headerId,
                      uint32_t     version)
{
   Source                             src;
   MyStreamOut              mAxis ("Out");
   ModuleIdx_t              moduleIdx = 1;
   ModuleConfig                    config;
   MonitorModule                  monitor;
   MonitorModule               expMonitor;
   int                        test_status;


   std::cout << "MODE = COPY" << std::endl;
   uint64_t timestamp = 0xab000000;

   configure_hls (src, mAxis, moduleIdx, headerId, version, timestamp, config, monitor);

   memset (&expMonitor, 0, sizeof (expMonitor));
   print_monitor (expMonitor, monitor, 0,-1);
   expMonitor  = monitor;


   static uint16_t adcs[PACKET_K_NSAMPLES][MODULE_K_NCHANNELS];
   for (int ipacket = 0; ipacket < NPACKETS; ipacket++)
   {
      uint32_t summary = 0;

      ssize_t nread = read_adcs (fd, filename, (uint16_t *)adcs, sizeof (adcs)/sizeof (adcs[0][0]));
      if (nread == 0) break;


      int runEnable = 1;
      int flush     = 0;


      // ----------------------------------
      // Fill a packets worth of Wib frames
      // -----------------------------------
      int nbytes = fill_packet (src, headerId, version, timestamp, adcs, runEnable, flush);

      // Process each packet for all time samples
      for (int isample = 0; isample < PACKET_K_NSAMPLES; isample++)
      {
         DuneDataCompressionCore(src.m_src, mAxis, moduleIdx, config, monitor);

         // --------------------------------------------------
         // Update monitoring status variables for this frame
         // Report if they are not as expected or this is the
         // last frame in the packet
         // -------------------------------------------------
         update (expMonitor, isample == (PACKET_K_NSAMPLES -1));
         if (differ (expMonitor, monitor) || isample == (PACKET_K_NSAMPLES-1))
         {
            static int Count = 0;
            if (Count < 32)
            {
               print_monitor (expMonitor, monitor, ipacket, isample);
            }
            Count     += 1;
            expMonitor = monitor;
         }

         summary |= monitor.read.summary.mask;
      }


      // ------------------------------
      // Add trailer to the check frame
      // and check the completed packet
      // -------------------------------
      src.add_trailer (version, headerId, summary, nbytes);
      test_status = MyStreamOut::check (mAxis, src.m_chk, ipacket);


      if (test_status)
      {
         printf ("Failed\n");
         break;
      }

      printf ("NEXT\n");
   }


   // ---------------------------------------------------
   // If have passed all the tests so far, make sure both
   // the input and output streams are not empty
   // ---------------------------------------------------
   if (test_status == 0)
   {
      test_status = check_empty (mAxis, src.m_src);
   }

   return test_status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int compress_test (int               fd,
                          char const *filename,
                          uint32_t    headerId,
                          uint32_t     version)
{
   Source                             src;
   MyStreamOut              mAxis ("Out");
   ModuleIdx_t              moduleIdx = 1;
   ModuleConfig                    config;
   MonitorModule               expMonitor;
   MonitorModule                  monitor;
   int                        test_status;

   #pragma HLS STREAM variable=mAxis depth=32768

   std::cout << "MODE = COMPRESS" << std::endl;
   std::cout << "-- BY PASSING CHECKING" << std::endl;

   memset (&expMonitor, 0, sizeof (expMonitor));
   ////print_monitor (expMonitor, monitor, 0,-1);

   // Ignore the first time flag;
   config.init = -1;
   uint64_t timestamp = 0x00800000LL;

   for (int ipacket = 0; ipacket < NPACKETS; ipacket++)
   {
      static uint16_t adcs[PACKET_K_NSAMPLES][MODULE_K_NCHANNELS];
      uint32_t summary = 0;

      // -------------------
      // Read one time slice
      // -------------------
      ssize_t nread = read_adcs (fd, filename, (uint16_t *)adcs, sizeof (adcs)/ sizeof (adcs[0][0]));
      if (nread == 0) break;

      int runEnable = 1;
      int flush     = 0;

      // ----------------------------------
      // Fill a packets worth of Wib frames
      // -----------------------------------
      int nbytes  = fill_packet (src, headerId, version, timestamp, adcs, runEnable, flush);
      timestamp  += 25 * PACKET_K_NSAMPLES;

      // -------------------
      // Compress the packet
      // -------------------
      DuneDataCompressionCore(src.m_src, mAxis, moduleIdx, config, monitor);

      print_monitor (expMonitor, monitor, ipacket, ipacket*0x400);

      // ------------------------------
      // Add trailer to the check frame
      // Do the check
      // ------------------------------
      src.add_trailer (version, headerId, summary, nbytes);

      decode (mAxis);
      test_status = 0; //// MyStreamOut::check (mAxis, src.m_chk, ipacket);

      // -------------------------
      // Abort the test on failure
      // -------------------------
      if (test_status)
      {
         printf ("Failed\n");
         break;
      }

      config.init = 0;
      printf ("NEXT\n");
   }


   // ---------------------------------------------------
   // If have passed all the tests so far, make sure both
   // the input and output streams are not empty
   // ---------------------------------------------------
   if (test_status == 0)
   {
      check_empty (mAxis,     src.m_src);
      src.drainCheck ();
   }

   return test_status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Open the test file of ADCs
 *   \return The file description of the opened file
 *
 *   \param[in]  filename  The name of the file to open
 *
 *   \par
 *    An unsuccessful opens terminates the program
 *
\* ---------------------------------------------------------------------- */
static int open_file (char const *filename)
{
   int fd = ::open (filename, O_RDONLY);
   if (fd < 0)
   {
      char const *err = strerror (errno);

      fprintf (stderr,
               "ERROR : Unable to open: %s\n"
               "Reason: %s\n", filename, err);
      exit (-1);
   }

   return fd;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads the specified number of \a adcs from the file
 *  \return The number of bytes write or 0 if EOF
 *
 *  \param[ in]       fd The file description of the opened file
 *  \param[ in] filename The file name, used only for reporting
 *  \param[out]     adcs Filled in with the read ADCs
 *  \param[ in[    nadcs The number of ADCs to read
 *
 *  \par
 *   Any read errors abort the program.
 *
\* ---------------------------------------------------------------------- */
static ssize_t read_adcs (int               fd,
                          char const *filename,
                          uint16_t       *adcs,
                          int            nadcs)
{
   ssize_t nbytes =    nadcs * sizeof (*adcs);
   ssize_t nread  = ::read (fd, adcs, nbytes);

   if (nread != nbytes)
   {
      if (nread < 0)
      {
         // Error
         char const *err = strerror (errno);
         fprintf (stderr,
                  "ERROR : error reading : %s\n"
                  "Reason: %s\n", filename, err);
         exit (-1);
      }
      else
      {
         // EOF
         return 0;
      }
   }


   return nread;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void configure_hls (Source            &src,
                           MyStreamOut     &mAxis,
                           ModuleIdx_t  moduleIdx,
                           uint32_t      headerId,
                           uint32_t       version,
                           uint64_t     timestamp,
                           ModuleConfig   &config,
                           MonitorModule &monitor)
{
   // This is suppose to do the configuration
   config.init  = true;
   config.mode  = MODE_K_COPY;
   config.limit = 1 + 30 * PACKET_K_NSAMPLES + 1;


   // Add the header to the check stream
   ///src.add_header (headerId, version);
   src.fill_runDisableAndFlush_frame (timestamp, 0);

   DuneDataCompressionCore (src.m_src, mAxis, moduleIdx, config, monitor);

   // --------------------------------------------------
   // Drain the check stream
   // --------------------------------------------------
   while (!src.m_chk.empty ()) src.m_chk.read ();

   // --------------------------------------------------
   // The output and input streams must be empty
   // The input frame should have been consumed
   // The frame is not written when the run is disabled
   // --------------------------------------------------
   int status = check_empty (mAxis, src.m_src);
   if (status)
   {
      printf ("Aborting\n");
      exit (-1);
   }

   //printf ("Kludge -- quitting after configuration\n");
   //exit (0);

   config.init = false;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int fill_packet (Source                                          &src,
                        uint32_t                                    headerId,
                        uint32_t                                     version,
                        uint64_t                                   timestamp,
                        uint16_t adcs[PACKET_K_NSAMPLES][MODULE_K_NCHANNELS],
                        int                                        runEnable,
                        int                                            flush)
 {

    // ----------------------------------
    // Add the header to the check stream
    // ----------------------------------
    //src.add_header (headerId, version);
    //int nbytes       = sizeof (Header);

    int nbytes = 0;
    for (int isample = 0; isample < PACKET_K_NSAMPLES; isample++)
    {
       if (flush | (runEnable == 0))
       {
          printf ("Sample[%4u] Flush:%d runEnable:%d\n", isample, flush, runEnable);
          src.fill_empty_frame (timestamp, runEnable, flush, isample);
       }
       else
       {
          src.fill_frame (timestamp, adcs[isample], runEnable, flush, isample);
          nbytes += sizeof (WibFrame);
       }

       timestamp += 25;
    }

    return nbytes;
 }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void update (MonitorModule &status, bool output_packet)
{
   status.read.summary.nframes    += 1;
   status.read.summary.nStates[0] += 1;

   status.write.npromoted += 1;
   status.write.npackets  += output_packet;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static bool differ (MonitorModule const &s0,
                    MonitorModule const &s1)
{
   // NOTE: types[0] is not checked.  This is the normal
   //       frame counter and would cause a difference
   //        every time.
   if (s0.common.pattern          != s1.common.pattern         ) return true;
   if (s0.cfg.m_ncfgs             != s1.cfg.m_ncfgs            ) return true;
   if (s0.cfg.m_mode              != s1.cfg.m_mode             ) return true;
   if (s0.read.summary.mask       != 0                         ) return true;
   if (s0.read.summary.nStates[0] != s1.read.summary.nStates[0]) return true;
   if (s0.read.summary.nStates[1] != s1.read.summary.nStates[1]) return true;
   if (s0.read.summary.nStates[2] != s1.read.summary.nStates[2]) return true;
   if (s0.read.summary.nStates[3] != s1.read.summary.nStates[3]) return true;
   if (s0.write.npackets          != s1.write.npackets         ) return true;


   for (unsigned idx = 0; idx < sizeof (s0.read.errs.nWibErrs) / sizeof (s1.read.errs.nWibErrs[0]); idx++)
   {
      if (s0.read.errs.nWibErrs[idx] != s1.read.errs.nWibErrs[idx]) return true;
   }

   return false;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
static void print_monitor (MonitorModule const &s0,
                           MonitorModule const &s1,
                           int             ipacket,
                           int             isample)
{
   printf ("New status at packet.sample: %3u.%4u\n"
         " Common              .pattern: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         " Cfg                    .mode: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                       .ncfgs: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         " Read                 .status: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                     .nframes: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         "     .state [         Normal]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [       Disabled]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [          Flush]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [    DisAndFlush]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         "     .frame [        ErrSofM]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        ErrSofU]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        ErrEofM]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        ErrEofU]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        ErrEofE]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         "     .wib   [       ErrComma]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [   ErrVersionId]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [          ErrId]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        ErrRsvd]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [      ErrErrors]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [ErrWibTimestamp]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        Unused6]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        Unused7]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         "    .cd     [     ErrStrErr1]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [     ErrStrErr2]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [       ErrRsvd0]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [      ErrChkSum]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [      ErrCvtCnt]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [      ErrErrReg]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [       ErrRsvd1]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [        ErrHdrs]: %8.8" PRIx32 " -> %8.8" PRIx32 ": %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         " Write             .npromoted: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                    .ndropped: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                    .npackets: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                      .nbytes: %8.8" PRIx32 " -> %8.8" PRIx32 "\n",
            ipacket, isample,

            s0.common.pattern,                                   s1.common.pattern,
            s0.cfg.m_mode,                                       s1.cfg.m_mode,
            s0.cfg.m_ncfgs,                                      s1.cfg.m_ncfgs,

            s0.read.summary.mask.to_uint (),                     s1.read.summary.mask.to_uint (),
            s0.read.summary.nframes,                             s1.read.summary.nframes,
            s0.read[MonitorRead::StateCounter::         Normal], s1.read[MonitorRead::StateCounter::         Normal],
            s0.read[MonitorRead::StateCounter::    RunDisabled], s1.read[MonitorRead::StateCounter::    RunDisabled],
            s0.read[MonitorRead::StateCounter::          Flush], s1.read[MonitorRead::StateCounter::          Flush],
            s0.read[MonitorRead::StateCounter::       DisFlush], s1.read[MonitorRead::StateCounter::       DisFlush],

            s0.read[MonitorRead::FrameCounter::        ErrSofM], s1.read[MonitorRead::FrameCounter::        ErrSofM],
            s0.read[MonitorRead::FrameCounter::        ErrSofU], s1.read[MonitorRead::FrameCounter::        ErrSofU],
            s0.read[MonitorRead::FrameCounter::        ErrEofM], s1.read[MonitorRead::FrameCounter::        ErrEofM],
            s0.read[MonitorRead::FrameCounter::        ErrEofU], s1.read[MonitorRead::FrameCounter::        ErrEofU],
            s0.read[MonitorRead::FrameCounter::        ErrEofE], s1.read[MonitorRead::FrameCounter::        ErrEofE],

            s0.read[MonitorRead::WibCounter::      ErrWibComma], s1.read[MonitorRead::WibCounter::      ErrWibComma],
            s0.read[MonitorRead::WibCounter::    ErrWibVersion], s1.read[MonitorRead::WibCounter::    ErrWibVersion],
            s0.read[MonitorRead::WibCounter::         ErrWibId], s1.read[MonitorRead::WibCounter::         ErrWibId],
            s0.read[MonitorRead::WibCounter::       ErrWibRsvd], s1.read[MonitorRead::WibCounter::       ErrWibRsvd],
            s0.read[MonitorRead::WibCounter::     ErrWibErrors], s1.read[MonitorRead::WibCounter::     ErrWibErrors],
            s0.read[MonitorRead::WibCounter::  ErrWibTimestamp], s1.read[MonitorRead::WibCounter::  ErrWibTimestamp],
            s0.read[MonitorRead::WibCounter::    ErrWibUnused6], s1.read[MonitorRead::WibCounter::    ErrWibUnused6],
            s0.read[MonitorRead::WibCounter::    ErrWibUnused7], s1.read[MonitorRead::WibCounter::    ErrWibUnused7],

            s0.read[MonitorRead::WibCounter::    ErrCd0StrErr1], s1.read[MonitorRead::WibCounter::    ErrCd0StrErr1],
            s0.read[MonitorRead::WibCounter::    ErrCd1StrErr1], s1.read[MonitorRead::WibCounter::    ErrCd1StrErr1],

            s0.read[MonitorRead::WibCounter::    ErrCd0StrErr2], s1.read[MonitorRead::WibCounter::    ErrCd0StrErr2],
            s0.read[MonitorRead::WibCounter::    ErrCd1StrErr2], s1.read[MonitorRead::WibCounter::    ErrCd1StrErr2],

            s0.read[MonitorRead::WibCounter::      ErrCd0Rsvd0], s1.read[MonitorRead::WibCounter::      ErrCd0Rsvd0],
            s0.read[MonitorRead::WibCounter::      ErrCd1Rsvd0], s1.read[MonitorRead::WibCounter::      ErrCd1Rsvd0],

            s0.read[MonitorRead::WibCounter::     ErrCd0ChkSum], s1.read[MonitorRead::WibCounter::     ErrCd0ChkSum],
            s0.read[MonitorRead::WibCounter::     ErrCd1ChkSum], s1.read[MonitorRead::WibCounter::     ErrCd1ChkSum],

            s0.read[MonitorRead::WibCounter::     ErrCd0CvtCnt], s1.read[MonitorRead::WibCounter::     ErrCd0CvtCnt],
            s0.read[MonitorRead::WibCounter::     ErrCd1CvtCnt], s1.read[MonitorRead::WibCounter::     ErrCd1CvtCnt],

            s0.read[MonitorRead::WibCounter::     ErrCd0ErrReg], s1.read[MonitorRead::WibCounter::     ErrCd0ErrReg],
            s0.read[MonitorRead::WibCounter::     ErrCd1ErrReg], s1.read[MonitorRead::WibCounter::     ErrCd1ErrReg],

            s0.read[MonitorRead::WibCounter::      ErrCd0Rsvd1], s1.read[MonitorRead::WibCounter::      ErrCd0Rsvd1],
            s0.read[MonitorRead::WibCounter::      ErrCd1Rsvd1], s1.read[MonitorRead::WibCounter::      ErrCd1Rsvd1],

            s0.read[MonitorRead::WibCounter::       ErrCd0Hdrs], s1.read[MonitorRead::WibCounter::       ErrCd0Hdrs],
            s0.read[MonitorRead::WibCounter::       ErrCd1Hdrs], s1.read[MonitorRead::WibCounter::       ErrCd1Hdrs],

            s0.write.npromoted,                                  s1.write.npromoted,
            s0.write.ndropped,                                   s1.write.ndropped,
            s0.write.npackets,                                   s1.write.npackets,
            s0.write.nbytes,                                     s1.write.nbytes);


      return;
}
/* ---------------------------------------------------------------------- */

#include "BFU.h"

/* ---------------------------------------------------------------------- */
static int check_empty (MyStreamOut &mAxis, MyStreamIn &sAxis)
{
   int status = 0;
   if (!mAxis.empty()) { status = -2; std::cout << "mAxis is not empty" << std::endl; }
   if (!sAxis.empty()) { status = -3; std::cout << "sAxis is not empty" << std::endl; }

   if (status == -2)
   {
      int idx = 0;
      std::cout << "mAxis Data.Dest.  Id.  Keep.Last.Strb.User" << std::endl;
      while (!mAxis.empty ())
      {
         AxisOut_t out = mAxis.read ();
         std::cout << ' ' << std::hex << std::setw (4) << idx
                   << std::setw(16) << ' ' << out.data << std::setw (4) << '.'
                   << out.dest      << '.' << out.id   << '.'
                   << out.keep      << '.' << out.last << '.' << out.strb << '.' << out.user << std::endl;
         idx += 1;
      }
   }


   if (status == -3)
   {
      int idx = 0;
      std::cout << "sAxis Data.Dest.  Id.  Keep.Last.Strb.User" << std::endl;
      while (!sAxis.empty ())
      {
         AxisOut_t out = sAxis.read ();
         std::cout << ' ' << std::hex << std::setw (4) << idx
                   << std::setw(16) << ' ' << out.data << std::setw (4) << '.'
                   << out.dest      << '.' << out.id   << '.'
                   << out.keep      << '.' << out.last << '.' << out.strb << '.' << out.user << std::endl;
         idx += 1;
      }
   }

   return status;
}
/* ---------------------------------------------------------------------- */



#define DECODE 1

struct Toc
{
   uint16_t            m_n64; /*!< Number of 64 bit words                 */
   uint16_t       m_nsamples; /*!< Number of samples                      */
   uint16_t      m_nchannels; /*!< Number of channels                     */
   uint32_t const *m_offsets; /*!< Pointer to the channel offsets         */
};

static int      toc_decode (Toc      *toc,                       uint64_t const *buf, int nbuf);
static int   header_decode (uint64_t *headers, uint32_t *status, uint64_t const *buf, int nbuf);
static int     chan_decode (uint64_t const *buf, int nbuf, int position);
static int     hist_decode (uint16_t *bins, int *nrbins, int *first, int *novrflw, BFU &bfu, uint64_t const *buf);
static void hist_integrate (uint16_t *table,uint16_t *bins, int nbins);
static int      sym_decode (uint16_t *syms, uint16_t const *table, uint16_t sym, BFU bfu, uint64_t const *buf, int ovrpos, int novrflw);
static int16_t     restore (uint16_t   sym);
static void  print_decoded (uint16_t   sym, int idy);


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Decode the Axis Stream
 *
 *   \param[in] mAxis  The output Axis Stream
 *
\* ---------------------------------------------------------------------- */
static void decode (AxisOut &mAxis)
{
   int  nbuf = 0;
   int nerrs = 0;
   uint64_t buf[(12*1024*128)/64 + 0x800];

   while (1)
   {
      bool dst_empty = mAxis.empty ();

      if (!dst_empty)
      {
         AxisOut_t  dst = mAxis.read  ();
         uint64_t  data = dst.data;

         if ( (nbuf & 0x7) == 0) std::cout << "Decode[" << std::setfill (' ') << std::hex << std::setw (5) << nbuf << "] = ";
         std::cout << ' ' << std::setfill ('0') << std::hex << std::setw (16) << data;
         if ((nbuf & 0x7) == 7) std::cout << std::endl;

         buf[nbuf] = data;
         nbuf++;
      }
      else
      {
         break;
      }
   }

   if (nbuf & 0x7) std::cout << std::endl;

   #if  0 && DECODE
   Toc toc;
   toc_decode (&toc, buf, nbuf);

   uint64_t headers[6*1024];
   uint32_t status;
   int n = header_decode (headers, &status,  buf, nbuf);

   int position = n << 6;
   for (int ichan = 0; ichan < toc.m_nchannels; ichan ++)
   {
      std::cout << "Position[" << std::setfill (' ') << std::hex << std::setw (4) << ichan
                << "] = " << std::hex << std::setw (8) << position << ':' << toc.m_offsets[ichan]
                << std::endl;
      position = chan_decode (buf, nbuf, position);
   }
   #endif

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int toc_decode (Toc *toc, uint64_t const *buf, int nbuf)
{
   uint64_t const *tlrPtr = buf + nbuf - 1 - 2;
   uint64_t           tlr = *tlrPtr;

   // Pack the trailer
    // Rsvd | #Channels-1 | #Samples-1 | Layout | Len64 | RecType | TlrFmt
    //  12  |         12  |         12 |      4 |    16 |       4 |      4
    //  52  |         40  |         28 |     24 |     8 |       4 |      0
   int recType      = ((tlr >>  4) &    0xf);
   toc->m_n64       = ((tlr >>  8) & 0xffff);
   toc->m_nsamples  = ((tlr >> 28) &  0xfff) + 1;
   toc->m_nchannels = ((tlr >> 40) &  0xfff) + 1;
   toc->m_offsets   = reinterpret_cast<decltype(toc->m_offsets)>(tlrPtr - toc->m_n64 + 1);

   return 0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int chan_decode (uint64_t const *buf, int nbuf, int position)
{
   BFU bfu;
   _bfu_put (bfu, buf[position>>6], position);


   int          nbins;
   int            sym;
   int        novrflw;
   uint16_t bins[128];
   int ovrpos = hist_decode (bins, &nbins, &sym, &novrflw, bfu, buf);

   uint16_t table[130];
   hist_integrate (table, bins, nbins);


   uint16_t syms[1024];
   position = sym_decode (syms, table, sym, bfu, buf, ovrpos, novrflw);

   std::cout << "Total Bits " << std::setfill (' ') << std::setw (8) <<  std::hex
             << position << ':' << ((position + 63 )>> 6) << "  " << nbuf << std::endl;


   return position;
}
/* ---------------------------------------------------------------------- */


static int   header_decode (uint64_t *headers, uint32_t *status, uint64_t const *buf, int nbuf)
{
   // -------------------------------------------------------------------------------
   // Compose the record header word
   //    status(32) | exception count(8) | length (16) | RecType(4) | HeaderFormat(4)
   // -------------------------------------------------------------------------------
   uint64_t recHdr = buf[0];
   int      hdrFmt = (recHdr >>  0) & 0xf;
   int      recTyp = (recHdr >>  4) & 0xf;
   int      recLen = (recHdr >>  8) & 0xffff;
   int      excCnt = (recHdr >> 24) & 0xff;
           *status = (recHdr >> 32);

   uint16_t const *excBuf = reinterpret_cast<decltype(excBuf)>(buf + 1);
   uint64_t const *hdrBuf = reinterpret_cast<decltype(hdrBuf)>(excBuf) + excCnt;
   uint64_t        lastts = buf[recLen - 1];

   int nhdrs   = 6;
   uint64_t prv[6];
   headers[0] = prv[0] = *hdrBuf++;
   headers[1] = prv[1] = *hdrBuf++;
   headers[2] = prv[2] = *hdrBuf++;
   headers[3] = prv[3] = *hdrBuf++;
   headers[4] = prv[4] = *hdrBuf++;
   headers[5] = prv[5] = *hdrBuf++;
   lastts     =          *hdrBuf++;

   std::cout << "Header Record:"
             <<               "Format   = " << std::setw ( 8) << std::hex <<  hdrFmt    << std::endl
             << "              RecType  = " << std::setw ( 8) << std::hex <<  recTyp    << std::endl
             << "              RecLen   = " << std::setw ( 8) << std::hex <<  recLen    << std::endl
             << "              ExcCnt   = " << std::setw ( 8) << std::hex <<  excCnt    << std::endl
             << "              Status   = " << std::setw ( 8) << std::hex << *status    << std::endl
             << "              Wib0     = " << std::setw (16) << std::hex <<  headers[0]<< std::endl
             << "              Wib01    = " << std::setw (16) << std::hex <<  headers[1]<< std::endl
             << "              Cd00     = " << std::setw (16) << std::hex <<  headers[2]<< std::endl
             << "              Cd01     = " << std::setw (16) << std::hex <<  headers[3]<< std::endl
             << "              Cd10     = " << std::setw (16) << std::hex <<  headers[4]<< std::endl
             << "              Cd11     = " << std::setw (16) << std::hex <<  headers[5]<< std::endl
             << "              LastTs   = " << std::setw (16) << std::hex <<  lastts    << std::endl;


   // Decode the exception frames
   for (int idx = 0; idx <= 4*excCnt; idx++)
   {
      uint16_t exception = *excBuf++;

      if (exception == 0) break;

      uint16_t     frame = exception & 0xffff;
      uint16_t      mask = exception >> 10;
      uint64_t       hdr;

      // Header word 0, all static fields
      if (mask &  0x1) hdr = *hdrBuf++;
      else             hdr =  prv[0];
      headers[nhdrs++]     =  prv[0] = hdr;


      // Header word 1, Timestamp, increment by 25 counts
      if (mask &  0x2) hdr = *hdrBuf++;
      else             hdr =  prv[1] + 25;
      headers[nhdrs++]     =  prv[1] = hdr;


      // Header word 2, Cold data convert count, increment by 1
      if (mask &  0x4) hdr = *hdrBuf++;
      else             hdr =  prv[2] + (1LL << 48);
      headers[nhdrs++]     =  prv[2] = hdr;


      // Header word 3, Cold data static fields
      if (mask &  0x8) hdr = *hdrBuf++;
      else             hdr = prv[3];
      headers[nhdrs++]     = prv[3] = hdr;


      // Header word 4, Cold data convert count, increment by 1
      if (mask & 0x10) hdr = *hdrBuf++;
      else             hdr = prv[4] + (1LL << 48);
      headers[nhdrs++]     = prv[4] = hdr;


      // Header word 4, Cold data static fields
      if (mask & 0x20) hdr = *hdrBuf++;
      else             hdr = prv[5];
      headers[nhdrs++]     = prv[5] = hdr;
   }

   return recLen;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int hist_decode (uint16_t      *bins,
                        int         *nrbins,
                        int          *first,
                        int        *novrflw,
                        BFU            &bfu,
                        uint64_t const *buf)
{
   bool debug = true;

   int64_t  m01s;
   int left      = PACKET_K_NSAMPLES - 1;
   int position = _bfu_get_pos  (bfu);
   int format   = _bfu_extractR (bfu, buf, position,  4);
   int nbins    = _bfu_extractR (bfu, buf, position,  8) + 1;
   int mbits    = _bfu_extractR (bfu, buf, position,  4);
   int nbits    = mbits;


   // Return decoding context
  *first        = _bfu_extractR (bfu, buf, position, 12);
  *novrflw      = _bfu_extractR (bfu, buf, position,  4);
  *nrbins       = nbins;


   // Extract the histogram bins
   for (int ibin = 0; ibin < nbins; ibin++)
   {
      // Extract the bits
      int cnts   = left ? _bfu_extractR (bfu, buf, position, nbits) : 0;

      bins[ibin] =  cnts;
      if ( (ibin & 0xf) == 0x0) std::cout << "Hist[" << std::hex << std::setw(2) << ibin << "] ";
      std::cout <<  ' ' << std::hex << std::setw(3) << cnts  << ":" << std::hex << std::setw(1) << nbits;
      if ( (ibin & 0xf) == 0xf) std::cout << std::endl;

      left -= cnts;

      nbits = 32 - __builtin_clz (left);
      if (nbits > mbits) nbits = mbits;
   }

   return position;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void hist_integrate (uint16_t *table, uint16_t *bins, int nbins)
{
   uint16_t total  = 0;
   table[0]        = nbins;

   for (int idx = 0; idx < nbins; idx++)
   {
      table[idx+1] = total;
      total       += bins[idx];
   }

   table[nbins + 1] = total;

   return;
}
/* ---------------------------------------------------------------------- */

#include "AP-Decode.h"

/* ---------------------------------------------------------------------- */
int sym_decode (uint16_t        *syms,
                uint16_t const *table,
                uint16_t          sym,
                BFU               bfu,
                uint64_t const   *buf,
                int            ovrpos,
                int           novrflw)
{
   APD_dtx dtx;

   int nbins  = table[0];  // Total number of bins
   int novr   = table[2];  // Number of overflows


   // Compute the number of bits in the overflow array
   int nobits = novrflw *   novr;
   int sympos = ovrpos +   nobits;

   APD_start (&dtx, buf, sympos);

   for (int idy = 0; ; idy++)
   {
      syms[idy] = sym;
      print_decoded (sym, idy);

      if (idy == PACKET_K_NSAMPLES -1)
      {
         break;
      }

      sym = APD_decode (&dtx, table);
      if (sym == 0)
      {
         // Have overflow
         int ovr = _bfu_extractR (bfu, buf, ovrpos, novrflw);
         sym     =  nbins + ovr;
      }
   }


   // Number of bits consumed by the decoding
   sympos = APD_finish (&dtx);

   // Total number of decoded symbol bits
   int totbits = sympos + ovrpos;
   return totbits;
}
/* ---------------------------------------------------------------------- */


static inline int16_t restore (uint16_t sym)
{
   if (sym & 1)  return -(sym >> 1);
   else          return  (sym >> 1);
}

static void print_decoded (uint16_t sym, int idy)
{
   static uint16_t Adcs[16];
   static uint16_t Prv;

   int idz = idy & 0xf;
   if (idz == 0) { std::cout << "Sym[" << std::setfill (' ') << std::hex << std::setw (4) << idy << "] "; }
   if (idy == 0) { Adcs[0]   = sym; }
   else          { Adcs[idz] = Adcs[(idz-1) & 0xf] + restore (sym); }


   std::cout << std::hex << std::setw (3) << sym;

   if (idz == 0xf)
   {
      std::cout << "  ";
      for (int i = 0; i < 16; i++)  std::cout << std::setfill (' ') << std::hex << std::setw (4) << Adcs[i];
      std::cout << std::endl;
   }
   ///std::cout << "Sym[" << idy << "] = " << syms[idy] << " : " << dsyms[idy] <<  std::hex << std::setw(5) << std::endl);

   return;
}


/* ====================================================================== */
/* DEPRECATED: Leftover from the waveform extraction testing              */
#if 0
/* ---------------------------------------------------------------------- */
#define CHANNELS_IN_FILE 256

/* ---------------------------------------------------------------------- *//*!

   \brief Transposes a ntimes x nchannels array

   \param[in]   samples  The transposed array;
   \param[in]      adcs  The source array
   \param[in]    ntimes  The number of rows in \a adcs
   \param[in] nchannels  The number of columns in \a adcs
                                                                          */
/* ---------------------------------------------------------------------- */
static void  transpose (uint16_t       *samples,
                        uint16_t const    *adcs,
                        unsigned int     ntimes,
                        unsigned int  nchannels)
{
   // Advance the output sample column for each new time
   for (unsigned int itimes = 0; itimes < ntimes; itimes++, samples++)
   {
      for (unsigned int ichn = 0; ichn < nchannels; ichn++)
      {
         // Page down to the correct row
         samples[ntimes * ichn] = *adcs++;
      }
   }

   return;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct Timesample
 *  \brief  The record structure of a time sample in the input file
 *
 *  This is dependent of the file, not how many channels that are being
 *  processed.  That value is defined as CHANNEL_PER_SAMPLE and can be
 *  set to anything from 4 to 128 in multiples of 4.
 *
 \* ---------------------------------------------------------------------- */
struct Timesample
{
        uint64_t             timestamp;
        uint16_t adcs[CHANNELS_IN_FILE];
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct Waveform
 *  \brief  Describes a simulated waveform
 *
 *  \par
 *   This is a very simplistic waveform. It is simply a ramp the goes up
 *   then comes down.
 *
\* ---------------------------------------------------------------------- */
struct Waveform
{
   int        start;   /*!< Stating time                                  */
   int          end;   /*!< Ending  time                                  */
   int          adc;   /*!< The value of the ADC                          */
};
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \struct Signal
 *  \brief  Defines a signal as any number of waveforms with the pedestal
 *          between them
\* ---------------------------------------------------------------------- */
struct Signal
{
   ModuleConfig const *cfgs;
   uint32_t           chans;
   int                  cnt;
   Waveform            *wfs;
};
/* ---------------------------------------------------------------------- */

#include <stdlib.h>
int noise ()
{
   uint64_t x = rand ();
   x += rand ();
   x += rand ();
   x += rand ();
   x += rand ();
   x += rand ();
   x  = 6 * x / (RAND_MAX);
   return x;
}

/* ----------------------------------------------------------------------- */
static void fill_signal (void *prm, Timesample *sample, int which)
{
   Signal const       *signal = reinterpret_cast<Signal const *>(prm);
   int               channels = signal->chans;
   Waveform const        *wfs = signal->wfs;
   int                    adc = 0;

   sample->timestamp = which;

   if (which & 0x400)
   {
      //channels |= (1 << 31);
   }
   else
   {
      channels = 0;
   }


   which &= 0x3ff;
   for (int idx = 0; idx < signal->cnt; idx++, wfs++)
   {
      int  start = wfs->start;
      if (which < start)
      {
         break;
      }

      int  end   = wfs->end;
      if (which <= end)
      {
         adc = wfs->adc; break;
      }
   }

   uint16_t pedestal = 0x800;
   for (int idx = 0; idx < MODULE_K_NCHANNELS; idx++)
   {
      if ((1 << idx) & channels)
      {
          sample->adcs[idx] = pedestal + adc;
          if (adc) printf ("sample[%4u].adcs[%2u] = %3.3x\n", which, idx, sample->adcs[idx]);

      }
      else
      {
          sample->adcs[idx] = pedestal + noise ();
      }

   }

   return;
}
/* ----------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class DataStream
{
public:
   typedef void (*Filler) (void *prm, Timesample *sample, int idx);
public:
   DataStream (std::ifstream &in);
   DataStream (int cnt, Filler filler, void *prm);
  ~DataStream () { free (const_cast<Timesample *>(m_beg_sample)); }

public:
   Timesample const *get ();

private:
   Timesample const *m_cur_sample;
   Timesample const *m_beg_sample;
   Timesample const *m_end_sample;
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
DataStream::DataStream (int cnt, Filler filler, void *prm)
{
   int         nbytes = cnt * sizeof (Timesample);
   Timesample *sample = reinterpret_cast<Timesample *>(malloc (nbytes));

   m_beg_sample = sample;
   m_cur_sample = sample;
  for (int idx = 0; idx < cnt; idx++)
  {
     filler (prm, sample, idx);
     sample += 1;
  }

  m_end_sample = sample;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
DataStream::DataStream  (std::ifstream &in)
{
   int in_cur = in.tellg ();
   in.seekg (0, std::ios_base::end);

   int in_end = in.tellg ();
   in.seekg (in_cur, std::ios_base::beg);

   int nbytes = in_end - in_cur;
   char *data = reinterpret_cast<char *>(malloc (nbytes));
   in.read  (data, nbytes);

   m_beg_sample = reinterpret_cast<Timesample const *>(data);
   m_end_sample = reinterpret_cast<Timesample const *>(data + nbytes);
   m_cur_sample = m_beg_sample;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
inline Timesample const *DataStream::get ()
{
   Timesample const *cur = m_cur_sample;

   // Advance to the next,
   // If off the end, reset to the beginning
   m_cur_sample += 1;
   if (m_cur_sample >= m_end_sample)
       m_cur_sample = m_beg_sample;

   return cur;
}
/* ---------------------------------------------------------------------- */




int main (int argc, char *const argv[])
{
#  define NSLICES 4

   AxisIn                           sAxis;
   AxisOut                          mAxis;
   ModuleIdx_t              moduleIdx = 3;
   ModuleConfig                    config;
   MonitorModule                    status;

   static Waveform wfs[1] = { { 100, 110, 0x210 } };

   static int     nwfs    =  1;  ///sizeof (wfs) / sizeof (wfs[0])
   static Signal signal = {&config, 0x2, 1, wfs };

   sAxis[0].data  = 0;

   // Create NSLICES of fake data
   DataStream ds (NSLICES * PACKET_K_NSAMPLES, fill_signal, &signal);


   // This is suppose to do the configuration
   config.init = true;
   DuneDataCompressionCore(sAxis, mAxis, moduleIdx, config, status);
   config.init = false;

   /*
   int fd          = create_file ("/tmp/dune.dat");
   int channel_cnt = MODULE_K_NCHANNELS;
   int beg         = 0;
   int end         = beg + channel_cnt - 1;
   */

   Pidx_t     pidx = 1;
   for (int islice = 0; islice < NSLICES; islice++)
   {
      printf ("Slice = %d\n", islice);

      // Process each slice of the time samples
      for (int itime = 0; itime < NOVA_K_SCALE * PACKET_K_NSAMPLES; itime += NOVA_K_SCALE)
      {
         bool       output;
         Timesample const *sample = ds.get ();


         fill_frame (sAxis, sample->timestamp << NOVA_V_SCALE, sample->adcs);
         output = DuneDataCompressionCore(sAxis, mAxis, moduleIdx, config, status);

         if (output)
         {
            //write_packet   (fd, mAxis);
            //print_monitor  (monitor);
            //print_packet   (mAxis);
            //SIMULATION (print_internal (pframe, beg, end);)
         }
      }

      pidx ^= 1;
   }

   puts ("Success: Well let's not be to hasty, not ready for this yet");
   return 0;
}
/* ---------------------------------------------------------------------- */
#endif

