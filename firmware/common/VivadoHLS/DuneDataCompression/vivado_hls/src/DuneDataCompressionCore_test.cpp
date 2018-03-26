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
   MonitorModule                    status;

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
  ~MyStreamOut ()           { puts ("Deleting output stream"); return; }
};


class Source
{
public:
   Source ();

   void                    add_header (uint32_t id,     uint32_t version);
   void                    add_trailer(uint64_t status, uint32_t id, int       nbytes);
   void                    fill_frame (uint64_t                      timestamp,
                                       uint16_t  const adcs[MODULE_K_NCHANNELS],
                                       int                           runEnable,
                                       int                               flush);
   void              fill_dummy_frame (uint64_t timestamp,
                                       int          first,
                                       int         flush);
   void              fill_empty_frame (uint64_t timestamp,
                                       int      runEnable,
                                       int         flush);
   void         fill_runDisable_frame (uint64_t timestamp);
   void              fill_flush_frame (uint64_t timestamp);
   void fill_runDisableAndFlush_frame (uint64_t timestamp);
   void                        commit (uint64_t    w,
                                       int      user,
                                       int      last);
   static void                 commit (MyStreamIn  &stream,
                                       uint64_t          w,
                                       int            user,
                                       int            last);
   static void                 commit (MyStreamOut &stream,
                                       uint64_t          w,
                                       int            user,
                                       int            last);


public:
   MyStreamIn  m_src;  /*!< The stream used by the HLS module    */
   MyStreamOut m_chk;  /*!< The stream used to check the results */
};


Source::Source () :
      m_src ("In"),
      m_chk ("Check")
{
   return;
}

inline void Source::commit (MyStreamIn &stream, uint64_t w, int user, int last)
{
   AxisIn_t in;
   in.data = w;
   in.keep = 0xff;
   in.strb = 0;
   in.user = user;
   in.last = last;
   in.id   = 0;
   in.dest = 0;

   stream.write (in);
}


inline void Source::commit (MyStreamOut &stream, uint64_t w, int user, int last)
{
   AxisIn_t out;
   out.data = w;
   out.keep = 0xff;
   out.strb = 0;
   out.user = user;
   out.last = last;
   out.id   = 0;
   out.dest = 0;

   if (user)
   {
      printf ("Setting User = %d", user);
   }

   stream.write (out);
}



inline void Source::commit (uint64_t    w,
                            int      user,
                            int      last)
{
   AxisIn_t in;
   in.data = w;
   in.keep = 0xff;
   in.strb = 0;
   in.user = user;
   in.last = last;
   in.id   = 0;
   in.dest = 0;

   m_src.write (in);

   // WIB Frames are not delimited on the check/output stream
   in.user = 0;
   in.last = 0;
   m_chk.write (in);

   return;
}

void Source::add_header (uint32_t id, uint32_t version)
{
   uint64_t header  = Header::header (id, version);

   commit (m_chk, header, (1 << (int)AxisUserFirst::Sof), 0);

   return;
}


// Add the trailer to the check stream
void Source::add_trailer (uint64_t status, uint32_t id, int nbytes)
{
   uint64_t tlr = Trailer::trailer (id | nbytes + sizeof (tlr));
   commit (m_chk, status, 0, 0);
   commit (m_chk, tlr,    0, (1 << (int)AxisLast::Eof));
   return;
}


void Source::fill_dummy_frame (uint64_t timestamp,
                               int          first,
                               int          flush)
{
   int     iaxis = 0;
   int      last = (1 << (int)AxisLast::Eof);
   uint16_t data = 0xfff;


   uint16_t adcs[MODULE_K_NCHANNELS];

   for (int idx = 0; idx < MODULE_K_NCHANNELS; idx++)
   {
      adcs[idx] = 0xf00 | idx;
   }

   fill_frame (timestamp, adcs, 0, flush);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes a dummy WIB frame to the AXI input stream when doing
 *         a run disable.
 *                                                                        */
/* ---------------------------------------------------------------------- */
inline void Source::fill_runDisable_frame (uint64_t timestamp)
{
   int first = (1 << (int)AxisUserFirst::Sof);
   int flush = 0;
   fill_dummy_frame (timestamp, first, flush);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes a dummy WIB frame to the AXI input stream when doing
 *         a frame flush
 *                                                                                 */
/* ---------------------------------------------------------------------- */
inline void Source::fill_flush_frame (uint64_t timestamp)
{
   int first = (1 << (int)AxisUserFirst::Sof)
             | (1 << (int)AxisUserFirst::RunEnable);
   int flush = (1 << (int)AxisUserLast ::Flush);

   fill_dummy_frame (timestamp, first, flush);
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes a dummy WIB frame to the AXI input stream when doing
 *         a run disable and flush
 *                                                                        */
/* ---------------------------------------------------------------------- */
inline void Source::fill_runDisableAndFlush_frame (uint64_t timestamp)
{
   int first = (1 << (int)AxisUserFirst::Sof);
   int flush = (1 << (int)AxisUserLast::Flush);

   fill_dummy_frame (timestamp, first, flush);
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes an empty WIB frame to the AXI input stream
 *
 *  \param[ in] runEnable   The run enable flag, if 0, data is pitched
 *  \param[ in]     flush   Flush flag
 *                                                                        */
/* ---------------------------------------------------------------------- */
inline void Source::fill_empty_frame (uint64_t timestamp,
                                      int      runEnable,
                                      int          flush)
{
   int first = (1 << (int)AxisUserFirst::Sof)
             | (runEnable ? (1 << (int)AxisUserFirst::RunEnable) : 0);
   flush     = flush      ? (1 << (int)AxisUserLast ::Flush)     : 0;

   fill_dummy_frame (timestamp, first, flush);
}
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *
 *  \param[out]      axis   The target AXI stream
 *  \param[ount]    xaxis   Duplicate copy of the target AXI stream
 *                          used in checking the output
 *  \param[ in]      nova   The nova timestamp + status bits
 *  \param[ in]       adc   The ADC value
 *  \param[ in] runEnable   The run enable flag, if 0, data is pitched
 *  \param[ in]     flush   Flush flag
 *                                                                        */
/* ---------------------------------------------------------------------- */
void Source::fill_frame (uint64_t                      timestamp,
                         uint16_t  const adcs[MODULE_K_NCHANNELS],
                         int                           runEnable,
                         int                               flush)
{
   static uint16_t CvtCnt = 0x100;
   static const WibFrame::Crate Crate (1);
   static const WibFrame::Slot  Slot  (2);
   static const WibFrame::Fiber Fiber (3);

   static const WibFrame::Id id = WibFrame::id (Crate, Slot, Fiber);
   int iaxis = 0;

   int first =              (1 << (int)AxisUserFirst::Sof)
             | (runEnable ? (1 << (int)AxisUserFirst::RunEnable) : 0);

   int last  = 0;

   flush  = flush ? (1 << (int)AxisUserLast::Flush) : 0;

   if (flush) printf ("Nonzero flush = %d\n", flush);

   WibFrame::Rsvd    rsvd0 = 0;
   WibFrame::ErrWord error = 0;

   // Set the first and runEnable flags = 3
   uint64_t w0 = WibFrame::w0 (WibFrame::K28_5, id, rsvd0, error);
   commit (m_src, w0, first, 0);
   commit (m_chk, w0, 0,     0);


   // 125/2 = 64.5
   uint64_t w1 = WibFrame::w1 (timestamp);
   commit (w1, 0, 0);

   WibFrame::ColdData::ConvertCount cvt_cnt = CvtCnt++;


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
      uint64_t s0 = WibFrame::ColdData::s0 (0, 0, 0, cvt_cnt);
      commit (s0, 0, 0);


      WibFrame::ColdData::ErrReg errReg (0);
      WibFrame::ColdData::Hdrs     hdrs (0);
      uint64_t s1 = WibFrame::ColdData::s1 (errReg, hdrs);
      commit (s1, 0, 0);


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
            if (icolddata == 1 && iadc == 63) { user = flush;  last = (1 << (int)AxisLast::Eof); }
            ////if (user) printf ("Non-zero user = %d (last = %d)\n", user, last);
            commit (sdat, user, last);

            ////std::cout << "WibIdx:" << std::setw (2) << iaxis
            ////          << "  "   << std::setw(16) << std::hex << sdat << std::endl;

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

static void update (MonitorModule &status, bool output_packet)
{
      status.read.nframes    += 1;
      status.read.nStates[0] += 1;
      status.write.npromoted += 1;
      status.write.npackets  += output_packet;

}


static bool differ (MonitorModule const &s0,
                    MonitorModule const &s1)
{
   // NOTE: types[0] is not checked.  This is the normal
   //       frame counter and would cause a difference
   //        every time.
   if (s0.common.pattern  != s1.common.pattern ) return true;
   if (s0.cfg.ncfgs       != s1.cfg.ncfgs      ) return true;
   if (s0.cfg.mode        != s1.cfg.mode       ) return true;
   if (s0.read.mask       != 0                 ) return true;
   if (s0.read.nStates[0] != s1.read.nStates[0]) return true;
   if (s0.read.nStates[1] != s1.read.nStates[1]) return true;
   if (s0.read.nStates[2] != s1.read.nStates[2]) return true;
   if (s0.read.nStates[3] != s1.read.nStates[3]) return true;
   if (s0.write.npackets  != s1.write.npackets ) return true;


   for (unsigned idx = 0; idx < sizeof (s0.read.nWibErrs) / sizeof (s1.read.nWibErrs[0]); idx++)
   {
      if (s0.read.nWibErrs[idx] != s1.read.nWibErrs[idx]) return true;
   }

   return false;
}

static void print_status (MonitorModule const &s0,
                          MonitorModule const &s1,
                          int             isample)
{
   printf ("New status at sample   : %6u\n"
         "               Common.pattern: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                     Cfg.mode: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                       .ncfgs: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         " Read                 .status: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                     .nframes: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         "     .state [         Normal]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [       Disabled]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [          Flush]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "            [    DisAndFlush]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"

         "            [        ErrSofM]: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
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

         "              Write.npromoted: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                    .ndropped: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                    .npackets: %8.8" PRIx32 " -> %8.8" PRIx32 "\n"
         "                      .nbytes: %8.8" PRIx32 " -> %8.8" PRIx32 "\n",
            isample,

            s0.common.pattern,                                   s1.common.pattern,
            s0.cfg.mode,                                         s1.cfg.mode,
            s0.cfg.ncfgs,                                        s1.cfg.ncfgs,

            s0.read.mask.to_uint (),                             s1.read.mask.to_uint (),
            s0.read.nframes,                                     s1.read.nframes,
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


static void update (MonitorModule  &status,
                    bool     output_packet);


static int check (MyStreamOut             &mAxis,
                  MyStreamOut             &sAxis,
                 MonitorModule      &last_status,
                 MonitorModule           &status,
                 int                     isample);

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
#  define NSLICES 1

    Source                             src;
    MyStreamOut              mAxis ("Out");
    ModuleIdx_t              moduleIdx = 1;
    ModuleConfig                    config;
    MonitorModule                   status;
    MonitorModule              last_status;
    int                        test_status;


   unsigned int                     run =                       15660;
   static char const DefaultDirectory[] =       "/u/eb/convery/ForJJ";
   static char const               *dir =            DefaultDirectory;
   static const char     FileTemplate[] = "%s/run%u_binary/run%u.bin";
   char filename[256];

   sprintf (filename, FileTemplate, dir, run, run);

   uint64_t timestamp = 0;

   uint32_t version = 1;
   uint32_t hdr_id  = Identifier::identifier (Identifier::FrameType::DATA,
                                              Identifier::DataType::WIB,
                                              0);

   // This is suppose to do the configuration
   config.init  = true;
   config.mode  = MODE_K_COPY;
   config.limit = 1 + 30 * 1024 + 1;
   memset (&last_status, 0, sizeof (last_status));

   // Add the header to the check stream
   src.add_header (hdr_id, version);
   src.fill_runDisableAndFlush_frame (timestamp);
   DuneDataCompressionCore (src.m_src, mAxis, moduleIdx, config, status);

   // Drain the check stream
   while (!src.m_chk.empty ()) src.m_chk.read ();

   print_status (last_status, status, -1);
   last_status = status;
   config.init = false;





   uint32_t summary;

   // ------------------
   // Open the test file
   // ------------------
   int fd = ::open (filename, O_RDONLY);
   if (fd < 0)
   {
      char const *err = strerror (errno);

      fprintf (stderr,
               "ERROR : Unable to open: %s\n"
               "Reason: %s\n", filename, err);
      exit (-1);
   }


   uint16_t adcs[1024][128];
   unsigned int    ntimes = sizeof (adcs)    / sizeof (adcs[0]);
   unsigned int nchannels = sizeof (adcs[0]) / sizeof (adcs[0][0]);
   for (int its = 0; its < NSLICES; its++)
   {
      summary = 0;

      // -------------------
      // Read one time slice
      // -------------------
      ssize_t nread = ::read (fd, adcs, sizeof (adcs));
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
            // ---
            // EOF
            // ---
            break;
         }
      }

      int runEnable = 1;
      int flush     = 0;
      runEnable     = 1;

      // Add the header to the check stream
      src.add_header (hdr_id, version);

      int nbytes = sizeof (Header);

      // Process each slice of 1024 time samples
      for (int isample = 0; isample < 1024; isample++)
      {
         ///flush     = (isample == 32) || (isample == 1023);
         ///runEnable = (isample != 64);

         timestamp += 25;

         if (flush | (runEnable == 0))
         {
            printf ("Sample[%4u] Flush:%d runEnable:%d\n", isample, flush, runEnable);
            src.fill_empty_frame (timestamp, flush, runEnable);
         }
         else
         {
            src.fill_frame (timestamp, adcs[isample], runEnable, flush);
            nbytes += sizeof (WibFrame);
         }

         DuneDataCompressionCore(src.m_src, mAxis, moduleIdx, config, status);
         update (last_status, isample == 1023);


         static int Count = 0;
         if (differ (last_status, status) || isample == 1023)
         {
            if (Count < 5)
            {
               print_status (last_status, status, isample);
            }
            Count += 1;
            last_status = status;
         }

         summary |= status.read.mask;
      }

      print_status (last_status, status, 0xffff);


      // Add trailer to the check frame
      src.add_trailer (summary, hdr_id, nbytes);
      test_status = check (mAxis, src.m_chk, last_status, status, its * 1024);

      if (test_status)
      {
         printf ("Failed\n");
         break;
      }


      //write_packet   (fd, mAxis);
      //print_status   (status);
      //print_packet   (mAxis);
      //SIMULATION (print_internal (pframe, beg, end);)

      printf ("NEXT\n");
   }

   if (!    mAxis.empty()) { test_status = -2; std::cout << "mAxis is not empty" << std::endl; }
   if (!src.m_src.empty()) { test_status = -3; std::cout << "sAxis is not empty" << std::endl; }

   if (test_status == 0)
   {
      printf ("Passed\n");
   }
   return test_status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Check that the output stream matches the input stream
 *   \return == 0, okay
 *   \return != 0, failure
 *
 *   \param[in]   mAxis   The output stream
 *   \param[in]   sAxis   The source input stream
 *
\* ---------------------------------------------------------------------- */
static int check (MyStreamOut             &mAxis,
                  MyStreamOut             &sAxis,
                 MonitorModule      &last_status,
                 MonitorModule           &status,
                 int                     isample)
{
   int idx = 0;
   int nerrs = 0;

   while (1)
   {
      bool dst_empty = mAxis.empty ();
      bool src_empty = sAxis.empty ();


      if (!dst_empty && !src_empty)
      {
         AxisOut_t  dst = mAxis.read  ();
         AxisIn_t   src = sAxis.read  ();

         int error = dst.data != src.data;

         if (error)
         {
            std::cout << std::dec << std::setw ( 5) << "Error: Data @" << idx
                      << std::dec << std::setw ( 2) << " User: " << dst.user
                                                    << " Last: " << dst.last
                      << std::hex << std::setw ( 4) << " Keep: " << dst.keep
                      << std::hex << std::setw (16) << " Data: " << dst.data << " : " << src.data
                      << std::endl;
            nerrs+= 1;
            if (nerrs >= 100) return -1;
         }

         // Bit 1 of the user field must be set on the first word
         if ( (idx == 0) && (((dst.user & 2) == 0) || ((src.user & 2) == 0)) )
         {
            std::cout << std::dec << std::setw ( 5) << "Error: User 1 bit clear @" << idx
                      << std::dec << std::setw ( 2) << " User: " << dst.user
                                                    << " Last: " << dst.last
                      << std::hex << std::setw ( 4) << " Keep: " << dst.keep
                      << std::hex << std::setw (16) << " Data: " << dst.data
                      << std::endl;
            return  -1;
         }

         // Bit 1 of the user field must be clear all but the first word
         if ( (idx != 0) && ( (dst.user & 2) || (src.user & 2)) )
         {
            std::cout << std::dec << std::setw ( 5) << "Error: User 1 bit set @" << idx
                      << std::dec << std::setw ( 2) << " User: " << dst.user
                                                    << " Last: " << dst.last
                      << std::hex << std::setw ( 4) << " Keep: " << dst.keep
                      << std::hex << std::setw (16) << " Data: " << dst.data
                      << std::endl;
            return -1;
         }

         if (dst.last.test(static_cast<unsigned>(AxisLast::Eof)))
         {
            if (src.last.test (static_cast<unsigned>(AxisLast::Eof)))
            {
               // Check that both are now empty
               dst_empty = mAxis.empty ();
               src_empty = sAxis.empty ();
               if (dst_empty && src_empty)
               {
                  // All is well
                  return 0;
               }
               else if (dst_empty)
               {
                  std::cout << "Error: destination empty, but source not @ "
                            << std::dec << std::setw (5) << idx << std::endl;
                  return -1;
               }
               else
               {
                  std::cout << "Error: source empty, but destination not @ "
                            << std::dec << std::setw (5) << idx << std::endl;
                  return -1;
               }
            }
         }
      }
      else if (dst_empty || src_empty)
      {
         // This should not happen, dst.user or dst.last should signal the end
         std::cout << "Error:Premature empty: dst:src[" << idx << ']' << dst_empty << ':' << src_empty
                   << std::endl;
         break;
      }

      idx += 1;

   }

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



