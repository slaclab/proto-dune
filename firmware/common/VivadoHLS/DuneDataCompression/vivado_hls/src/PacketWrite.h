
#include "Parameters.h"

class HeaderContext
{
public:
   HeaderContext () :
      m_hdx (0),
      m_edx (0)
   {
      return;
   }

   void reset ()
   {
      m_hdx = 0;
      m_edx = 0;
   }


   void addHeader (uint64_t hdr)
   {
      m_hdrsBuf[m_hdx++] = hdr;
   }

   void addException (ap_uint<6> excMask, ap_uint<10> nframe)
   {
      ap_uint<16> exc = (excMask, nframe);
      ap_int<2> shift =   m_edx & 0x3;

      if (shift == 0) m_excBuf[m_edx] = 0;

      m_excBuf[m_edx] |= static_cast<uint64_t>(exc << (shift<<4));

      if (~shift) m_edx += 1;
   }

   void commit (AxisOut  &axis, int &odx, bool write, uint32_t status)
   {
      for (int idx = 0; idx < m_hdx; idx++)
      {
         ::commit (axis, odx, write, m_hdrsBuf[idx], 0, 0);
      }


      for (int idx = 0; idx < m_edx; idx++)
      {
         ::commit (axis, odx, write, m_excBuf[idx], 0, 0);
      }

      // Compose the summary word
      uint64_t summary = (static_cast<uint64_t>(status) << 32) | (m_hdx << 16) | (m_edx << 0);
      ::commit (axis, odx, write, summary, 0, 0);
   }

public:
   int                                 m_hdx;   /*!< Header buffer index     */
   int                                 m_edx;   /*!< Exception buffer index  */
   uint64_t m_hdrsBuf[6 * PACKET_K_NSAMPLES];   /*!< Buffer for header words */
   uint64_t m_excBuf[(6 * PACKET_K_NSAMPLES)
                   / (sizeof (uint64_t)
                   /  sizeof (uint16_t))];     /*!< Buffer for exceptions    */
};
/* ------------------------------------------------------------------------- */


static void  write_packet (AxisOut                                                  &mAxis,
                           PacketContext                                           &pktCtx,
                           Histogram      const                   hists[MODULE_K_NCHANNELS],
                           Symbol_t       const syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                           ModuleIdx_t                                           moduleIdx);

typedef ap_uint<PACKET_B_NSAMPLES>     FrameIdx_t;
typedef ap_uint<MODULE_B_NCHANNELS> ChannelIdx_t;

static inline void transpose (Symbol_t dstSyms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                              Symbol_t const              srcSyms[MODULE_K_NCHANNELS],
                              int                                             iframe);

/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct WriteContext
 *  \brief  Structure to bind the write context with the output stream
 *
\* ---------------------------------------------------------------------- */
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
    int            odx; /*!< The output word index (count of output words)*/
    int            fdx; /*!< Frame index (count of frames)                */
    HeaderContext  hdr; /*!< The compressed header context                */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define WRITE_SYMS_PRINT 1
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
    ap_uint<22>  data; /*!< Size, in bits, of the encoded  data           */
    ap_uint<22>  over; /*!< Size, in bits, of the overflow symbols        */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if WRITE_SYMS_PRINT
/* ---------------------------------------------------------------------- */
static int List[] = {0};

static void
write_syms_print (Symbol_t const syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES])
{
   for (int ilist = 0; ilist < sizeof (List) / sizeof (List[0]); ilist++)
   {
      int ichan = List[ilist];

      std::cout << "Dumping Adc Channel: " << std::hex << std::setw (5)
       << ichan << std::endl;

      Symbol_t const *a = syms[ichan];

      for (int idx = 0; idx < PACKET_K_NSAMPLES; idx++)
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
#else
/* ---------------------------------------------------------------------- */

#define write_syms_print(_syms)
#define write_syms_bit_size_print(_ichan, _ebit_size)
#define write_syms_encode_chan(_ichan)
#define write_sizes_print(_ichan, csize, osize)

/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */

#include "AP-Encode.h"

static inline void
       write_syms (AxisOut                                             &mAxis,
                   int                                                   &mdx,
                   uint64_t                                             &data,
                   int                                                   &odx,
                   Symbol_t const syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                   Histogram   const                hists[MODULE_K_NCHANNELS]);

static __inline void   encodeN (APE_etx                               *etx,
                                Histogram const                     *hists,
                                Symbol_t  const  syms[][PACKET_K_NSAMPLES],
                                int                                  nsyms,
                                int                               nstreams,
                                int                                 nchans,
                                int                                  ichan);

static void             writeN (AxisOut                             &mAxis,
                                uint64_t                              &buf,
                                int                                   &mbx,
                                APE_etx                              etx[],
                                int                               nstreams,
                                int                                 nchans,
                                int                                 ichan);

static void               pack (AxisOut                            &mAxis,
                                uint64_t                             &buf,
                                int                                  &bdx,
                                BitStream64                           &bs);


static void write_encoded_data (AxisOut      &mAxis,
                                int            &mdx,
                                BitStream64     &bs);

static void    write_overflow  (AxisOut      &mAxis,
                                int            &mdx,
                                BitStream64     &bs);


// Define which channels to process, debug only
#define   NCHANS MODULE_K_NCHANNELS
#define BEG_CHAN  0
#define END_CHAN (BEG_CHAN + NCHANS)


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Writes one packets worth of data to the output stream
 *
 *   \param[out]     mAxis  The output data stream
 *   \param[ in]     wibId  The crate, slot, fiber # of the WIB fiber
 *   \param[ in] timestamp  The timestamp of the packet to be written
 *   \param[ in]      syms  The symbols (processed ADCs) to be compressed
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
static void  write_packet (AxisOut                                                   &mAxis,
                           PacketContext const                                      &pktCtx,
                           Histogram     const                    hists[MODULE_K_NCHANNELS],
                           Symbol_t      const  syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                           ModuleIdx_t                                            moduleIdx)
{
   #define COMPRESS_VERSION 0x10000000
   #define HLS INLINE off

   int   mdx = 0;
   uint64_t data;

   uint32_t frameId  = Frame::V0::identifier (Frame::V0::DataType::COMPRESSED);
   uint64_t hdr = Frame::V0::header (frameId);
   commit (mAxis, mdx, true, hdr,       0x2, 0);
   commit (mAxis, mdx, true, pktCtx.wibId,     0x0, 0);
   commit (mAxis, mdx, true, pktCtx.timestamp, 0x0, 0);

   write_syms (mAxis, mdx, data, mdx, syms, hists);

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



static void write_symsN (AxisOut                                     &mAxis,
                         int                                           &bdx,
                         uint64_t                                     &data,
                         int                                           &odx,
                         Symbol_t const syms[NPARALLEL][PACKET_K_NSAMPLES],
                         Histogram      const             hists[NPARALLEL],
                         int                                         ichan);

/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the symbols (processed ADcs) for all channels to the
 *         output stream
 *
 *  \param[in]     mAxis  The output stream
 *  \param[in:out]   mdx  The index into the ouput stream, mAxis
 *  \param[in:out]  data  The 64-bit data accumulation buffer
 *  \param[in:out]   odx  The 16-bit index into \a data
 *  \param[in]      syms  The symbols to encode
 *  \param[in:out] hists  The histograms of the symbols. These are transformed
 *                        into the cumulative distributions used to do
 *                        the actual encoding
 *
\* ---------------------------------------------------------------------- */
static inline void
       write_syms (AxisOut                                             &mAxis,
                   int                                                   &mdx,
                   uint64_t                                             &data,
                   int                                                   &odx,
                   Symbol_t const syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                   Histogram      const             hists[MODULE_K_NCHANNELS])
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE


   #define NSERIAL ((NCHANS + NPARALLEL -1)/NPARALLEL)



   uint64_t       buf;
   write_syms_print (syms);


   // ------------------------------------
   // Transform 64-bit index to bit index
   // ------------------------------------
   int bdx = mdx << 6;

   WRITE_SYMS_CHANNEL_LOOP:
   for (int ichan = 0; ichan < NCHANS; ichan += NPARALLEL)
   {

      //write_symsN (mAxis, bdx, data, odx, &syms[ichan], &hists[ichan], ichan);


      APE_etx                  etx[NPARALLEL];
      #pragma     HLS ARRAY_PARTITION variable=etx complete dim=1

      //ChannelSize   sizes[MODULE_K_NCHANNELS];

      write_syms_encode_chan (ichan);
      encodeN (etx,  hists,  syms, PACKET_K_NSAMPLES, NPARALLEL, NSERIAL, ichan);
      writeN  (mAxis,  buf,   bdx,               etx, NPARALLEL, NSERIAL, ichan);
   }

   return;
}
/* ---------------------------------------------------------------------- */


static void write_symsN (AxisOut                                     &mAxis,
                         int                                           &bdx,
                         uint64_t                                     &data,
                         int                                           &odx,
                         Symbol_t const syms[NPARALLEL][PACKET_K_NSAMPLES],
                         Histogram      const             hists[NPARALLEL],
                         int                                         ichan)
{
   #pragma HLS INLINE
   //#pragma HLS DATAFLOW

   ///#define NSERIAL ((NCHANS + NPARALLEL -1)/NPARALLEL)

   APE_etx                  etx[NPARALLEL];
   //ChannelSize   sizes[MODULE_K_NCHANNELS];
   #pragma     HLS ARRAY_PARTITION variable=etx complete dim=1

   uint64_t       buf;

   write_syms_encode_chan (ichan);
   encodeN (etx,  hists,  syms, PACKET_K_NSAMPLES, NPARALLEL, NSERIAL, ichan);
   writeN  (mAxis,  buf,   bdx,               etx, NPARALLEL, NSERIAL, ichan);

   return;
}


/* -------------------------------------------------------------------- *//*!
 *
 *  \brief Encode the \a ichan channels in parallel
 *
 *  \param[in:out]  etx  The vector of encoding contexts
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
static __inline void encodeN (APE_etx                              *etx,
                              Histogram const                    *hists,
                              Symbol_t  const syms[][PACKET_K_NSAMPLES],
                              int                                 nsyms,
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


      // ------------------------------------------------
      // Encode the histogram, bot syms and the overflow
      // ------------------------------------------------
      APE_encode (etx[idx], hists[ichan], syms[ichan], PACKET_K_NSAMPLES);

#if 0
      // -----------------------------------------
      // Executed only if APE_CHECKER is non-zero
      // -----------------------------------------
      while (1)
      {
         bool failure = encode_check (etx[idx], hists[ichan], syms[ichan]);
         if (failure) APE_encode (etx[idx], hists[ichan], syms[ichan], PACKET_K_NSAMPLES);
         else         break;
      }
#endif


      write_sizes_print (ichan, etx[idx].ba.m_idx, etx[idx].oa.m_idx);


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
static void writeN (AxisOut                        &mAxis,
                    uint64_t                         &buf,
                    int                              &bdx,
                    APE_etx                         etx[],
                    int                          nstreams,
                    int                            nchans,
                    int                             ichan)
{
   #pragma HLS INLINE


   WRITE_CHANNEL_LOOP:
   for (int idx = 0;  idx < nstreams; idx++)
   {
      //#pragma HLS PIPELINE

      // Output the compressed data and ADC overflow bit streams
      pack (mAxis, buf, bdx, etx[idx].ba);
      pack (mAxis, buf, bdx, etx[idx].oa);

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
                  uint64_t                          &bsbuf,
                  int                                 &bdx,
                  BitStream64                          &bs)
{
    #pragma HLS INLINE off

   // Get the index of the first bit in this stream
   ap_uint<6> bbeg = bdx;
   int           mdx = bdx >> 6;
   ap_uint<6> rshift = bdx;

   ap_uint<128> b128 = bsbuf;
   // ----------------------------------------------------------------
   // Need to merge the shards of
   //     a) the buffered output stream --        buf
   // and b) the leftovers from the input stream  bsbuf of bsvld bits
   // ---------------------------------------------------------------


   // ----------------------------------
   // Count the bits that will be output
   // when the loop is completed.
   // -----------------------------------
   uint64_t data;
   int bleft = bs.m_idx;
   bdx      +=    bleft;


    AxisOut_t out;
    out.keep = 0xFF;
    out.strb = 0x0;
    out.user = 0;
    out.last = 0;
    out.id   = 0x0;
    out.dest = 0x0;

   PACK_LOOP:
   while (bleft > 0)
   {
      #pragma HLS LOOP_TRIPCOUNT min=32 max=48 avg=32

      // Make room. get and insert the new 64-bts worth of data
      b128 <<= 64;
      bs.m_out.read_nb (data);
      b128  |= data;

      // Get the top 64 bits
      data   = b128 >> rshift;
      pack_data_print (bleft, data, rshift, 0);

      out.data = data;
      commit (mAxis, mdx, out);

      //commit (mAxis, mdx, true, data, 0, 0);
      bleft -= 64;
   }

   // Preserve the lower 64 bits
   // Only rshift are meaningful
   // They will eventually naturally be shifted out.
   bsbuf = data >> (-bleft);

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
 *         to restore the overflow symbols
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
#endif
