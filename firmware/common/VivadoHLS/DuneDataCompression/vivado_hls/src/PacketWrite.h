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
 *   In addition, the code in writeSymsN is structured to dataflow
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
#define WRITE_SYMS_PRINT 1


#ifdef __SYNTHESIS__
#undef  CHECKER
#undef  WRITE_SYMS_PRINT
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

static void write_syms (AxisBitStream                               &bAxis,
                        AxisOut                                     &mAxis,
                        ChannelOffset_t        offsets[MODULE_K_NCHANNELS],
                        CompressionContext                        &cmpCtx);

static inline void
           write_syms4 (AxisBitStream                              &bAxis,
                        AxisOut                                    &mAxis,
                        ChannelOffset_t                         offsets[],
                        Symbol_t                 syms0[PACKET_K_NSAMPLES],
                        Symbol_t                 syms1[PACKET_K_NSAMPLES],
                        Symbol_t                 syms2[PACKET_K_NSAMPLES],
                        Symbol_t                 syms3[PACKET_K_NSAMPLES],
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
                              Symbol_t        syms0[PACKET_K_NSAMPLES],
                              Symbol_t        syms1[PACKET_K_NSAMPLES],
                              Symbol_t        syms2[PACKET_K_NSAMPLES],
                              Symbol_t        syms3[PACKET_K_NSAMPLES],
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
                                Symbol_t          syms[PACKET_K_NSAMPLES],
                                int                                nsyms);

static void               pack (AxisBitStream                      &baxis,
                                AxisOut                            &mAxis,
                                OStream                           &osream);

/* ---------------------------------------------------------------------- */
#if WRITE_SYMS_PRINT
/* ---------------------------------------------------------------------- */

static void write_syms_print  (Symbol_t const syms0[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                               Symbol_t const syms1[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                               Symbol_t const syms2[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                               Symbol_t const syms3[MODULE_K_NSERIAL][PACKET_K_NSAMPLES]);

static void write_sizes_print (int ichan, int csize, int osize);

/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */

#define write_syms_print(_syms0, _syms1, _syms2, _syms3)
#define write_syms_bit_size_print(_ichan, _ebit_size)
#define write_syms_encode_chan(_ichan)
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
 *   \param[out]     mAxis  The output data stream
 *   \param[ in]    pktCtx  The per packet context
 *   \param[ in]      syms  The symbols (processed ADCs) to be compressed
 *   \param[ in]     hists  The histograms used to compose the compression
 *                          tables
 *   \param[ in] moduleIdx  The module idx of this group of channels
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

   ///// Histogram Hists[MODULE_K_NCHANNELS];
   ///// PRAGMA_HLS(HLS ARRAY_PARTITION  variable=Hists block factor=NHISTPORTS dim=1)


   //// Symbol_t   Syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES];
   ////  PRAGMA_HLS(HLS ARRAY_PARTITION  variable=Syms  block factor=NADCPORTS  dim=1)

   ChannelOffset_t    offsets[MODULE_K_NCHANNELS+2];
   write_syms (bAxis, mAxis, offsets, cmpCtx);

   odx = (bAxis.m_idx + 63) >> 6;


   offsets[NCHANS]   = bAxis.m_idx;
   offsets[NCHANS+1] = 0;
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


   lclMonitor.nbytes    += odx << 6;
   lclMonitor.npackets  += 1;
   lclMonitor.npromoted += 1;

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
 *  \brief Writes the symbols (processed ADcs) for all channels to the
 *         output stream
 *
 *  \param[in:out] bAxis
 *  \param[in]     mAxis  The output stream
 *  \param[out]  offsets  The starting bit offset for each channel
 *  \param[in]      syms  The symbols to encode
 *  \param[in:out] hists  The histograms of the symbols. These are transformed
 *                        into the cumulative distributions used to do
 *                        the actual encoding
 *
\* ---------------------------------------------------------------------- */
static inline void
       write_syms (AxisBitStream                                       &bAxis,
                   AxisOut                                             &mAxis,
                   ChannelOffset_t                offsets[MODULE_K_NCHANNELS],
                   CompressionContext                                 &cmpCtx)
{
   #pragma HLS INLINE

   // Diagnostic printout
   write_syms_print (cmpCtx.syms.sg0,
                     cmpCtx.syms.sg1,
                     cmpCtx.syms.sg2,
                     cmpCtx.syms.sg3);

   WRITE_SYMS_CHANNEL_LOOP:
   for (int isg = 0; isg < NSERIAL; isg++)
   {
      write_syms4 (bAxis, mAxis, offsets,
                   cmpCtx. syms.sg0[isg],
                   cmpCtx. syms.sg1[isg],
                   cmpCtx. syms.sg2[isg],
                   cmpCtx. syms.sg3[isg],
                   cmpCtx.hists.sg0[isg],
                   cmpCtx.hists.sg1[isg],
                   cmpCtx.hists.sg2[isg],
                   cmpCtx.hists.sg3[isg],
                   isg * 4);
   }

   bAxis.flush (mAxis);

   return;
}
/* -------------------------------------------------------------------- */




/* -------------------------------------------------------------------- *//*!
 *
 *   \brief  Writes out the encoded output for NPARALLEL channels
 *
\* -------------------------------------------------------------------- */
static inline void
       write_syms4 (AxisBitStream                              &bAxis,
                    AxisOut                                    &mAxis,
                    ChannelOffset_t                         offsets[],
                    Symbol_t                 syms0[PACKET_K_NSAMPLES],
                    Symbol_t                 syms1[PACKET_K_NSAMPLES],
                    Symbol_t                 syms2[PACKET_K_NSAMPLES],
                    Symbol_t                 syms3[PACKET_K_NSAMPLES],
                    Histogram                                 &hist0,
                    Histogram                                 &hist1,
                    Histogram                                 &hist2,
                    Histogram                                 &hist3,
                    int                                        ichan)
{
   #pragma HLS INLINE off
   #pragma HLS DATAFLOW

   APE_etxOut            etxOut[NPARALLEL];
   #pragma HLS ARRAY_PARTITION variable=etxOut complete dim=1

#if 0
   {
      #pragma HLS DATAFLOW

      Histogram Hists[NPARALLEL];
      #pragma HLS ARRAY_PARTITION variable=Hists complete

      Symbol_t Syms[NPARALLEL][PACKET_K_NSAMPLES];
      #pragma HLS ARRAY_PARTITION variable=Syms complete dim=1

      copyN            (Hists, &hists[ichan], Syms,   &syms[ichan]);
      encodeN (etxOut,  Hists, Syms, PACKET_K_NSAMPLES, NPARALLEL, NSERIAL, ichan);
   }
#else

    encode4 (etxOut,
             hist0, hist1, hist2, hist3,
             syms0, syms1, syms2, syms3,
             ichan);
#endif

   writeN  (bAxis, mAxis, &offsets[ichan], etxOut, NPARALLEL, NSERIAL, ichan);
}
/* -------------------------------------------------------------------- */



/* -------------------------------------------------------------------- */
static __inline void encode4 (APE_etxOut                          *etx,
                              Histogram                        &hists0,
                              Histogram                        &hists1,
                              Histogram                        &hists2,
                              Histogram                        &hists3,
                              Symbol_t        syms0[PACKET_K_NSAMPLES],
                              Symbol_t        syms1[PACKET_K_NSAMPLES],
                              Symbol_t        syms2[PACKET_K_NSAMPLES],
                              Symbol_t        syms3[PACKET_K_NSAMPLES],
                              int                               ichan)
{
   #pragma HLS INLINE off
   #pragma HLS DATAFLOW


   encode (etx[0], hists0, syms0, PACKET_K_NSAMPLES);
   encode (etx[1], hists1, syms1, PACKET_K_NSAMPLES);
   encode (etx[2], hists2, syms2, PACKET_K_NSAMPLES);
   encode (etx[3], hists3, syms3, PACKET_K_NSAMPLES);


   #if CHECKER
      // -----------------------------------------
      // Executed only if APE_CHECKER is non-zero
      // -----------------------------------------

      for (int idx = 0; idx < 4; idx++)
      {
         write_sizes_print (ichan, etx[idx].ba.m_cidx, etx[idx].ha.m_cidx);
      }

      bool failure0 = encode_check (etx[0], hists0, syms0);
      if  (failure0)    APE_encode (etx[0], hists0, syms0, PACKET_K_NSAMPLES);

      bool failure1 = encode_check (etx[1], hists1, syms1);
      if  (failure1)    APE_encode (etx[1], hists1, syms1, PACKET_K_NSAMPLES);

      bool failure2 = encode_check (etx[2], hists2, syms2);
      if  (failure2)    APE_encode (etx[2], hists2, syms2, PACKET_K_NSAMPLES);

      bool failure3 = encode_check (etx[3], hists3, syms3);
      if  (failure3)    APE_encode (etx[3], hists3, syms3, PACKET_K_NSAMPLES);

      if (failure0 || failure1 || failure2 || failure3)
      {
         exit (-1);
      }
   #endif
}
/* -------------------------------------------------------------------- */


#if 0
/* -------------------------------------------------------------------- *//*!
 *
 *  \brief Encode the \a ichan channels in parallel
 *
 *  \param[in:out]  etx  The vector of encoded outputs
 *  \param[in:out] hist  The vector of histograms to use generate
 *                       compression tables
 *  \param[in]     syms  The vector of symbols to encode
 *  \param[in]    nsyms  The number of symbols to encode for each channel
 *  \param[in] nstreams  The number of parallel streams
 *  \param[in]   nchans  The number of channels per stream
 *  \param[in]    ichan  The channel number of the first channel to
 *                       be encoded
 *
\* -------------------------------------------------------------------- */
static __inline void encodeN (APE_etxOut                               *etx,
                              Histogram                              *hists,
                              Symbol_t             syms[][PACKET_K_NSAMPLES],
                              int                                     nsyms,
                              int                                  nstreams,
                              int                                    nchans,
                              int                                     ichan)
{
   #pragma HLS INLINE off
   #pragma HLS DATAFLOW

   ////write_syms_encode_chan (ichan);

   ENCODE_N_ENCODELOOP:
   for (int idx = 0; idx < NPARALLEL; idx++)
   {
      //// #pragma HLS PIPELINE
      #pragma HLS UNROLL

      /////HISTOGRAM_statement (hists[idx].print ());
      encode (etx[idx], hists[idx], syms[idx], PACKET_K_NSAMPLES);




      // -------------------------------------------------------
      // Increment the channels by the number of channels
      // and check if done with all the channels.  This check is
      // necessary since the number of channels may not be an
      // integer multiple of the number of streams.
      // -------------------------------------------------------
      ///ichan     += 1

#if 0
      if (ichan >= NCHANS)
      {
         break;
      }
#endif

   }


   return;
}
/* ---------------------------------------------------------------------- */
#endif


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Wrapper to encapsulate any per channel encoding stuff
 *
 *   \param[out]   etx  The encoding context for this channel
 *   \param[ in]  hist  The encoding frequency distribution
 *   \param[ in]  syms  The symbols to be encoded
 *   \param[ in] nsyms  The number of symbols to encoded
 *
\* ---------------------------------------------------------------------- */
static void encode (APE_etxOut                        &etx,
                    Histogram                        &hist,
                    Symbol_t       syms[PACKET_K_NSAMPLES],
                    int                              nsyms)
{
  #pragma HLS INLINE

   APE_encode (etx, hist, syms, PACKET_K_NSAMPLES);

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
static void writeN (AxisBitStream                  &bAxis,
                    AxisOut                        &mAxis,
                    ChannelOffset_t             offsets[],
                    APE_etxOut                      etx[],
                    int                          nstreams,
                    int                            nchans,
                    int                             ichan)
{
  #pragma HLS INLINE off


   WRITE_CHANNEL_LOOP:
   for (int idx = 0;  idx < nstreams; idx++)
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
    #pragma HLS INLINE  off
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
   PACK_LOOP:
   for (int idx = 0; idx < cnt/2; idx++)
   {
      #pragma HLS LOOP_TRIPCOUNT min=12 max=48 avg=16

      #pragma HLS PIPELINE II=2     /// STRIP -- this caused problems at one time
      #pragma HLS UNROLL factor=1

      // Make room. get and insert the new 64-bts worth of data
      data = ostream.read ();
      bAxis.insert (mAxis, data, 64);

      // Make room. get and insert the new 64-bts worth of data
      data = ostream.read ();
      bAxis.insert (mAxis, data, 64);
   }

   if (cnt&1)
   {
      data = ostream.read ();
      bAxis.insert (mAxis, data, 64);
   }

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

   int idz = idy & 0xf;
   if (idz == 0) { std::cout << "Sym[" << std::hex << std::setw (4) << idy << "] "; }
   if (idy == 0) { Adcs[0]   = sym; }
   else          { Adcs[idz] = Adcs[(idz-1) & 0xf] + restore (sym); }


   std::cout << std::hex << std::setw (3) << sym;

   if (idz == 0xf)
   {
      std::cout << "  ";
      for (int i = 0; i < 16; i++)  std::cout << std::hex << std::setw (4) << Adcs[i];
      std::cout << std::endl;
   }
   ///std::cout << "Sym[" << idy << "] = " << syms[idy] << " : " << dsyms[idy] <<  std::hex << std::setw(5) << std::endl);

   return;
}
)

static bool decode_data (uint16_t      dsyms[PACKET_K_NSAMPLES],
                         BFU                              &ebfu,
                         uint64_t  const                  *ebuf,
                         BFU                              &obfu,
                         uint64_t const                   *obuf,
                         Histogram const                  &hist,
                         Symbol_t const syms[PACKET_K_NSAMPLES])
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
   int       prv;
   int   novrflw;
   int       sym;
   int oposition = hist_decode (bins, &nbins, &sym, &novrflw, obfu, obuf);


   APD_dtx dtx;
   APD_start (&dtx, ebuf, 0);


   int Nerrs = 0;

   for (int idy = 0; ; idy++)
   {
      dsyms[idy] = sym;
      APD_dumpStatement (print_decoded (sym, idy));

      if (dsyms[idy] != syms[idy])
      {
         std::cout << std::endl << "Error @" << std::hex << idy << std::endl;
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
         int ovr = _bfu_extractR (obfu, obuf, oposition, novrflw);
         sym     =  nbins + ovr;
      }
   }


   APD_finish (&dtx);
   return Nerrs != 0;
}



static int copy (uint64_t *buf, OStream &ostream)
{
   // Copy the data into a temporary bit stream
   int idx = 0;
   while (1)
   {
      uint64_t dat;
      bool okay = ostream.read_nb (buf[idx]);
      if (!okay) break;
      idx += 1;
   }

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
   for (int idx = 0; idx < nbuf; idx++)
   {
      ostream.write_nb (buf[idx]);
   }

   return;
}

bool encode_check (APE_etxOut &etx, Histogram const &hist, Symbol_t const syms[PACKET_K_NSAMPLES])
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


   uint16_t dsyms[PACKET_K_NSAMPLES];
   bool failure = decode_data (dsyms, ebfu, ebuf, hbfu, hbuf, hist, syms);

   restore (etx.ha, hbuf, hcnt);
   restore (etx.ba, ebuf, ecnt);


   return failure;
}
#endif


/* ---------------------------------------------------------------------- */
#if WRITE_SYMS_PRINT
/* ---------------------------------------------------------------------- */
static int List[] = {0};

static void
  write_syms_print  (Symbol_t const syms0[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                     Symbol_t const syms1[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                     Symbol_t const syms2[MODULE_K_NSERIAL][PACKET_K_NSAMPLES],
                     Symbol_t const syms3[MODULE_K_NSERIAL][PACKET_K_NSAMPLES])
{
   for (int ilist = 0; ilist < sizeof (List) / sizeof (List[0]); ilist++)
   {
      int ichan = List[ilist];

      std::cout << "Encoded Symbols[" << std::hex << std::setw (2) << ichan << ']' << std::endl;

      Symbol_t const *syms;
      int isg     = ichan % MODULE_K_NPARALLEL;
      int iserial = ichan / MODULE_K_NPARALLEL;
      if      (isg == 0) syms = syms0[iserial];
      else if (isg == 1) syms = syms1[iserial];
      else if (isg == 2) syms = syms2[iserial];
      else if (isg == 3) syms = syms3[iserial];

      for (int idx = 0; idx < PACKET_K_NSAMPLES; idx++)
      {
          #define COLUMNS 0x20

          int sym = syms[idx];
          int col = idx % COLUMNS;
          if (col == 0x0)     std::cout << std::setfill(' ') << std::hex << std::setw(3) << idx;
          std::cout << ' ' << std::setfill (' ') << std::hex << std::setw(3) << sym;
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


static void write_syms_encode_chan (int ichan)
 {
 std::cout << "Encoding chan: " << std::hex << ichan << std::endl;
 }

/* ---------------------------------------------------------------------- */
#endif


#if defined (OLD_TEST_OF_WRITE_PACKET_DATAFLOW)
static void write_packet (AxisOut &mAxis)
{
   int odx = 0;
   commit (mAxis, odx, true, (0xdeadbeefLL << 32) | 1 , 0, 0);
   commit (mAxis, odx, true, (0xaaaaaaaaLL << 32) | 2 , 0, 0);
   commit (mAxis, odx, true, (0xbbbbbbbbLL << 32) | 3 , 0, 0);
}


static void write_packet (AxisOut &mAxis, ReadStatus_t status)
{
   int odx = 0;
   uint64_t tmp = status;
   commit (mAxis, odx, true, (0xabadbabeLL << 32) | tmp, 0, 0);

   commit (mAxis, odx, true, (0xdeadbeefLL << 32) | 0x31, 0, 0);
   commit (mAxis, odx, true, (0xaaaaaaaaLL << 32) | 0x32, 0, 0);
   commit (mAxis, odx, true, (0xbbbbbbbbLL << 32) | 0x33, 0, 0);
}

static void  write_packet (AxisOut                                                   &mAxis,
                           Symbol_t      const  syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES])
{
   int odx = 0;
   commit (mAxis, odx, true, (0xdeadbeefLL << 32) | 4 , 0, 0);
   commit (mAxis, odx, true, (0xaaaaaaaaLL << 32) | 5 , 0, 0);
   commit (mAxis, odx, true, (0xbbbbbbbbLL << 32) | 6 , 0, 0);
}

static void  write_packet (AxisOut                                                   &mAxis,
                           Histogram     const                    hists[MODULE_K_NCHANNELS],
                           Symbol_t      const  syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES])
{
   int odx = 0;
   commit (mAxis, odx, true, (0xdeadbeefLL << 32) | 0x11 , 0, 0);
   commit (mAxis, odx, true, (0xaaaaaaaaLL << 32) | 0x12 , 0, 0);
   commit (mAxis, odx, true, (0xbbbbbbbbLL << 32) | 0x13 , 0, 0);


   for (int idx = 0; idx < 32; idx++)
   {
      commit (mAxis, odx, true, hists[0].m_bins[idx], 0, 0);
   }

   for (int idx = 0; idx < 128; idx++)
   {
      commit (mAxis, odx, true, syms[0][idx], 0, 0);
   }
}

static void write_packet (AxisOut &mAxis, PacketContext const &pktCtx)
{
   int odx = 0;
   pktCtx.commit (mAxis, odx,  true, true, pktCtx.m_status);
   pktCtx.commit (mAxis, odx, false, true, pktCtx.m_status);


   uint64_t w64 = pktCtx.m_cedx;
   w64 <<= 16;
   w64  |= pktCtx.m_chdx;
   w64  |= 0xd0000000LL << 32;
   commit (mAxis, odx, true, w64,                         0, 0);
   commit (mAxis, odx, true, (0x11111111LL << 32) | pktCtx.m_status, 0, 0);
   commit (mAxis, odx, true, (0xabadcafeLL << 32) | 0xab, 0, 0);
   commit (mAxis, odx, true, (0xdeadbeefLL << 32) |    1, 0, 0);
   commit (mAxis, odx, true, (0xaaaaaaaaLL << 32) |    2, 0, 0);
   commit (mAxis, odx, true, (0xbbbbbbbbLL << 32) |    3, 0, 0);

   for (int idx = 0; idx < 7; idx++)
    {
       commit (mAxis, odx, true, pktCtx.m_hdrsBuf[idx], 0, 0);
    }

}
#endif
