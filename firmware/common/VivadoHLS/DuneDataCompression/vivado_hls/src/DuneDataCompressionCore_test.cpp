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
#include "DuneDataCompressionPacket.h"
#include "WibFrame.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>



#define CHANNELS_IN_FILE 256

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


static inline void fill_frame (AxisIn                             &axis,
                               uint64_t                            nova,
                               uint16_t  const adcs[MODULE_K_NCHANNELS]);


#if 0
int main (int argc, char *const argv[])
{
#  define NSLICES 4

   AxisIn                           sAxis;
   AxisOut                          mAxis;
   ModuleIdx_t              moduleIdx = 3;
   ModuleConfig                    config;
   ModuleStatus                    status;

   static Waveform wfs[1] = { { 100, 110, 0x210 } };

   static int     nwfs    =  1;  ///sizeof (wfs) / sizeof (wfs[0])
   static Signal signal = {&config, 0x2, 1, wfs };

   sAxis[0].data  = 0;

   // Create NSLICES of fake data
   DataStream ds (NSLICES*1024, fill_signal, &signal);


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

      // Process each slice of 1024 time samples
      for (int itime = 0; itime < NOVA_K_SCALE * 1024; itime += NOVA_K_SCALE)
      {
         bool       output;
         Timesample const *sample = ds.get ();


         fill_frame (sAxis, sample->timestamp << NOVA_V_SCALE, sample->adcs);
         output = DuneDataCompressionCore(sAxis, mAxis, moduleIdx, config, status);

         if (output)
         {
            //write_packet   (fd, mAxis);
            //print_status   (status);
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

static inline void commit (AxisIn &axis, uint64_t w, int user, int last)
{
   AxisIn_t in;
   in.data = w;
   in.keep = 0xff;
   in.strb = 0;
   in.user = user;
   in.last = last;
   in.id   = 0;
   in.dest = 0;

   axis.write (in);
   return;
}

static void fill_frame (AxisIn                            &axis,
                        uint64_t                      timestamp,
                        uint16_t  const adcs[MODULE_K_NCHANNELS],
                        int                           runEnable,
                        int                               flush);

static void fill_dummy_frame (AxisIn &axis, int first, int flush)
{
   int     iaxis = 0;
   int      last = (int)AxisLast::Eof;
   uint16_t data = 0xfff;


   uint16_t adcs[MODULE_K_NCHANNELS];

   for (int idx = 0; idx < MODULE_K_NCHANNELS; idx++)
   {
      adcs[idx] = 0xf00 | idx;
   }

   fill_frame (axis, 0, adcs, 0, flush);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *                                                                        */
/* ---------------------------------------------------------------------- */
static inline void fill_runDisable_frame (AxisIn &axis)
{
   int first = (int)AxisUserFirst::Sof;
   int flush = 0;
   fill_dummy_frame (axis, first, flush);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *                                                                        */
/* ---------------------------------------------------------------------- */
static inline void fill_flush_frame (AxisIn &axis)
{
   int first =  (int)AxisUserFirst::Sof
             |  (int)AxisUserFirst::RunEnable;
   int flush = (int)AxisUserLast::Flush;

   fill_dummy_frame (axis, first, flush);
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *                                                                        */
/* ---------------------------------------------------------------------- */
static inline void fill_runDisableAndFlush_frame (AxisIn &axis)
{
   int first = (int)AxisUserFirst::Sof;
   int flush = (int)AxisUserLast::Flush;

   fill_dummy_frame (axis, first, flush);
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *                                                                        */
/* ---------------------------------------------------------------------- */
static inline void fill_empty_frame (AxisIn   &axis,
                                     int  runEnable,
                                     int      flush)
{
   int first = (int)AxisUserFirst::Sof
             | (runEnable ? (int)AxisUserFirst::RunEnable : 0);
   flush     = flush ? (int)AxisUserLast::Flush : 0;

   fill_dummy_frame (axis, first, flush);
}
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *
 *  \param[out]      axis   The target AXI stream
 *  \param[ in]      nova   The nova timestamp + status bits
 *  \param[ in]       adc   The ADC value
 *  \param[ in] runEnable   The run enable flag, if 0, data is pitched
 *  \param[ in]     flush   Flush flag
 *                                                                        */
/* ---------------------------------------------------------------------- */
static void fill_frame (AxisIn                            &axis,
                        uint64_t                      timestamp,
                        uint16_t  const adcs[MODULE_K_NCHANNELS],
                        int                           runEnable,
                        int                               flush)
{
   static const WibFrame::Crate Crate (1);
   static const WibFrame::Slot  Slot  (2);
   static const WibFrame::Fiber Fiber (3);
   static int                   Phase (8);

   static const WibFrame::Id id = WibFrame::id (Crate, Slot, Fiber);
   int iaxis = 0;

   int first =              (int)AxisUserFirst::Sof
             | (runEnable ? (int)AxisUserFirst::RunEnable : 0);
   int last  = 0;
   flush  = flush ? (int)AxisUserLast::Flush : 0;

   if (flush) printf ("Nonzero flush = %d\n", flush);

   // Set the first and runEnable flags = 3
   uint64_t w0 = WibFrame::w0 (timestamp);
   commit (axis, w0, first, 0);

   // 125/2 = 64.5
   WibFrame::Timestamp ts (64*timestamp + timestamp/2 + Phase);
   uint64_t w1 = WibFrame::w1 (ts, id);
   commit (axis, w1, 0, 0);


   uint16_t const *in = (uint16_t const *)adcs;
   uint64_t  sdat  =  0;
   int       shift =  0;
   int        left = 64;
   int        user =  0;

   for (int icolddata = 0; icolddata < 2; icolddata++)
   {
      WibFrame::ColdData::ErrWord      err (0);
      WibFrame::ColdData::ChecksumWord csA (0);
      WibFrame::ColdData::ChecksumWord csB (0);
      uint64_t s0 = WibFrame::ColdData::s0 (0, 0, 0, timestamp);
      commit (axis, s0, 0, 0);


      WibFrame::ColdData::ErrReg errReg (0);
      WibFrame::ColdData::Hdrs     hdrs (0);
      uint64_t s1 = WibFrame::ColdData::s1 (errReg, hdrs);
      commit (axis, s1, 0, 0);


      int shift =  0;
      int left  = 64;
      for (int iadc = 0; iadc < 64; iadc++)
      {
         uint64_t adc = (uint64_t)*in++;

         sdat  |= adc << shift;
         //std::cout << "Adc = " << std::setw (3) << std::hex << adc
         //          << " sdat = " << std::setw(16) << std::hex << sdat
         //          << std::endl;
         if (left <= 12)
         {
            /// If on the last word...
            ///if (icolddata == 1 && iadc == 63) { user = flush;  last = (int)AxisLast::Eof; }
            ///if (user) printf ("Non-zero user = %d (last = %d)\n", user, last);
            commit (axis, sdat, user, last);

            std::cout << "WibIdx:" << std::setw (2) << iaxis
                      << "  "   << std::setw(16) << std::hex << sdat << std::endl;

            sdat   = adc >> left;
            shift  = 12 - left;
            left   = 64 - shift;
            iaxis += 1;
         }
         else
         {
            sdat  |= adc << shift;
            shift += 12;
            left  -= 12;
         }
      }
   }


   /*
   for (int idx = 0; idx < iaxis; idx++)
   {
      std::cout << "Idx:" << std::setw (2) << idx
                << "  "   << std::setw(16) << std::hex << axis[idx].data.VAL
                << std::endl;
   }
   */

   return;
}
/* ---------------------------------------------------------------------- */


#include <string.h>
#include <errno.h>
static void  transpose (uint16_t       *samples,
                        uint16_t const    *adcs,
                        unsigned int     ntimes,
                        unsigned int  nchannels);

static void  print (AxisOut &mAxis);

static bool differ (ModuleStatus const &s0,
                    ModuleStatus const &s1)
{
   // NOTE: types[0] is not checked.  This is the normal
   //       frame counter and would cause a difference
   //        every time.
   if (s0.common.pattern != s1.common.pattern) return true;
   if (s0.cfg.ncfgs      != s1.cfg.ncfgs     ) return true;
   if (s0.cfg.mode       != s1.cfg.mode      ) return true;
   if (s0.read.ntypes[1] != s1.read.ntypes[1]) return true;
   if (s0.read.ntypes[2] != s1.read.ntypes[2]) return true;
   if (s0.read.ntypes[3] != s1.read.ntypes[3]) return true;
   if (s0.write.nwrote   != s1.write.nwrote  ) return true;


   return false;
}

static void print_status (ModuleStatus const &s0,
                          ModuleStatus const &s1,
                          int            isample)
{
   printf ("New status at sample   : %6u\n"
         "          C ommon.pattern: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                 Cfg.mode: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                   .ncfgs: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         " Read.ntypes[     Normal]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[   Disabled]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[      Flush]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[DisAndFlush]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[    ErrSofM]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[    ErrSofU]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[    ErrEofM]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[    ErrEofU]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[    ErrEofE]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[   ErrK28_5]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "     .ntypes[     ErrSeq]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "          Write.npromoted: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                .ndropped: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                  .nwrote: %8.8" PRIx32 " -> %8.8" PRIx32 "\n",
            isample,
            s0.common.pattern, s1.common.pattern,
            s0.cfg.mode,       s1.cfg.mode,
            s0.cfg.ncfgs,      s1.cfg.ncfgs,
            s0.read.ntypes[static_cast<int>(StatusRead::Type::     Normal)], s1.read.ntypes[static_cast<int>(StatusRead::Type::     Normal)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::RunDisabled)], s1.read.ntypes[static_cast<int>(StatusRead::Type::RunDisabled)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::      Flush)], s1.read.ntypes[static_cast<int>(StatusRead::Type::      Flush)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::   DisFlush)], s1.read.ntypes[static_cast<int>(StatusRead::Type::   DisFlush)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::    ErrSofM)], s1.read.ntypes[static_cast<int>(StatusRead::Type::    ErrSofM)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::    ErrSofU)], s1.read.ntypes[static_cast<int>(StatusRead::Type::    ErrSofU)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::    ErrEofM)], s1.read.ntypes[static_cast<int>(StatusRead::Type::    ErrEofM)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::    ErrEofU)], s1.read.ntypes[static_cast<int>(StatusRead::Type::    ErrEofU)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::    ErrEofE)], s1.read.ntypes[static_cast<int>(StatusRead::Type::    ErrEofE)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::   ErrK28_5)], s1.read.ntypes[static_cast<int>(StatusRead::Type::   ErrK28_5)],
            s0.read.ntypes[static_cast<int>(StatusRead::Type::     ErrSeq)], s1.read.ntypes[static_cast<int>(StatusRead::Type::     ErrSeq)],
            s0.write.npromoted,s1.write.npromoted,
            s0.write.ndropped, s1.write.ndropped,
            s0.write.nwrote,   s1.write.nwrote);


      return;
}

class MyStreamIn : public AxisIn
{
public:
   MyStreamIn () :                 AxisIn ("input stream") { return; }
   MyStreamIn (const char *name) : AxisIn (name)           { return; }
  ~MyStreamIn ()         { puts ("Deleting input stream"); return; }
};

class MyStreamOut : public AxisOut
{
public:
   MyStreamOut () :                 AxisIn ("output stream") { return; }
   MyStreamOut (const char *name) : AxisIn (name)            { return; }
  ~MyStreamOut ()         { puts ("Deleting output stream"); return; }
};


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
#  define NSLICES 4


    MyStreamIn                sAxis ("In");
    MyStreamOut              mAxis ("Out");
    ModuleIdx_t              moduleIdx = 3;
    ModuleConfig                    config;
    ModuleStatus                    status;
    ModuleStatus               last_status;


   unsigned int run = 15660;
   static char const DefaultDirectory[] = "/u/eb/convery/ForJJ";
   static char const *dir = DefaultDirectory;

   static const char  FileTemplate[] = "%s/run%u_binary/run%u.bin";
   char filename[256];

   sprintf (filename, FileTemplate, dir, run, run);

   Packets::Pidx_t     pidx = 1;
   uint64_t timestamp = 0;

   // This is suppose to do the configuration
   config.init = true;
   config.mode = MODE_K_COPY;
   memset (&last_status, 0, sizeof (last_status));
   fill_runDisableAndFlush_frame  (sAxis);
   DuneDataCompressionCore (sAxis, mAxis, moduleIdx, config, status);
   print_status (last_status, status, -1);
   last_status = status;
   config.init = false;

   ////return 0;  //////   !!! KLUDGE !!!



   int fd = open (filename, O_RDONLY);
   if (fd < 0)
   {
      char const *err = strerror (errno);

      fprintf (stderr,
               "ERROR : Unable to open: %s\n"
               "Reason: %s\n", filename, err);
      exit (-1);
   }


   uint16_t    adcs[1024][128];

   unsigned int ntimes    = sizeof (adcs)    / sizeof (adcs[0]);
   unsigned int nchannels = sizeof (adcs[0]) / sizeof (adcs[0][0]);
   for (int its = 0; its < NSLICES; its++)
   {
      ssize_t nread = read (fd, adcs, sizeof (adcs));
      if (nread != sizeof (adcs))
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
            break;
         }
      }

      int runEnable = 1;
      int flush     = 0;

      AxisOut buf ("Buf");


      runEnable = 1;

      // Process each slice of 1024 time samples
      for (int isample = 0; isample < 1024; isample++)
      {
         ///flush     = (isample == 32) || (isample == 1023);
         ///runEnable = (isample != 64);

         if (flush | (runEnable == 0))
         {
            printf ("Sample[%4u] Flush:%d runEnable:%d\n", isample, flush, runEnable);
            fill_empty_frame (sAxis, flush, runEnable);
         }
         else
         {
            fill_frame (sAxis, timestamp, adcs[isample], runEnable, flush);
         }

         DuneDataCompressionCore(sAxis, mAxis, moduleIdx, config, status);

         if (differ (last_status, status))
         {
            print_status (last_status, status, isample);
            last_status = status;
         }

         timestamp += 1;
         while (!mAxis.empty ())
         {
            AxisOut_t out = mAxis.read ();
            std::cout << std::dec << std::setw ( 2) << "User: "  << out.user
                                                    << " Last: " << out.last
                      << std::hex << std::setw ( 4) << " Keep: " << out.keep
                      << std::hex << std::setw (16) << " Data: " << out.data << std::endl;

            if ( (out.user & 2) || (out.last & 1))
            {
               // Flush frame
               if (!buf.empty ())
               {
                  print_status (last_status, status, isample);
                  print (buf);
               }
            }


            //write_packet   (fd, mAxis);
            //print_status   (status);
            //print_packet   (mAxis);
            //SIMULATION (print_internal (pframe, beg, end);)
         }

      }

      pidx ^= 1;
      printf ("NEXT\n");
   }

   if (!mAxis.empty())  std::cout << "mAxis is not empty" << std::endl;
   if (!sAxis.empty())  std::cout << "sAxis is not empty" << std::endl;
    //if (!buf.empty  ())  std::cout << "buf   is not empty" << std::endl;
   return 0;
}
/* ---------------------------------------------------------------------- */


static void print (AxisOut &mAxis)
{
   std::cout << "OUTPUT DATA\n";
   int frame = 0;
   int odx   = 0;

   {
      AxisOut_t out = mAxis.read ();
      uint64_t data = out.data;
      int      user = out.user;
      int      last = out.last;
      odx += 1;
      std::cout << "Hdr: " << std::setw(16) << std::hex << data
                << "user:" << std::setw (4) << std::hex << user
                << "last:" << std::setw (4) << std::hex << last << std::endl;
   }


   while (1)
   {
      // There are 30 64-bit words in a WIB frame
      for (int widx = 0; widx < 30; widx++)
      {
         if ((widx % 4) == 0) std::cout << std::setw (5) << std::hex << odx << ':';

         AxisOut_t out = mAxis.read ();
         uint64_t data = out.data;
         int      user = out.user;
         int      last = out.last;

         // Check if this is the last word
         if (last)
         {
             if ((widx % 4) == 3) std::cout << std::endl;

             odx += 1;
             std::cout << "Tlr: " << std::setw(16) << std::hex << data
                          << "user:" << std::setw (4) << std::hex << user
                          << "last:" << std::setw (4) << std::hex << last << std::endl;

             goto EXIT;
         }
         else if (user != 0)
         {
            if ((widx % 4) == 3) std::cout << std::endl;

            std::cout << "Error: " << std::setw(16) << std::hex << data
                             << "user:" << std::setw (4) << std::hex << user
                             << "last:" << std::setw (4) << std::hex << last << std::endl;

         }
         else
         {
            std::cout << ' ' << std::setw (16) << std::hex << data;
         }

         if ((widx % 4) == 3) std::cout << std::endl;

         odx += 1;
      }


      std::cout << " Frame: " << std::setw(4) << std::hex << frame << std::endl;
      frame += 1;
      std::cout << std::endl;
      if (odx > 1 + 30*1024 + 1)
      {
         std::cout << "Overrun\n";
         break;
      }
   }


   EXIT:


   return;
}


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



