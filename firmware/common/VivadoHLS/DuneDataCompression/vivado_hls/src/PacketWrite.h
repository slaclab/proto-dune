#include "AxisBitStream.h"
#include "Parameters.h"
#include "AP-Encode.h"



/* ====================================================================== *\
 *
 *   This is just to check-point before making extensive changes which
 *   we feel are necessaryto get encodeN to dataflow without doing a copy
 *   of the data.
 *
 *   This is 'an almost' a working version; it misses meeting the timing
 *   by a factor of 3. (The C/RTL cosim indicated on the order of 200K
 *   cycles for 128 channels. The problem child is the copyN routine;
 *   it either takes too long or takes too many LUTs.
 *
 *   In addition, the code in writeAdcsN is structured to dataflow
 *   copyN and encodeN.  What Vivado HLS seems to have produced is
 *   copyN running serially and encodeN + writeN in dataflow.
 *
 *   What one really wants is all three to pipelined.  I tried this
 *   once and while it synthesized, the C/RTL cosimulation hung.
 *
 *   The commited version passed the C-Simulation/Synthesised and
 *   C/RTL cosimulation with both
 *          NCHANs = 8
 *          NCHANS = MODULE_K_NCHANNELS (i.e. 128)
 *
\* ====================================================================== */


// Define which channels to process, debug only
#define WRITE_DATA 1


#define NSERIAL    MODULE_K_NSERIAL
#define NCHANS     (NSERIAL*MODULE_K_NPARALLEL)

#define NPARALLEL  MODULE_K_NPARALLEL
#define COMPRESS_VERSION 0x10000000

#define CHECKER          1
#define WRITE_ADCS_PRINT 0  /// STRIP remove output 2018-06-28


#ifdef __SYNTHESIS__
#undef  CHECKER
#undef  WRITE_ADCS_PRINT
#endif



static void  write_packet (AxisOut                                   &mAxis,
                           PacketContext                            &pktCtx,
                           CompressionContext                       &cmpCtx,
                           MonitorRead                         &monitorRead);


/* ====================================================================== */
/* Local Definitions/Prototypes                                           */
/* ---------------------------------------------------------------------- */
typedef uint32_t ChannelOffset_t;

static void   write_toc (AxisOut                                    &mAxis,
                         int                                          &odx,
                         int                                        nchans,
                         int                                      nsamples,
                         ChannelOffset_t                         offsets[]);

static void write_adcs (AxisBitStream                               &bAxis,
                        AxisOut                                     &mAxis,
                        ChannelOffset_t        offsets[MODULE_K_NCHANNELS],
                        CompressionContext                        &cmpCtx);

static inline void
           write_adcs4 (AxisBitStream                              &bAxis,
                        AxisOut                                    &mAxis,
                        ChannelOffset_t                         offsets[],
                        AdcIn_t                  adcs0[PACKET_K_NSAMPLES],
                        AdcIn_t                  adcs1[PACKET_K_NSAMPLES],
                        AdcIn_t                  adcs2[PACKET_K_NSAMPLES],
                        AdcIn_t                  adcs3[PACKET_K_NSAMPLES],
                        Histogram                                  &hist0,
                        Histogram                                  &hist1,
                        Histogram                                  &hist2,
                        Histogram                                  &hist3,
                        int                                         ichan);

static __inline void encode4 (APE_etxOut                          *etx,
                              Histogram                        &hists0,
                              Histogram                        &hists1,
                              Histogram                        &hists2,
                              Histogram                        &hists3,
                              AdcIn_t         adcs0[PACKET_K_NSAMPLES],
                              AdcIn_t         adcs1[PACKET_K_NSAMPLES],
                              AdcIn_t         adcs2[PACKET_K_NSAMPLES],
                              AdcIn_t         adcs3[PACKET_K_NSAMPLES],
                              int                               ichan);

static void             writeN (AxisBitStream                       &bAxis,
                                AxisOut                             &mAxis,
                                ChannelOffset_t                  offsets[],
                                APE_etxOut                           etx[],
                                int                               nstreams,
                                int                                 nchans,
                                int                                 ichan);

static void             encode (APE_etxOut                           &etx,
                                Histogram                           &hist,
                                AdcIn_t           adcs[PACKET_K_NSAMPLES],
                                int                                nadcs);

static void               pack (AxisBitStream                      &baxis,
                                AxisOut                            &mAxis,
                                OStream                           &osream);

/* ---------------------------------------------------------------------- */
#if WRITE_ADCS_PRINT
/* ---------------------------------------------------------------------- */

static void write_adcs_print  (AdcIn_t const adcs0[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                               AdcIn_t const adcs1[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                               AdcIn_t const adcs2[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                               AdcIn_t const adcs3[MODULE_K_NSERIAL][PACKET_K_NSAMPLES]);

static void write_sizes_print (int ichan, int csize, int osize);

/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */

#define write_adcs_print(_adcs0, _adcs1, _adcs2, _adcs3)
#define write_adcs_bit_size_print(_ichan, _ebit_size)
#define write_adcs_encode_chan(_ichan)
#define write_sizes_print(_ichan, csize, osize)

/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */
/* Local Prototypes/Definitions                                           */
/* ====================================================================== */


/* ====================================================================== */
/* Implementation                                                         */
/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Writes one packets worth of data to the output stream
 *
 *   \param[out]        mAxis  The output data stream
 *   \param[ in]       pktCtx  The per packet context
 *   \param[ in]       cmpCtx  The per channeel compression context
 *   \param[out] monitorWrite  Contains the variables monitoring the
 *                             output.  These are things like the
 *                             number of bytes and packets written,
 *
 *   \par
 *   Packets are only written when a full 1024 time samples have been
 *   accumulated.
 *
 \* ---------------------------------------------------------------------- */
static void  write_packet (AxisOut                               &mAxis,
                           PacketContext                        &pktCtx,
                           CompressionContext                   &cmpCtx,
                           MonitorWrite                   &monitorWrite)
{
   #define HLS INLINE


   static bool        First= true;
   static MonitorWrite lclMonitor;

   if (First)
   {
      lclMonitor.init ();
      First = false;
   }


   int             odx = 0;
   ReadStatus_t status = pktCtx.m_status;

   // -----------------------------------------------------------------------
   // Output the WIB header information.  For packets with no errors,
   // this consist of just the header words (6 of them) from the first
   // WIB packet. For these packets, the rest can be generated from these
   // For packets with WIB frames in error, the header words in disagreement
   // are included.  An exception index, consisting of
   //    1) an index of which WIB frame is in error
   //    2) a bit mask indicating which of the 6 words are in error
   // allows reconstruction of the WIB frame.
   // -----------------------------------------------------------------------
   pktCtx.commit (mAxis, odx, true, true, status);

   int bdx = odx << 6;
   AxisBitStream bAxis (bdx);


   ChannelOffset_t    offsets[MODULE_K_NCHANNELS+2];
   write_adcs (bAxis, mAxis, offsets, cmpCtx);

   ///odx = (bAxis.m_idx + 63) >> 6;


   ////offsets[NCHANS]   = bAxis.m_idx;
   offsets[NCHANS+1] = 0;
   odx = (offsets[NCHANS] + 63) >> 6;
   write_toc (mAxis, odx, NCHANS, PACKET_K_NSAMPLES, offsets);

   // -----------------------------------------------------------------
   // The header has been removed. Before, information in this header
   // was consumed by the receiver and then stripped by sending only
   // the data after the header. This does not work for RSSI which
   // cannot send starting at an offset.  The  information needed by
   // the host side has been moved to the end. Since the length of
   // the DMA is known, the receiver can locate it.  It is effectively
   // discarded by limiting the RSSI transaction to not include this
   // word.
   // ----------------------------------------------------------------
   epilogue (mAxis, odx, Identifier::DataType::COMPRESSED, status);

   static uint32_t MaxBytes = 0;
   uint32_t nbytes = odx << 3;

   MaxBytes = (nbytes > MaxBytes) ? nbytes : MaxBytes;

   lclMonitor.nbytes    += nbytes;
   lclMonitor.npackets  += 1;
   lclMonitor.ndropped   = MaxBytes; /// KLUDGE
   lclMonitor.npromoted  = nbytes;

   monitorWrite = lclMonitor;


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Output the table of contents, this gives the starting bit
 *          position within the record for each channel
 *
 *   \param[   out]    mAxis  The output data stream
 *   \param[in:out]      odx  The current 64-bit output index
 *   \param[    in]   nchans  The number of channels
 *   \param[    in] nsamples  The number of samples per channel
 *   \param[    in]  offsets  The bit offsets for each channel
 *
\* ---------------------------------------------------------------------- */
static void   write_toc (AxisOut                       &mAxis,
                         int                             &odx,
                         int                           nchans,
                         int                         nsamples,
                         ChannelOffset_t            offsets[])
{
   #pragma HLS INLINE
   //// STRIP #pragma HLS PIPELINE
   uint64_t w64;
   static const int HeaderFmt  = 3;
   static const int TocRecType = 2;

   // -------------------------------------------------------------
   // The last entry contains the position of the last bit written.
   // This allows the sizes to be calculated by doing a simple
   // subtraction between adjacent elements.
   // This is the reasons the comparison is '<=' rather than '<'
   // -------------------------------------------------------------

   WRITE_TOC_LOOP:
   for (int ichan = 0; ichan <= nchans; ichan += 2)
   {
      #pragma HLS PIPELINE
      w64  = offsets[ichan];
      w64  |= (uint64_t)offsets[ichan + 1] << 32;
      commit (mAxis, odx, true, w64, 0, 0);
   }

   // ------------------------------------------------------------------------------
   // Pack the trailer
   // ----------------
   //         | Rsvd | #Channels-1 | #Samples-1 | Layout | Len64 | RecType | TlrFmt
   //  # Bits |  12  |         12  |         12 |      4 |    16 |       4 |      4
   //  Offset |  52  |         40  |         28 |     24 |     8 |       4 |      0
   // ------------------------------------------------------------------------------
   w64 = ((uint64_t)(nchans - 1)  << 40) |((uint64_t)(nsamples - 1) << 28)
       | (0 << 24) | ((((nchans + 2) / 2) + 1) << 8) | (TocRecType << 4) | (HeaderFmt << 0);

   commit (mAxis, odx, true, w64, 0, 0);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Compresses and writes the ADcs for all channels to the
 *         output stream
 *
 *  \param[in:out] bAxis  An intermediate 64-bit bit stream used to
 *                        concatenate the compressed data streams
 *  \param[in]     mAxis  The output stream
 *  \param[out]  offsets  The starting bit offset for each channel
 *  \param[ in]   cmpCtx  The per channeel compression context
 *
\* ---------------------------------------------------------------------- */
static inline void
       write_adcs (AxisBitStream                                       &bAxis,
                   AxisOut                                             &mAxis,
                   ChannelOffset_t                offsets[MODULE_K_NCHANNELS],
                   CompressionContext                                 &cmpCtx)
{
   #pragma HLS INLINE

   // Diagnostic printout
   write_adcs_print (cmpCtx.adcs.sg0,
                     cmpCtx.adcs.sg1,
                     cmpCtx.adcs.sg2,
                     cmpCtx.adcs.sg3);

   WRITE_ADCS_CHANNEL_LOOP:
   for (int isg = 0; isg < NSERIAL; isg++)
   {
      write_adcs4 (bAxis, mAxis, offsets,
                   cmpCtx. adcs.sg0[isg],
                   cmpCtx. adcs.sg1[isg],
                   cmpCtx. adcs.sg2[isg],
                   cmpCtx. adcs.sg3[isg],
                   cmpCtx.hists.sg0[isg],
                   cmpCtx.hists.sg1[isg],
                   cmpCtx.hists.sg2[isg],
                   cmpCtx.hists.sg3[isg],
                   isg * 4);
   }

   ///bAxis.flush (mAxis);

   return;
}
/* -------------------------------------------------------------------- */




/* -------------------------------------------------------------------- *//*!
 *
 *   \brief  Writes out the encoded output for NPARALLEL channels
 *
\* -------------------------------------------------------------------- */
static inline void
       write_adcs4 (AxisBitStream                             &bAxis,
                    AxisOut                                   &mAxis,
                    ChannelOffset_t                        offsets[],
                    AdcIn_t                 adcs0[PACKET_K_NSAMPLES],
                    AdcIn_t                 adcs1[PACKET_K_NSAMPLES],
                    AdcIn_t                 adcs2[PACKET_K_NSAMPLES],
                    AdcIn_t                 adcs3[PACKET_K_NSAMPLES],
                    Histogram                                 &hist0,
                    Histogram                                 &hist1,
                    Histogram                                 &hist2,
                    Histogram                                 &hist3,
                    int                                        ichan)
{
   #pragma HLS INLINE /// STRIP 2018-07-01 off -- With inline on it fails at chan 4
   #pragma HLS DATAFLOW


   struct Container
   {
      APE_etxOut                           etxOut[NPARALLEL];
   };

   Container container;
   #pragma HLS ARRAY_PARTITION variable=container.etxOut complete dim=1
   #pragma HLS STREAM          variable=container depth=2
   /// #pragma HLS STREAM variable=container off

    encode4 (container.etxOut,
             hist0, hist1, hist2, hist3,
             adcs0, adcs1, adcs2, adcs3,
             ichan);

   writeN  (bAxis, mAxis, &offsets[ichan], container.etxOut, NPARALLEL, NSERIAL, ichan);
}
/* -------------------------------------------------------------------- */



/* -------------------------------------------------------------------- */
static __inline void encode4 (APE_etxOut                          *etx,
                              Histogram                        &hists0,
                              Histogram                        &hists1,
                              Histogram                        &hists2,
                              Histogram                        &hists3,
                              AdcIn_t        adcs0[PACKET_K_NSAMPLES],
                              AdcIn_t        adcs1[PACKET_K_NSAMPLES],
                              AdcIn_t        adcs2[PACKET_K_NSAMPLES],
                              AdcIn_t        adcs3[PACKET_K_NSAMPLES],
                              int                               ichan)
{
   #pragma HLS INLINE off
   #pragma HLS DATAFLOW


   encode (etx[0], hists0, adcs0, PACKET_K_NSAMPLES);
   encode (etx[1], hists1, adcs1, PACKET_K_NSAMPLES);
   encode (etx[2], hists2, adcs2, PACKET_K_NSAMPLES);
   encode (etx[3], hists3, adcs3, PACKET_K_NSAMPLES);


   #if CHECKER
      // -----------------------------------------
      // Executed only if APE_CHECKER is non-zero
      // -----------------------------------------

      for (int idx = 0; idx < 4; idx++)
      {
         write_sizes_print (ichan, etx[idx].ba.m_cidx, etx[idx].ha.m_cidx);
      }

      bool failure0 = encode_check (etx[0], hists0, adcs0);
      if  (failure0)
      {
         APE_encode (etx[0], hists0, adcs0, PACKET_K_NSAMPLES);
         encode_check (etx[0], hists0, adcs0);

      }

      bool failure1 = encode_check (etx[1], hists1, adcs1);
      if  (failure1)
      {
         APE_encode (etx[1], hists1, adcs1, PACKET_K_NSAMPLES);
         encode_check (etx[1], hists1, adcs1);

      }

      bool failure2 = encode_check (etx[2], hists2, adcs2);
      if  (failure2)
      {
         APE_encode (etx[2], hists2, adcs2, PACKET_K_NSAMPLES);
         encode_check (etx[2], hists2, adcs2);

      }

      bool failure3 = encode_check (etx[3], hists3, adcs3);
      if  (failure3)
      {
         APE_encode (etx[3], hists3, adcs3, PACKET_K_NSAMPLES);
         encode_check (etx[3], hists3, adcs3);
      }

      if (failure0 || failure1 || failure2 || failure3)
      {
         exit (-1);
      }
   #endif
}
/* -------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Wrapper to encapsulate any per channel encoding stuff
 *
 *   \param[out]   etx  The encoding context for this channel
 *   \param[ in]  hist  The encoding frequency distribution
 *   \param[ in]  adcs  The ADCs to be encoded
 *   \param[ in] nadcs  The number of ADCs to be encoded
 *
\* ---------------------------------------------------------------------- */
static void encode (APE_etxOut                        &etx,
                    Histogram                        &hist,
                    AdcIn_t        adcs[PACKET_K_NSAMPLES],
                    int                             nadcs)
{
  #pragma HLS INLINE

   APE_encode (etx, hist, adcs, PACKET_K_NSAMPLES);

}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the \a ichan channels in parallel
 *
 *  \param[in:out] mAxis The output stream
 *  \param[in:out]   buf The temporary staging buffer,
 *                       contains any left over shard.
 *  \param[in:out]   mdx The bit index of the output array
 *  \param[in]       etx The vector of encoding contexts
 *  \param[in]       end The last bit of each channel
 *  \param[in]  nstreams The number of parallel streams
 *  \param[in]    nchans The number of channels per stream
 *  \param[in]     ichan The channel number of the first channel to
 *                       be encoded
 *
\* -------------------------------------------------------------------- */
static void writeN (AxisBitStream                  &tAxis,
                    AxisOut                        &mAxis,
                    ChannelOffset_t             offsets[],
                    APE_etxOut                      etx[],
                    int                          nstreams,
                    int                            nchans,
                    int                             ichan)
{
  #pragma HLS INLINE off

   static AxisBitStream bAxis;

   if (ichan == 0)
   {
      bAxis.m_idx = tAxis.m_idx;
      bAxis.m_cur = 0;
   }

   int idx;

   WRITE_CHANNEL_LOOP:
   for (idx = 0;  idx < nstreams; idx++)
   {
      //#pragma HLS PIPELINE -- This caused problems at one time

      offsets[idx] = bAxis.m_idx;

      // Output the Histogram/ADC overflow and compressed bit streams
      pack (bAxis, mAxis, etx[idx].ha);
      pack (bAxis, mAxis, etx[idx].ba);

#if 0
      if (ichan >= (NCHANS-1))
      {
         return;
      }
#endif

   }

   if (ichan == 124)
   {
      bAxis.flush (mAxis);
      offsets[idx] = bAxis.m_idx;
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


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Packs and writes \a ostream to the output stream
 *
 *   \param[out]    bAxis  The output bitstream.
 *   \param[out]    mAxis  The AXI output stream
 *   \param[ in]  ostream  The stream to write out
 *
 *   Bits are pushed into \a bAxis and when 64-bits are available
 *   they are written to the output stream, \a mAxis.  Any leftover
 *   bits remain in \a bAxis.  When all writing is complete, a flush
 *   operation must be performed to force the last few bits out.
 *
\* ---------------------------------------------------------------------- */
static void pack (AxisBitStream                     &bAxis,
                  AxisOut                           &mAxis,
                  OStream                         &ostream)
{
    #pragma HLS INLINE off /// jjr 2018-08-02 added the off
    ///#pragma HLS PIPELINE


   // ----------------------------------------------------------------
   // Need to merge the shards of
   //     a) the buffered output stream --        buf
   // and b) the leftovers from the input stream  bsbuf of bsvld bits
   // ---------------------------------------------------------------


   // ----------------------------------
   // Count the bits that will be output
   // when the loop is completed.
   // -----------------------------------
   uint64_t           data;
   BitStream64::Idx_t cidx = ostream.m_cidx;

   ///bs.m_cidx.read (cidx);
   int bleft = cidx;

   AxisOut_t out;
   out.keep = 0xFF;
   out.strb = 0x0;
   out.user = 0;
   out.last = 0;
   out.id   = 0x0;
   out.dest = 0x0;

   int cnt = bleft >> 6;

#if 1
   PACK_LOOP:
   for (int idx = 0; idx < cnt; idx++)
   {
       #pragma HLS LOOP_TRIPCOUNT min=32 max=128 avg=32
       #pragma HLS PIPELINE /// STRIP fails when pipelined
       ////#pragma HLS UNROLL factor=2

      // Make room. get and insert the new 64-bts worth of data
       #if USE_FIFO
           data = ostream.read ();
       #else
          data = ostream.read (idx);
       #endif

       bAxis.insert (mAxis, data, 64);
   }
#else
   PACK_LOOP:
   int cntRnd = cnt & ~0x1;
   for (int idx = 0; idx < cntRnd; idx += 2)
   {
      #pragma HLS LOOP_TRIPCOUNT min=12 max=48 avg=16

      #pragma HLS PIPELINE II=2     /// STRIP -- this caused problems at one time
      #pragma HLS UNROLL factor=1

      // Make room. get and insert the new 64-bts worth of data
      #if USE_FIFO
          data = ostream.read ();
      #else
         data = ostream.read (idx);
      #endif

      bAxis.insert (mAxis, data, 64);


      // Make room. get and insert the new 64-bts worth of data
     #if USE_FIFO
         data = ostream.read ();
     #else
         data = ostream.read (idx + 1);
     #endif

      bAxis.insert (mAxis, data, 64);
   }

   if (cnt&1)
   {
      #if USE_FIFO
          data = ostream.read ();
      #else
          data = ostream.read (cnt-1);
      #endif

      bAxis.insert (mAxis, data, 64);
   }
#endif

   // Push an leftover input bits into the output stream.
   int nbits = bleft & 0x3f; // NEW ostream.bidx();
   if (nbits)
   {
      bAxis.insert (mAxis, ostream.m_ccur, nbits);
   }


   return;
}
/* ---------------------------------------------------------------------- */



#if       CHECKER
#define   APD_DUMP  1
#include "AP-Decode.h"

APD_dumpStatement
(

static inline int16_t restore (int sym)
{
   if (sym & 1)  return -(sym >> 1);
   else          return  (sym >> 1);
}

static void print_decoded (uint16_t sym, int idy)
{
   static uint16_t Adcs[16];
   static uint16_t Prv;

   return;  /// STRIP added 2018-06-28 to reduce output

   // ---------------------------------------------------------------------------------------
   // Print the symbols and accumulate the restored ADCs to be printed after every 16 symbols
   // ---------------------------------------------------------------------------------------
   int idz = idy & 0xf;
   if (idz == 0) { std::cout << "Sym[" << std::hex << std::setw (4) << idy << "] "; }
   if (idy == 0) { Adcs[0]   = sym; }
   else          { Adcs[idz] = Adcs[(idz-1) & 0xf] + restore (sym); }
   std::cout << std::hex << std::setw (3) << sym;


   // ----------------------------------------
   // If have accumulated the ADCs, print them
   // ----------------------------------------
   if (idz == 0xf)
   {
      std::cout << "  ";
      for (int i = 0; i < 16; i++)  std::cout << std::hex << std::setw (4) << Adcs[i];
      std::cout << std::endl;
   }

   return;
}
)

static bool decode_data (uint16_t      dadcs[PACKET_K_NSAMPLES],
                         BFU                              &ebfu,
                         uint64_t  const                  *ebuf,
                         BFU                              &obfu,
                         uint64_t const                   *obuf,
                         Histogram const                  &hist,
                         AdcIn_t const  adcs[PACKET_K_NSAMPLES])
{
   // Integrate the histogram
   APD_table_t table[Histogram::NBins + 2];
   int  cnt  = hist.m_lastgt0 + 1;
   table[0]  = cnt;
   int total = 0;
   for (int idx = 1;; idx++)
   {
      table[idx] = total;
      if (idx > cnt) break;
      total     += hist.m_omask.test (idx -1) ? hist.m_bins[idx-1] : Histogram::Entry_t (0);
   }

   uint16_t bins[Histogram::NBins];
   int     nbins;
   int   novrflw;

   int       prv;
   int oposition = hist_decode (bins, &nbins, &prv, &novrflw, obfu, obuf);


   APD_dtx dtx;
   APD_start (&dtx, ebuf, 0);


   int Nerrs = 0;
   uint16_t sym = prv;
   for (int idy = 0; ; idy++)
   {
      dadcs[idy] = prv;
      APD_dumpStatement (print_decoded (sym, idy));

      if (dadcs[idy] != adcs[idy])
      {
         std::cout << std::endl << "Error @" << std::hex << idy << "sym: " << std::hex << sym << std::endl;
         if (Nerrs++ > 20) return true;
      }

      if (idy == PACKET_K_NSAMPLES -1)
      {
         break;
      }


      sym = APD_decode (&dtx, table);
      if (sym == 0)
      {
         // Have overflow
         // Insert check for novrflw == 0, the bit extracter does not do 0
         int ovr = novrflw ? _bfu_extractR (obfu, obuf, oposition, novrflw) : 0;
         sym     = nbins + ovr;
      }

      prv += restore (sym);
   }


   APD_finish (&dtx);
   return Nerrs != 0;
}



static int copy (uint64_t *buf, OStream &ostream)
{
   // Copy the data into a temporary bit stream
#if USE_FIFO
      int idx = 0;
      while (1)
      {
         uint64_t dat;
         bool okay = ostream.read_nb (buf[idx]);
         if (!okay) break;
         idx += 1;
      }
   #else
      int idx;
      int cnt = ostream.m_cidx >> 6;
      for (idx = 0; idx < cnt; idx++)
      {
         buf[idx] = ostream.read (idx);
      }
      idx = cnt;
   #endif

   // Get the last one
   // This is kind of cheating. It believes that
   // bs.m_cur is not going to change between this
   // copy and the restore.
   int left = ostream.m_cidx & 0x3f;
   if (left)
   {
      uint64_t last =  ostream.m_ccur << (0x40 - left);
      buf[idx] = last;
   }

   return idx;
}

void restore (OStream &ostream, uint64_t *buf, int nbuf)
{
   // Push this all back from whence it came
   #if USE_FIFO
       for (int idx = 0; idx < nbuf; idx++)
       {
           ostream.write_nb (buf[idx]);
       }
   #else
       // Nothing to do hear, the read is non-destructive
   #endif

   return;
}

bool encode_check (APE_etxOut &etx, Histogram const &hist, AdcIn_t const adcs[PACKET_K_NSAMPLES])
{
   static uint64_t hbuf[PACKET_K_NSAMPLES/(sizeof (uint64_t) / sizeof (int16_t))];
   static uint64_t ebuf[PACKET_K_NSAMPLES/(sizeof (uint64_t) / sizeof (int16_t)) + Histogram::NBins * 10/64 + 10];


   // Copy the data into a temporary bit stream
   int hcnt = copy (hbuf, etx.ha);
   int ecnt = copy (ebuf, etx.ba);

   BFU hbfu;
     _bfu_put (hbfu, hbuf[0], 0);

   BFU ebfu;
   _bfu_put (ebfu, ebuf[0], 0);


   uint16_t dadcs[PACKET_K_NSAMPLES];
   bool failure = decode_data (dadcs, ebfu, ebuf, hbfu, hbuf, hist, adcs);

   restore (etx.ha, hbuf, hcnt);
   restore (etx.ba, ebuf, ecnt);


   return failure;
}
#endif


/* ---------------------------------------------------------------------- */
#if WRITE_ADCS_PRINT
/* ---------------------------------------------------------------------- */
static int List[] = {0};

static void
  write_adcs_print  (AdcIn_t const adcs0[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                     AdcIn_t const adcs1[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                     AdcIn_t const adcs2[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                     AdcIn_t const adcs3[MODULE_K_NSERIAL][PACKET_K_NSAMPLES])
{
   for (int ilist = 0; ilist < sizeof (List) / sizeof (List[0]); ilist++)
   {
      int ichan = List[ilist];

      std::cout << "Encoded Adcs[" << std::hex << std::setw (2) << ichan << ']' << std::endl;

      AdcIn_t const *adcs;
      int isg     = ichan % MODULE_K_NPARALLEL;
      int iserial = ichan / MODULE_K_NPARALLEL;
      if      (isg == 0) adcs = adcs0[iserial];
      else if (isg == 1) adcs = adcs1[iserial];
      else if (isg == 2) adcs = adcs2[iserial];
      else if (isg == 3) adcs = adcs3[iserial];

      for (int idx = 0; idx < PACKET_K_NSAMPLES; idx++)
      {
          #define COLUMNS 0x20

          int adc = adcs[idx];
          int col = idx % COLUMNS;
          if (col == 0x0)     std::cout << std::setfill(' ') << std::hex << std::setw(3) << idx;
          std::cout << ' ' << std::setfill (' ') << std::hex << std::setw(3) << adc;
          if (col == (COLUMNS -1)) std::cout << std::endl;

          #undef COLUMNS
      }
   }

    return;
}


static void write_sizes_print (int ichan, int csize, int osize)
{
   std::cout << "Bit Size[0x" << std::setw (2) << std::hex << ichan << "] = 0x"
             << csize << ":ox" << osize << std::endl;
}


static void write_adcs_encode_chan (int ichan)
 {
 std::cout << "Encoding chan: " << std::hex << ichan << std::endl;
 }
/* ---------------------------------------------------------------------- */
#endif



