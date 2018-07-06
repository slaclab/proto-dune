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



int NPackets = 1;

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

   // Get the number of 1024 packets to process
   if (argc > 1)
   {
      NPackets = strtoul (argv[1], 0, 0);
   }

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
   for (int ipacket = 0; ipacket < NPackets; ipacket++)
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

      printf (" %d\n", ipacket);
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

   for (int ipacket = 0; ipacket < NPackets; ipacket++)
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
      printf ("NEXT %d\n", ipacket);
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


static const int RecordLen = 462;
static const uint64_t Record[462] =
{
  0x0010102000000813 ,0x00000000000001bc ,0x0000000000000000 ,0xaa5c000000000000
 ,0x0000000000000000 ,0xaa5c000000000000 ,0x0000000000000000 ,0x0101808018180101
 ,0x01fa8000002047f7 ,0xd1a522ee92a63c02 ,0xf85b8d30799ae8c6 ,0x00fd4000001023fb
 ,0xe8d2917749531e01 ,0x7c2dc6983ccd7463 ,0x007ea000000811fd ,0xf46948bba4a98f00
 ,0xbe16e34c1e66ba31 ,0x803f5000000408fe ,0x7a34a45dd254c780 ,0x5f0b71a60f335d18
 ,0xc01fa8000002047f ,0xbd1a522ee92a63c0 ,0x2f85b8d30799ae8c ,0x600fd4000001023f
 ,0x5e8d2917749531e0 ,0x17c2dc6983ccd746 ,0x3007ea000000811f ,0x2f46948bba4a98f0
 ,0x0be16e34c1e66ba3 ,0x1803f5000000408f ,0x17a34a45dd254c78 ,0x05f0b71a60f335d1
 ,0x8c01fa8000002047 ,0x8bd1a522ee92a63c ,0x02f85b8d30799ae8 ,0xc600fd4000001023
 ,0xc5e8d2917749531e ,0x017c2dc6983ccd74 ,0x63007ea000000811 ,0xe2f46948bba4a98f
 ,0x00be16e34c1e66ba ,0x31803f5000000408 ,0x717a34a45dd254c7 ,0x805f0b71a60f335d
 ,0x18c01fa800000204 ,0xb8bd1a522ee92a63 ,0xc02f85b8d30799ae ,0x8c600fd400000102
 ,0x5c5e8d2917749531 ,0xe017c2dc6983ccd7 ,0x463007ea00000081 ,0x2e2f46948bba4a98
 ,0xf00be16e34c1e66b ,0xa31803f500000040 ,0x9717a34a45dd254c ,0x7805f0b71a60f335
 ,0xd18c01fa80000020 ,0x4b8bd1a522ee92a6 ,0x3c02f85b8d30799a ,0xe8c600fd40000010
 ,0x25c5e8d291774953 ,0x1e017c2dc6983ccd ,0x7463007ea0000008 ,0x92e2f46948bba4a9
 ,0x8f00be16e34c1e66 ,0xba31803f50000004 ,0xc9717a34a45dd254 ,0xc7805f0b71a60f33
 ,0x5d18c01fa8000002 ,0xe4b8bd1a522ee92a ,0x63c02f85b8d30799 ,0xae8c600fd4000001
 ,0x725c5e8d29177495 ,0x31e017c2dc6983cc ,0xd7463007ea000000 ,0xb92e2f46948bba4a
 ,0x98f00be16e34c1e6 ,0x6ba31803f5000000 ,0xdc9717a34a45dd25 ,0x4c7805f0b71a60f3
 ,0x35d18c01fa800000 ,0x6e4b8bd1a522ee92 ,0xa63c02f85b8d3079 ,0x9ae8c600fd400000
 ,0x3725c5e8d2917749 ,0x531e017c2dc6983c ,0xcd7463007ea00000 ,0x1b92e2f46948bba4
 ,0xa98f00be16e34c1e ,0x66ba31803f500000 ,0x0dc9717a34a45dd2 ,0x54c7805f0b71a60f
 ,0x335d18c01fa80000 ,0x06e4b8bd1a522ee9 ,0x2a63c02f85b8d307 ,0x99ae8c600fd40000
 ,0x83725c5e8d291774 ,0x9531e017c2dc6983 ,0xccd7463007ea0000 ,0x41b92e2f46948bba
 ,0x4a98f00be16e34c1 ,0xe66ba31803f50000 ,0xa0dc9717a34a45dd ,0x254c7805f0b71a60
 ,0xf335d18c01fa8000 ,0x506e4b8bd1a522ee ,0x92a63c02f85b8d30 ,0x799ae8c600fd4000
 ,0x283725c5e8d29177 ,0x49531e017c2dc698 ,0x3ccd7463007ea000 ,0x941b92e2f46948bb
 ,0xa4a98f00be16e34c ,0x1e66ba31803f5000 ,0xca0dc9717a34a45d ,0xd254c7805f0b71a6
 ,0x0f335d18c01fa800 ,0x6506e4b8bd1a522e ,0xe92a63c02f85b8d3 ,0x0799ae8c600fd400
 ,0x3283725c5e8d2917 ,0x749531e017c2dc69 ,0x83ccd7463007ea00 ,0x1941b92e2f46948b
 ,0xba4a98f00be16e34 ,0xc1e66ba31803f500 ,0x0ca0dc9717a34a45 ,0xdd254c7805f0b71a
 ,0x60f335d18c01fa80 ,0x86506e4b8bd1a522 ,0xee92a63c02f85b8d ,0x30799ae8c600fd40
 ,0x43283725c5e8d291 ,0x7749531e017c2dc6 ,0x983ccd7463007ea0 ,0x21941b92e2f46948
 ,0xbba4a98f00be16e3 ,0x4c1e66ba31803f50 ,0x90ca0dc9717a34a4 ,0x5dd254c7805f0b71
 ,0xa60f335d18c01fa8 ,0x486506e4b8bd1a52 ,0x2ee92a63c02f85b8 ,0xd30799ae8c600fd4
 ,0xa43283725c5e8d29 ,0x17749531e017c2dc ,0x6983ccd7463007ea ,0x521941b92e2f4694
 ,0x8bba4a98f00be16e ,0x34c1e66ba31803f5 ,0xa90ca0dc9717a34a ,0x45dd254c7805f0b7
 ,0x1a60f335d18c01fa ,0xd486506e4b8bd1a5 ,0x22ee92a63c02f85b ,0x8d30799ae8c600fd
 ,0xea43283725c5e8d2 ,0x917749531e017c2d ,0xc6983ccd7463007e ,0x7521941b92e2f469
 ,0x48bba4a98f00be16 ,0xe34c1e66ba31803f ,0x3a90ca0dc9717a34 ,0xa45dd254c7805f0b
 ,0x71a60f335d18c01f ,0x9d486506e4b8bd1a ,0x522ee92a63c02f85 ,0xb8d30799ae8c600f
 ,0xcea43283725c5e8d ,0x2917749531e017c2 ,0xdc6983ccd7463007 ,0xe7521941b92e2f46
 ,0x948bba4a98f00be1 ,0x6e34c1e66ba31803 ,0x73a90ca0dc9717a3 ,0x4a45dd254c7805f0
 ,0xb71a60f335d18c01 ,0x39d486506e4b8bd1 ,0xa522ee92a63c02f8 ,0x5b8d30799ae8c600
 ,0x1cea43283725c5e8 ,0xd2917749531e017c ,0x2dc6983ccd746300 ,0x0e7521941b92e2f4
 ,0x6948bba4a98f00be ,0x16e34c1e66ba3180 ,0x3f5000000408fefd ,0x34a45dd254c7805f
 ,0x0b71a60f335d18c0 ,0x1fa8000002047f7e ,0x1a522ee92a63c02f ,0x85b8d30799ae8c60
 ,0x0fd4000001023fbf ,0x8d2917749531e017 ,0xc2dc6983ccd74630 ,0x07ea000000811fdf
 ,0x46948bba4a98f00b ,0xe16e34c1e66ba318 ,0x03f5000000408fef ,0xa34a45dd254c7805
 ,0xf0b71a60f335d18c ,0x01fa8000002047f7 ,0xd1a522ee92a63c02 ,0xf85b8d30799ae8c6
 ,0x00fd4000001023fb ,0xe8d2917749531e01 ,0x7c2dc6983ccd7463 ,0x007ea000000811fd
 ,0xf46948bba4a98f00 ,0xbe16e34c1e66ba31 ,0x803f5000000408fe ,0x7a34a45dd254c780
 ,0x5f0b71a60f335d18 ,0xc01fa8000002047f ,0xbd1a522ee92a63c0 ,0x2f85b8d30799ae8c
 ,0x600fd4000001023f ,0x5e8d2917749531e0 ,0x17c2dc6983ccd746 ,0x3007ea000000811f
 ,0x2f46948bba4a98f0 ,0x0be16e34c1e66ba3 ,0x1803f5000000408f ,0x17a34a45dd254c78
 ,0x05f0b71a60f335d1 ,0x8c01fa8000002047 ,0x8bd1a522ee92a63c ,0x02f85b8d30799ae8
 ,0xc600fd4000001023 ,0xc5e8d2917749531e ,0x017c2dc6983ccd74 ,0x63007ea000000811
 ,0xe2f46948bba4a98f ,0x00be16e34c1e66ba ,0x31803f5000000408 ,0x717a34a45dd254c7
 ,0x805f0b71a60f335d ,0x18c01fa800000204 ,0xb8bd1a522ee92a63 ,0xc02f85b8d30799ae
 ,0x8c600fd400000102 ,0x5c5e8d2917749531 ,0xe017c2dc6983ccd7 ,0x463007ea00000081
 ,0x2e2f46948bba4a98 ,0xf00be16e34c1e66b ,0xa31803f500000040 ,0x9717a34a45dd254c
 ,0x7805f0b71a60f335 ,0xd18c01fa80000020 ,0x4b8bd1a522ee92a6 ,0x3c02f85b8d30799a
 ,0xe8c600fd40000010 ,0x25c5e8d291774953 ,0x1e017c2dc6983ccd ,0x7463007ea0000008
 ,0x92e2f46948bba4a9 ,0x8f00be16e34c1e66 ,0xba31803f50000004 ,0xc9717a34a45dd254
 ,0xc7805f0b71a60f33 ,0x5d18c01fa8000002 ,0xe4b8bd1a522ee92a ,0x63c02f85b8d30799
 ,0xae8c600fd4000001 ,0x725c5e8d29177495 ,0x31e017c2dc6983cc ,0xd7463007ea000000
 ,0xb92e2f46948bba4a ,0x98f00be16e34c1e6 ,0x6ba31803f5000000 ,0xdc9717a34a45dd25
 ,0x4c7805f0b71a60f3 ,0x35d18c01fa800000 ,0x6e4b8bd1a522ee92 ,0xa63c02f85b8d3079
 ,0x9ae8c600fd400000 ,0x3725c5e8d2917749 ,0x531e017c2dc6983c ,0xcd7463007ea00000
 ,0x1b92e2f46948bba4 ,0xa98f00be16e34c1e ,0x66ba31803f500000 ,0x0dc9717a34a45dd2
 ,0x54c7805f0b71a60f ,0x335d18c01fa80000 ,0x06e4b8bd1a522ee9 ,0x2a63c02f85b8d307
 ,0x99ae8c600fd40000 ,0x83725c5e8d291774 ,0x9531e017c2dc6983 ,0xccd7463007ea0000
 ,0x41b92e2f46948bba ,0x4a98f00be16e34c1 ,0xe66ba31803f50000 ,0xa0dc9717a34a45dd
 ,0x254c7805f0b71a60 ,0xf335d18c01fa8000 ,0x506e4b8bd1a522ee ,0x92a63c02f85b8d30
 ,0x799ae8c600fd4000 ,0x283725c5e8d29177 ,0x49531e017c2dc698 ,0x3ccd7463007ea000
 ,0x941b92e2f46948bb ,0xa4a98f00be16e34c ,0x1e66ba31803f5000 ,0xca0dc9717a34a45d
 ,0xd254c7805f0b71a6 ,0x0f335d18c01fa800 ,0x6506e4b8bd1a522e ,0xe92a63c02f85b8d3
 ,0x0799ae8c600fd400 ,0x3283725c5e8d2917 ,0x749531e017c2dc69 ,0x83ccd7463007ea00
 ,0x1941b92e2f46948b ,0xba4a98f00be16e34 ,0xc1e66ba31803f500 ,0x0ca0dc9717a34a45
 ,0xdd254c7805f0b71a ,0x60f335d18c01fa80 ,0x86506e4b8bd1a522 ,0xee92a63c02f85b8d
 ,0x30799ae8c600fd40 ,0x43283725c5e8d291 ,0x7749531e017c2dc6 ,0x983ccd7463007ea0
 ,0x21941b92e2f46948 ,0xbba4a98f00be16e3 ,0x4c1e66ba31803f50 ,0x90ca0dc9717a34a4
 ,0x5dd254c7805f0b71 ,0xa60f335d18c01fa8 ,0x486506e4b8bd1a52 ,0x2ee92a63c02f85b8
 ,0xd30799ae8c600fd4 ,0xa43283725c5e8d29 ,0x17749531e017c2dc ,0x6983ccd7463007ea
 ,0x521941b92e2f4694 ,0x8bba4a98f00be16e ,0x34c1e66ba31803f5 ,0xa90ca0dc9717a34a
 ,0x45dd254c7805f0b7 ,0x1a60f335d18c01fa ,0xd486506e4b8bd1a5 ,0x22ee92a63c02f85b
 ,0x8d30799ae8c600fd ,0xea43283725c5e8d2 ,0x917749531e017c2d ,0xc6983ccd7463007e
 ,0x7521941b92e2f469 ,0x48bba4a98f00be16 ,0xe34c1e66ba31803f ,0x3a90ca0dc9717a34
 ,0xa45dd254c7805f0b ,0x71a60f335d18c01f ,0x9d486506e4b8bd1a ,0x522ee92a63c02f85
 ,0xb8d30799ae8c600f ,0xcea43283725c5e8d ,0x2917749531e017c2 ,0xdc6983ccd7463007
 ,0xe7521941b92e2f46 ,0x948bba4a98f00be1 ,0x6e34c1e66ba31803 ,0x73a90ca0dc9717a3
 ,0x4a45dd254c7805f0 ,0xb71a60f335d18c01 ,0x39d486506e4b8bd1 ,0xa522ee92a63c02f8
 ,0x5b8d30799ae8c600 ,0x1cea43283725c5e8 ,0xd2917749531e017c ,0x2dc6983ccd746300
 ,0x0e7521941b92e2f4 ,0x6948bba4a98f00be ,0x16e34c1e66ba3180 ,0x3f5000000408fefd
 ,0x34a45dd254c7805f ,0x0b71a60f335d18c0 ,0x1fa8000002047f7e ,0x1a522ee92a63c02f
 ,0x85b8d30799ae8c60 ,0x0fd4000001023fbf ,0x8d2917749531e017 ,0xc2dc6983ccd74630
 ,0x07ea000000811fdf ,0x46948bba4a98f00b ,0xe16e34c1e66ba318 ,0x03f5000000408fef
 ,0xa34a45dd254c7805 ,0xf0b71a60f335d18c ,0x0000084100000200 ,0x000014c300000e82
 ,0x0000214500001b04 ,0x00002dc700002786 ,0x00003a4900003408 ,0x000046cb0000408a
 ,0x0000534d00004d0c ,0x00005fcf0000598e ,0x00006c5100006610 ,0x000078d300007292
 ,0x0000855500007f14 ,0x000091d700008b96 ,0x00009e5900009818 ,0x0000aadb0000a49a
 ,0x0000b75d0000b11c ,0x0000c3df0000bd9e ,0x0000d0610000ca20 ,0x0000dce30000d6a2
 ,0x0000e9650000e324 ,0x0000f5e70000efa6 ,0x000102690000fc28 ,0x00010eeb000108aa
 ,0x00011b6d0001152c ,0x000127ef000121ae ,0x0001347100012e30 ,0x000140f300013ab2
 ,0x00014d7500014734 ,0x000159f7000153b6 ,0x0001667900016038 ,0x000172fb00016cba
 ,0x00017f7d0001793c ,0x00018bff000185be ,0x0001988100019240 ,0x0001a50300019ec2
 ,0x0001b1850001ab44 ,0x0001be070001b7c6 ,0x0001ca890001c448 ,0x0001d70b0001d0ca
 ,0x0001e38d0001dd4c ,0x0001f00f0001e9ce ,0x0001fc910001f650 ,0x00020913000202d2
 ,0x0002159500020f54 ,0x0002221700021bd6 ,0x00022e9900022858 ,0x00023b1b000234da
 ,0x0002479d0002415c ,0x0002541f00024dde ,0x000260a100025a60 ,0x00026d23000266e2
 ,0x000279a500027364 ,0x0002862700027fe6 ,0x000292a900028c68 ,0x00029f2b000298ea
 ,0x0002abad0002a56c ,0x0002b82f0002b1ee ,0x0002c4b10002be70 ,0x0002d1330002caf2
 ,0x0002ddb50002d774 ,0x0002ea370002e3f6 ,0x0002f6b90002f078 ,0x0003033b0002fcfa
 ,0x00030fbd0003097c ,0x00031c3f000315fe ,0x0000000000032280 ,0x00007f3ff0004223
 ,0x0010102013006670 ,0x708b309e01000000
};


#define DECODE 1

struct Toc
{
   uint16_t            m_n64; /*!< Number of 64 bit words                 */
   uint16_t       m_nsamples; /*!< Number of samples                      */
   uint16_t      m_nchannels; /*!< Number of channels                     */
   uint32_t const *m_offsets; /*!< Pointer to the channel offsets         */
};

static int          decode (uint64_t const *buf, int nbuf);
static int      toc_decode (Toc      *toc,                       uint64_t const *buf, int nbuf);
static int   header_decode (uint64_t *headers, uint32_t *status, uint64_t const *buf, int nbuf);
static int     chan_decode (uint64_t const *buf, int nbuf, int position, bool printit);
static int     hist_decode (uint16_t *bins, int *nrbins, int *first, int *novrflw, BFU &bfu, uint64_t const *buf, bool printit);
static void hist_integrate (uint16_t *table,uint16_t *bins, int nbins);
static int      sym_decode (uint16_t *syms, uint16_t const *table, uint16_t sym, BFU bfu, uint64_t const *buf, int ovrpos, int novrflw, bool printit);
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

   decode (Record, RecordLen);

   while (1)
   {
      bool dst_empty = mAxis.empty ();

      if (!dst_empty)
      {
         AxisOut_t  dst = mAxis.read  ();
         uint64_t  data = dst.data;

         ///if ( (nbuf & 0x7) == 0) std::cout << "Decode[" << std::setfill (' ') << std::hex << std::setw (5) << nbuf << "] = ";
         ///std::cout << ' ' << std::setfill ('0') << std::hex << std::setw (16) << data;
         ///if ((nbuf & 0x7) == 7) std::cout << std::endl;

         buf[nbuf] = data;
         nbuf++;
      }
      else
      {
         break;
      }
   }

   /// if (nbuf & 0x7) std::cout << std::endl;

   decode (buf, nbuf);

}
/* ---------------------------------------------------------------------- */

static int decode (uint64_t const *buf, int nbuf)
{
   // Print the first 32 64-bit words
    int idx;
    for (idx = 0; idx < 32; idx++)
    {
       if ( (idx & 0x7) == 0) std::cout << "Decode[" << std::setfill (' ') << std::hex << std::setw (5) << idx << "] = ";
       std::cout << ' ' << std::setfill ('0') << std::hex << std::setw (16) << buf[idx];
       if ((idx & 0x7) == 7) std::cout << std::endl;
    }

    std::cout << "...... skipping" << std::endl;

    // Print the last 256 64-bit words
    int beg = (nbuf > 256) ? ((nbuf - 32*8) & ~0x7) : idx;
    for (idx = beg; idx < nbuf; idx++)
    {
       if ( (idx & 0x7) == 0) std::cout << "Decode[" << std::setfill (' ') << std::hex << std::setw (5) << idx << "] = ";
       std::cout << ' ' << std::setfill ('0') << std::hex << std::setw (16) << buf[idx];
       if ((idx & 0x7) == 7) std::cout << std::endl;
    }


    if (nbuf & 0x7) std::cout << std::endl;


    #if  1 && DECODE
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

       bool printit = (ichan < 4);

       if (position != toc.m_offsets[ichan])
       {
          std::cout << "Error at chan " << ichan;
          ///// exit (-1);
       }

       position = chan_decode (buf, nbuf, position, printit);
    }
    #endif

    return 0;
}

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
static int chan_decode (uint64_t const *buf, int nbuf, int position, bool printit)
{
   BFU bfu;
   _bfu_put (bfu, buf[position>>6], position);


   int          nbins;
   int            sym;
   int        novrflw;
   uint16_t bins[128];
   int ovrpos = hist_decode (bins, &nbins, &sym, &novrflw, bfu, buf, printit);

   uint16_t table[130];
   hist_integrate (table, bins, nbins);


   uint16_t syms[1024];
   position = sym_decode (syms, table, sym, bfu, buf, ovrpos, novrflw, printit);

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
   lastts     =          *hdrBuf++;
   headers[2] = prv[2] = *hdrBuf++;
   headers[3] = prv[3] = *hdrBuf++;
   headers[4] = prv[4] = *hdrBuf++;
   headers[5] = prv[5] = *hdrBuf++;

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
                        uint64_t const *buf,
                        bool        printit)
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

      if (printit)
      {
         if ( (ibin & 0xf) == 0x0) std::cout << "Hist[" << std::hex << std::setw(2) << ibin << "] ";
         std::cout <<  ' ' << std::hex << std::setw(3) << cnts  << ":" << std::hex << std::setw(1) << nbits;
         if ( (ibin & 0xf) == 0xf) std::cout << std::endl;
      }

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
                int           novrflw,
                bool          printit)
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
      if (printit)
      {
         print_decoded (sym, idy);
      }

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

