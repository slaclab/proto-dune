#ifndef _WIBFRAME_PROCESS_H_
#define _WIBFRAME_PROCESS_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     WibFrame-Process.h
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
 *  2018.04.23
 *
 *  @par Last commit:
 *  \$Date: $ by \$Author: $.
 *
 *  @par Credits:
 *  SLAC
 *
 *  @par
 *  This accepts one WIB frame, does the necessary decoding and commits it
 *  to the output packet.  The \i necessary \i decoding includes
 *
 *    -# Processing the header for the expected values.
 *    -# Extracting the ADCs from the cold data stream
 *    -# Processing the ADCs into symbols
 *
 *  @par Headers
 *  There are 6 x 64-bit header words, each composed of a variety of fields.
 *  Many of these fields have static values (\e e.g version number, WIB
 *  crate.slot.fiber, \e etc) or values that are predictable from the
 *  previous WIB frame (\e e.g. the timestamp, the 2 convert counts).
 *
 *  These headers are checked with the excepted values,  The intention is
 *  that only expections will be written to the output stream.  Note that
 *  if this is not done, the headers will contribute about 25-50% of the
 *  data volume for conpression factors of 2 - 4.
 *
 *  @par Extracting the ADCs
 *  The ADCs are not packed in a straight-forward manner in the cold data
 *  streams and need to be reassembled.  See the comments in the
 *  extract_adcX methods.
 *
 *  @par Processing the ADCs into symbols
 *  The raw ADCs are not directly compressed, but a rather \e processed
 *  into what are called \a symbols.  The transformation of an ADC to
 *  a symbol is a reversiable function (so that the ADCs may be
 *  reconstituted when decoded).  The exact transformation will likely
 *  change over time as one gains experience.
 *
 *  The symbols are stored in an 2D output array which has the effect
 *  of transposing the time and channel order so that the compressor
 *  can work on a contigious array of one channel's worth of data.
 *
 *  Histograms of the symbols are also formed.  These histograms will
 *  later be transformed into the compression tables.
 *
\* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\

   HISTORY
   -------

   DATE       WHO WHAT
   ---------- --- ---------------------------------------------------------
   2018.04.23 jjr Created

\* ---------------------------------------------------------------------- */

#include "Parameters.h"
#include "DuneDataCompressionHistogram.h"


//// STRIP typedef Histogram::Symbol_t Symbol_t;

typedef AdcIn_t Symbol_t;




/* ======================================================================== */
/* OPTIONAL PRINT METHODS                                                   */
/* These are neutered if doing SYNTHESIS                                    */
/* ------------------------------------------------------------------------ */
#if     !defined(__SYNTHESIS__)
#define PROCESS_PRINT      0
#define PROCESS_PRINT_ADCS 0
#else
#define PROCESS_PRINT_ADCS 0
#define PROCESS_PRINT      0
#endif


#if    PROCESS_PRINT_ADCS
static void print_adcs (AdcIn_t const adcs0[PACKET_K_NSAMPLES],
                        AdcIn_t const adcs1[PACKET_K_NSAMPLES],
                        AdcIn_t const adcs2[PACKET_K_NSAMPLES],
                        AdcIn_t const adcs3[PACKET_K_NSAMPLES],
                        int                             iframe)

#else
#define print_adcs(_adcs0, _adcs1, _adcs2, _adcs3, _iframe)
#endif


/* ---------------------------------------------------------------------------- *//*!
 *
 *  \class PacketContext
 *  \brief This is write portion of the per packet information.
 *
\* ---------------------------------------------------------------------------- */
class PacketContext
{
public:
   const static int MaxExc = 32;

public:
   PacketContext ()
   {
      //// STRIP #pragma HLS ARRAY_PARTITION variable=m_excsBuf cyclic factor=8 dim=1
      //// STRIP #pragma HLS ARRAY_PARTITION variable=m_hdrsBuf cyclic factor=8 dim=1
      return;
   }



   void commit (AxisOut  &axis, int &odx, bool first, bool write, uint32_t status) const
   {
      #pragma HLS INLINE
      #pragma HLS PIPELINE


      const static int HeaderFmt   = 3; // Generic Header Format = 3
      const static int HdrsRecType = 1; // TpcStream Record Subtype = 1;
      const static int HdrFixedN64 = 1; // Record Header

      // -------------------------------------------------------------------------------
      // Compose the record header word
      //    status(32) | exception count(8) | length (16) | RecType(4) | HeaderFormat(4)
      // -------------------------------------------------------------------------------
      int cedx = (m_cedx + 3) >> 2;
      uint64_t recHeader =  (static_cast<uint64_t>(status) << 32)
                         |  (m_cedx                        << 24)
                         | ((cedx + m_chdx + HdrFixedN64)  <<  8)
                         |  (HdrsRecType                   <<  4)
                         |  (HeaderFmt                     <<  0);
      ::commit (axis, odx, write, recHeader,  first ? (1 << (int)AxisUserFirst::Sof) : 0, 0);


      // Write out the headers
      int chdx = m_chdx;
      HEADER_CONTEXT_HDR_LOOP:
      for (int idx = 0; idx < chdx; idx++)
      {
         #pragma HLS LOOP_TRIPCOUNT min=6 avg=6 max=1024
         #pragma HLS PIPELINE
         ::commit (axis, odx, write, m_hdrsBuf[idx], 0, 0);
      }

      // Write out the exception buffer
      HEADER_CONTEXT_EXC_LOOP:
      for (int idx = 0; idx < cedx; idx++)
      {
         #pragma HLS LOOP_TRIPCOUNT min=0 avg=1 max=256
         #pragma HLS PIPELINE
         ::commit (axis, odx, write, m_excsBuf[idx], 0, 0);
      }

      return;
   }


public:
   int                                 m_chdx;  /*!< Header buffer index      */
   int                                 m_cedx;  /*!< Exception buffer index   */
   ReadStatus_t                      m_status;  /*!< Summary status           */

   uint64_t m_hdrsBuf[7+6*MaxExc];              /*!< Buffer for header words  */
   uint64_t m_excsBuf[  MaxExc/4];              /*!< Buffer for 8 exceptions  */

#if 0 //// STRIP
    uint64_t m_hdrsBuf[ 6 * PACKET_K_NSAMPLES];   /*!< Buffer for header words*/
    uint64_t m_excsBuf[(6 * PACKET_K_NSAMPLES)
                    / (sizeof (uint64_t)
                    /  sizeof (uint16_t))];     /*!< Buffer for exceptions    */
#endif
};
/* ------------------------------------------------------------------------- */

/* ======================================================================== *//*!
 *
 *  \class HeaderContextLcl
 *  \brief Captures the context of per packet information.
 *
 *   Since the inforation gathered by this class is involved in a dataflow
 *   routine, the write part must be segregated from the read part.  This
 *   class contains information that is read/write.  The information is then
 *   transferred to the dataflow equivalent class, the key being that this
 *   receiving class is only written to in this part of the dataflow.
 *
\* ------------------------------------------------------------------------ */
class HeaderContextLcl
{
   public:
      HeaderContextLcl ()
      {
         return;
      }

      void reset ()
      {
         #pragma HLS INLINE
         m_hdx = 0;
         m_edx = 0;
      }

      void addHeader (uint64_t *hdrsBuf, uint64_t hdr)
      {
         #pragma HLS INLINE
         hdrsBuf[m_hdx] = hdr;
         m_hdx += 1;
      }

      void addException (uint64_t *excBuf, ap_uint<6> excMask, ap_uint<10> nframe)
      {
         #pragma HLS INLINE
         //// STRIP #pragma HLS PIPELINE

         ap_uint<16> exc = (excMask, nframe);
         ap_int<2> shift =  m_edx & 0x3;


         uint64_t    exc64 = exc;
         ap_uint<6>  shf64 = shift;
         if (shift == 0) m_tmp  = exc64;
         else            m_tmp |= exc64 << (shf64<<4);

         if (~shift == 0)
         {
            excBuf[m_edx>>2] = m_tmp;
         }

         m_edx        += 1;

      }

      void evaluate (uint64_t          *hdrBuf,
                     uint64_t          *excBuf,
                     ReadStatus_t const status,
                     int                iframe,
                     uint64_t             wib0,
                     uint64_t             wib1,
                     uint64_t             cd00,
                     uint64_t             cd01,
                     uint64_t             cd10,
                     uint64_t             cd11)
      {
         #pragma HLS INLINE
         #pragma HLS PIPELINE

         if (m_edx >= PacketContext::MaxExc) return;

         ap_uint<6> exceptions = 0;
         if (ReadStatus::isWibHdr0Bad (status))
         {
            addHeader (hdrBuf, wib0);
            exceptions.set (0);
         }

         if (ReadStatus::isWibHdr1Bad (status))
         {
            addHeader (hdrBuf, wib1);
            exceptions.set (1);
         }

         if (ReadStatus::isCd0Hdr0Bad (status))
         {
            addHeader (hdrBuf, cd00);
            exceptions.set (2);
         }

         if (ReadStatus::isCd0Hdr1Bad (status))
         {
            addHeader (hdrBuf, cd01);
            exceptions.set(3);
         }

         if (ReadStatus::isCd1Hdr0Bad (status))
         {
            addHeader (hdrBuf, cd10);
            exceptions.set (4);
         }

         if (ReadStatus::isCd1Hdr1Bad (status))
         {
            addHeader (hdrBuf, cd11);
            exceptions.set (5);
         }

         if (exceptions)
         {
            addException (excBuf, exceptions, iframe);
         }
      }


   public:
      int                                 m_hdx;   /*!< Header buffer index     */
      int                                 m_edx;   /*!< Exception buffer index  */
      ReadStatus_t                     m_status;   /*!< Status summary          */
      uint64_t                            m_tmp;   /*< Accumulate 4 16-bit exc  */
};
/* ---------------------------------------------------------------------------- */




struct Adcs
{
   AdcIn_t  sg0[MODULE_K_NSERIAL][PACKET_K_NSAMPLES]; /*!< 0, 4,  8, ... */
   AdcIn_t  sg1[MODULE_K_NSERIAL][PACKET_K_NSAMPLES]; /*!< 1, 5,  9, ... */
   AdcIn_t  sg2[MODULE_K_NSERIAL][PACKET_K_NSAMPLES]; /*!< 2, 6, 10, ... */
   AdcIn_t  sg3[MODULE_K_NSERIAL][PACKET_K_NSAMPLES]; /*!< 3, 7, 11, ... */
};



struct Histograms
{
public:
   Histogram sg0[MODULE_K_NSERIAL];  /*!< 0, 4,  8, ... */
   Histogram sg1[MODULE_K_NSERIAL];  /*!< 1, 5,  9, ... */
   Histogram sg2[MODULE_K_NSERIAL];  /*!< 2, 6, 10, ... */
   Histogram sg3[MODULE_K_NSERIAL];  /*!< 3, 7, 11, ... */ 

public:
   HISTOGRAM_statement (
   void set_ids (int nparallel)
   {
      Histogram::set_ids (sg0, sizeof (sg0) / sizeof (sg0[0]), 0, nparallel);
      Histogram::set_ids (sg1, sizeof (sg1) / sizeof (sg1[0]), 1, nparallel);
      Histogram::set_ids (sg2, sizeof (sg2) / sizeof (sg2[0]), 2, nparallel);
      Histogram::set_ids (sg3, sizeof (sg3) / sizeof (sg3[0]), 3, nparallel);
   }
   )
};

/* ------------------------------------------------------------------------ *//*!
 *
 * \brief The compression context
 *
 *  This contains the information needed to compression the data
 *
\* ------------------------------------------------------------------------ */
struct CompressionContext
{
   Adcs        adcs;
   Histograms hists;
};
/* ------------------------------------------------------------------------ */



/* ======================================================================== */
/* Local Prototypes                                                         */
/* ------------------------------------------------------------------------ */

typedef ap_uint<48>        Adc48_t;


static inline void        process4 (AdcIn_t   adcs0[PACKET_K_NSAMPLES],
                                    AdcIn_t   adcs1[PACKET_K_NSAMPLES],
                                    AdcIn_t   adcs2[PACKET_K_NSAMPLES],
                                    AdcIn_t   adcs3[PACKET_K_NSAMPLES],
                                    Histogram                    &hist0,
                                    Histogram                    &hist1,
                                    Histogram                    &hist2,
                                    Histogram                    &hist3,
                                    Histogram               histsLcl[4],
                                    AdcIn_t                      prv[4],
                                    Adc48_t                      adcs48,
                                    int                          iframe,
                                    int                           ichan);

static inline AdcIn_t extract_adc0 (Adc48_t                       adcs4);
static inline AdcIn_t extract_adc1 (Adc48_t                       adcs4);
static inline AdcIn_t extract_adc2 (Adc48_t                       adcs4);
static inline AdcIn_t extract_adc3 (Adc48_t                       adcs4);

#if PROCESS_PRINT
static void    process4_adcs_print (int                            oidx,
                                    int                           ichan,
                                    AdcIn_t                        adc0,
                                    AdcIn_t                        adc1,
                                    AdcIn_t                        adc2,
                                    AdcIn_t                        adc3);

static void process4_adcs_prv_sym_print
                (int     oidx, int    ichan,
                 AdcIn_t adc0, AdcIn_t prv0,  Histogram::Symbol_t sym0,
                 AdcIn_t adc1, AdcIn_t prv1,  Histogram::Symbol_t sym1,
                 AdcIn_t adc2, AdcIn_t prv2,  Histogram::Symbol_t sym2,
                 AdcIn_t adc3, AdcIn_t prv3,  Histogram::Symbol_t sym3);
#else
#define process4_adcs_print(_oidx,_ichan,_adc0,_adc1,_adc2,_adc3)


#define process4_adcs_prv_sym_print(_oidx, _ichan,        \
                                    _adc0, _prv0, _sym0,  \
                                    _adc1, _prv1, _sym1,  \
                                    _adc2, _prv2, _sym2,  \
                                    _adc3, _prv3, _sym3)
#endif
/* ======================================================================== */




/* ====================================================================+== */
/* Implementation                                                          */
/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Processes 1 WIB frame, extracting the headers and unpacking the
 *         ADCs
 *
 *  \param[in]      frame  The target WIB frame
 *  \param[in]     iframe  The frame number within the packet
 *  \param[in]     config  The configuration parameters
 *  \param[in:out] pktCtx  The packet context
 *  \param[in:out] cmpCtx  The compression context, this maintained on a
 *                         per channel basis
 *
\* ----------------------------------------------------------------------- */
static void process_frame
            (ReadFrame        const                        &frame,
             ap_uint<PACKET_B_NSAMPLES>                    iframe,
             ModuleConfig     const                       &config,
             PacketContext                                &pktCtx,
             CompressionContext                           &cmpCtx)
{
   #pragma HLS INLINE off /// 2018-08-10 jjr Removing off did not work

   static AdcIn_t Prv[MODULE_K_NCHANNELS];
   #pragma HLS RESET           variable=Prv off
   #pragma HLS ARRAY_PARTITION variable=Prv cyclic factor= 8 ///complete ////factor=16 dim=1

   static Histogram HistsLcl[MODULE_K_NCHANNELS];
   #pragma HLS RESOURCE variable=HistsLcl  core=RAM_2P_LUTRAM
    PRAGMA_HLS(HLS ARRAY_PARTITION  variable=HistsLcl cyclic factor=NHISTPORTS dim=1)
   ///PRAGMA_HLS(HLS ARRAY_PARTITION  variable=HistsLcl cyclic factor=8 dim=1)

   static HeaderContextLcl HdrLcl;
   #pragma HLS RESET variable=HdrLcl off


   ReadStatus_t      status = frame.m_status;
   ReadFrame::Data const *d = frame.m_dat.d;

   if (iframe == 0)
   {
      HdrLcl.reset ();
      HdrLcl.m_status  = status = ReadStatus::errsOnFirst (status);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[0]);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[1]);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[1]);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[2]);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[3]);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[16]);
      HdrLcl.addHeader (pktCtx.m_hdrsBuf, d[17]);


      ///pktCtx.m_excsBuf[0] = 0;
      ///pktCtx.m_excsBuf[1] = 0;
   }
   else
   {
      // -----------------------------------------------------------
      // Add the headers that do not agree with the expected values
      // -------------------------------------------------------
      ///HdrLcl.evaluate (pktCtx.m_hdrsBuf, pktCtx.m_excsBuf,
      ///                 status, iframe, d[0], d[1], d[2], d[3], d[16], d[17]);
      HdrLcl.m_status |= status;
   }

   // ---------------------------------------------------------------
   // This will be the timestamp of the last packet.
   // Together with the first timestamp captured in the m_hdrsBuf[1],
   // it is used to define the time range of the packet.
   // ---------------------------------------------------------------
   if (iframe > 1020)
   {
        std::cout << "Timestamp[" << std::hex << std::setw (3) << iframe << "] = " << d[1] << std::endl;
   }
   pktCtx.m_hdrsBuf[2] = d[1];
   pktCtx.m_chdx       = HdrLcl.m_hdx;
   pktCtx.m_cedx       = HdrLcl.m_edx;
   pktCtx.m_status     = HdrLcl.m_status;


   // ---------------------------------------------------------------------
   // Pack the 128 ADCs into a vector 16 x 96 bits (8 x 12 ADCS per element
   // ---------------------------------------------------------------------
   int             idx = 4;
   ap_uint<8*12> adcs8[16];
   #pragma HLS ARRAY_PARTITION variable=adcs8 factor=2 cyclic
   PROCESS_COLDDATA_EXTRACT_LOOP:
   for (int odx = 0; odx < 16;)
   {
      #pragma HLS PIPELINE
      ap_uint<64> a0 = d[idx++];
      ap_uint<64> a1 = d[idx++];
      ap_uint<64> a2 = d[idx++];

      adcs8[odx++] = (a1(31,0), a0);
      adcs8[odx++] = (a2, a1(63,32));

      // --------------------------------------------------------
      // If have reached the start of the second colddata stream
      // Then skip the 2 header words
      // --------------------------------------------------------

      if (odx == 8)
      {
         idx += 2;
      }
   }


   // -----------------------------------------------------------
   // Access the 12-bit ADCs, form the symbols and histogram them
   // -----------------------------------------------------------
   PROCESS_COLDDATA_PROCESS_LOOP:
   int ichan = 0;
   for (int iserial  = 0; iserial < MODULE_K_NSERIAL;)
   {
      #pragma HLS PIPELINE II=2

      ap_uint<8*12> adc8 = adcs8[ichan >> 3];

      ap_uint<4*12> adc4 = adc8;
      process4 (cmpCtx. adcs.sg0[iserial],
                cmpCtx. adcs.sg1[iserial],
                cmpCtx. adcs.sg2[iserial],
                cmpCtx. adcs.sg3[iserial],

                cmpCtx.hists.sg0[iserial],
                cmpCtx.hists.sg1[iserial],
                cmpCtx.hists.sg2[iserial],
                cmpCtx.hists.sg3[iserial],

                &HistsLcl[ichan],
                &Prv[ichan],
                adc4,
                iframe,
                ichan);
      iserial += 1;
      ichan   += 4;

      adc4 = adc8 >> 48;
      process4 (cmpCtx. adcs.sg0[iserial],
                cmpCtx. adcs.sg1[iserial],
                cmpCtx. adcs.sg2[iserial],
                cmpCtx. adcs.sg3[iserial],

                cmpCtx.hists.sg0[iserial],
                cmpCtx.hists.sg1[iserial],
                cmpCtx.hists.sg2[iserial],
                cmpCtx.hists.sg3[iserial],

                &HistsLcl[ichan],
                &Prv[ichan],
                adc4,
                iframe,
                ichan);
      iserial += 1;
      ichan   += 4;
   }


   print_adcs (cmpCtx.adcs.sg0,
               cmpCtx.adcs.sg1,
               cmpCtx.adcs.sg2,
               cmpCtx.adcs.sg3,
               iframe);

   return;
}
/* ----------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Extracts, saves and histograms 4 ADCs. The histogram is of
 *          the difference between the current and previous ADC values
 *
 *   \param[in:out]  hists The 4 histograms
 *   \param[out]      adcs  The storage for the ADCs
 *   \param[in:out]    prv  The previous values of the 4 ADCs. After
 *                          being used to form the differences they
 *                          are replaced by the current ADC values.
 *   \param[in]      adcs4  The 4 x 12 input ADCs
 *
 *  \param[in]      iframe  The frame number
 *  \param[in]       ichan  For diagnostic purposes only (printf's...)
 *
\* ---------------------------------------------------------------------- */
static void process4 (AdcIn_t adcs0[PACKET_K_NSAMPLES],
                      AdcIn_t adcs1[PACKET_K_NSAMPLES],
                      AdcIn_t adcs2[PACKET_K_NSAMPLES],
                      AdcIn_t adcs3[PACKET_K_NSAMPLES],
                      Histogram                  &hist0,
                      Histogram                  &hist1,
                      Histogram                  &hist2,
                      Histogram                  &hist3,
                      Histogram             histsLcl[4],
                      AdcIn_t                    prv[4],
                      Adc48_t                     adcs4,
                      int                        iframe,
                      int                         ichan)
{
   #pragma HLS   INLINE
   #pragma HLS PIPELINE

   // ---------------------------------------------
   // Extract the 4 adcs packet into the input word
   // --------------------------------------------------
   AdcIn_t adc0 = extract_adc0 (adcs4);
   AdcIn_t adc1 = extract_adc1 (adcs4);
   AdcIn_t adc2 = extract_adc2 (adcs4);
   AdcIn_t adc3 = extract_adc3 (adcs4);

   // ----------------------------------------
   // Check if the frist sample in this packet
   // ----------------------------------------
   if (iframe == 0)
   {
      // --------------------------------------------------
      // The first ADC is the seed value is not compressed
      // --------------------------------------------------
      histsLcl[0].clear ();
      histsLcl[1].clear ();
      histsLcl[2].clear ();
      histsLcl[3].clear ();

      // ------------------------------------------------
      // !!! TEST !!!
      // See if this removes messages about elements not
      // being initialized.
      // ------------------------------------------------
      hist0.clear ();
      hist1.clear ();
      hist2.clear ();
      hist3.clear ();

      // -----------------------------------------------------
      // Store the ADCs
      // ----------------------------------------------------
      adcs0[iframe] = adc0;
      adcs1[iframe] = adc1;
      adcs2[iframe] = adc2;
      adcs3[iframe] = adc3;

      // -------------------------
      // Diagnostic only print-out
      // -------------------------
      process4_adcs_print (iframe, ichan, adc0, adc1, adc2, adc3);
   }
   else
   {
      // -----------------------------------------
      // Retrieve the adc from the previous sample
      // -----------------------------------------
      AdcIn_t prv0 = prv[0];
      AdcIn_t prv1 = prv[1];
      AdcIn_t prv2 = prv[2];
      AdcIn_t prv3 = prv[3];

      // ------------------------------------------------------------
      // Use the difference of the current ADC and its previous value
      // as the symbol to be encoded/compressed.
      // --------------------------------------------------
      Histogram::Symbol_t sym0 = Histogram::symbol (adc0, prv0);
      Histogram::Symbol_t sym1 = Histogram::symbol (adc1, prv1);
      Histogram::Symbol_t sym2 = Histogram::symbol (adc2, prv2);
      Histogram::Symbol_t sym3 = Histogram::symbol (adc3, prv3);

      // -----------------------------
      // Update the encoding histogram
      // -----------------------------
      histsLcl[0].bump (hist0, sym0);
      histsLcl[1].bump (hist1, sym1);
      histsLcl[2].bump (hist2, sym2);
      histsLcl[3].bump (hist3, sym3);

      // ----------------------------------
      // Store the symbols to be compressed  -- Change to storing the ADCs  STRIP
      // ----------------------------------
      adcs0[iframe] = adc0; /// STRIP sym0;
      adcs1[iframe] = adc1; /// STRIP sym1;
      adcs2[iframe] = adc2; /// STRIP sym2;
      adcs3[iframe] = adc3; /// STRIP sym3;

      // ------------------------
      // Diagnostic only printout
      // ------------------------
      process4_adcs_prv_sym_print (iframe, ichan,
                                   adc0,   prv0, sym0,
                                   adc1,   prv1, sym1,
                                   adc2,   prv2, sym2,
                                   adc3,   prv3, sym3);
   }

   // -------------------------------------------------------
   // Replace the previous ADC value with its current value
   // Note that the values from the last sample in the packet
   // are never used, but doing the check costs extra logic
   // so the values are just stored even though they are used.
   // -------------------------------------------------------
   prv[0] = adc0;
   prv[1] = adc1;
   prv[2] = adc2;
   prv[3] = adc3;

   return;
}
/* ====================================================================== */




/* ====================================================================== *\
 | Extacting 4 12-bit ADCs from a 48-bit input                            |
 + ---------------------------------------------------------------------- +
 |                                                                        |
 |  The ADCs originate from 2 x 12-bit streams interleaved by bytes,      |
 |  That is 1 byte is taken from stream 0, then 1 byte from stream 1.     |
 |                                                                        |
 |  Given that the ADCs are 12 bits, this leads to the ADCs being broken  |
 |  at nibble boundaries and results in a pattern that repeats every 48   |
 |  bits and containing 4 ADCs.                                           |
 |                                                                        |
 |  The following gives the layout of each of the 3 nibbles in 4 ADCs     |
 |  as the occur in a 12 (48-bit) bit field. For clarity the 3 nibbles    |
 |  of the ADC are labelled as a,b,c (from low to high)                   |
 |                                                                        |
 |  Nibble #   ba 98 76 54 32 10                                          |
 |  Bit        22 22 11 11 00 00                                          |
 |  Number     c8 40 c8 40 c8 40                                          |
 |             -- -- -- -- -- --                                          |
 |  ADC #      33 22 31 20 11 00                                          |
 |  ADC Nibble cb cb ac ac ba ba                                          |
 |                                                                        |
 |  For example ADC 2 has it's 3 nibbles in nibbles 9,8,5                 |
 |                                                                        |
\* ====================================================================== */





/* ====================================================================== */
/* ADC Extraction Methods                                                 */
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Extracts the ADC0 from the 48 bits
 *  \return ADC0
 *
 *  \param[in]  adcs4  The jumbled nibbles containing 4 ADCs
 *
 *  \par
 *   Here are the relevant nibbles for ADC0
 *
 *          22 22 11 11 00 00
 *          c8 40 c8 40 c8 40
 *          -- -- -- -- -- --
 *                    0    00
 *                    2    10
 *
\* ---------------------------------------------------------------------- */
static inline AdcIn_t extract_adc0 (Adc48_t adcs4)
{
   #pragma HLS INLINE

   AdcIn_t adc0 = (adcs4 (0x13,0x10), adcs4 (0x07, 0x00));
   return  adc0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Extracts the ADC1 from the 48 bits
 *  \return ADC1
 *
 *  \param[in]  adcs4  The jumbled nibbles containing 4 ADCs
 *
 *  \par
 *   Here are the relevant nibbles for ADC1
 *
 *          22 22 11 11 00 00
 *          c8 40 c8 40 c8 40
 *          -- -- -- -- -- --
 *                 0    11
 *                 2    11
 *
\* ---------------------------------------------------------------------- */
static inline AdcIn_t extract_adc1 (Adc48_t adcs4)
{
   #pragma HLS INLINE

   AdcIn_t adc1 = (adcs4 (0x1b,0x18), adcs4 (0x0f, 0x08));
   return  adc1;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Extracts ADC2 from the 48 bits
 *  \return ADC2
 *
 *  \param[in]  adcs4  The jumbled nibbles containing 4 ADCs
 *
 *  \par
 *   Here are the relevant nibbles for ADC2
 *
 *          22 22 11 11 00 00
 *          c8 40 c8 40 c8 40
 *          -- -- -- -- -- --
 *             22    2
 *             21    0
 *
\* ---------------------------------------------------------------------- */
static inline AdcIn_t extract_adc2 (Adc48_t adcs4)
{
   #pragma HLS INLINE

   AdcIn_t adc2 = (adcs4 (0x27,0x20), adcs4 (0x17, 0x14));
   return  adc2;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Extracts ADC3 from the 48 bits
 *  \return ADC3
 *
 *  \param[in]  adcs4  The jumbled nibbles containing 4 ADCs
 *
 *  \par
 *   Here are the relevant nibbles for ADC2
 *
 *          22 22 11 11 00 00
 *          c8 40 c8 40 c8 40
 *          -- -- -- -- -- --
 *          33    3
 *          21    0
 *
\* ---------------------------------------------------------------------- */
static inline AdcIn_t extract_adc3 (Adc48_t adcs4)
{
   #pragma HLS INLINE

   AdcIn_t adc3 = (adcs4 (0x2f,0x28), adcs4 (0x1f, 0x1c));
   return  adc3;
}
/* ====================================================================== */



/* ====================================================================== */
#if PROCESS_PRINT_ADCS
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Reconstitutes the ADCs from the symbols
 *
 *  \param[in]  adcs0  The adcs for channels 0, 4,  8 ..... 124
 *  \param[in]  adcs1  The adcs for channels 1, 5,  9 ..... 125
 *  \param]in]  adcs2  The adcs for channels 2, 6, 10 ..... 126
 *  \param[in]  adcs3  The adcs for channels 3, 7, 11 ..... 127
 *
\* ---------------------------------------------------------------------- */
static void print_adcs (AdcIn_t const adcs0[PACKET_K_NSAMPLES],
                        AdcIn_t const adcs1[PACKET_K_NSAMPLES],
                        AdcIn_t const adcs2[PACKET_K_NSAMPLES],
                        AdcIn_t const adcs3[PACKET_K_NSAMPLES],
                        int                              iframe)
{
   // ----------
   // Title line
   // ----------
   std::cout << "Output Adcs[" << std::setw (3) << std::hex << iframe << ']' << std::endl;


   // ----------------------
   // Print ADCs 16 per line
   // ----------------------
   for (int idx = 0; idx < 32; idx++)
   {
      // -------------------------------
      // Put title out on the first line
      // -------------------------------
      if ((idx & 0xf) == 0)
      {
         std::cout << "Adcs[" << std::setw(2) << idx << "] = ";
      }

      // ----------------------------------------------------------
      // If not the first, must add the previous value for this ADC
      // ----------------------------------------------------------
      int adc;
      int nserial   = idx / MODULE_K_NPARALLEL;
      int nparallel = idx % MODULE_K_NPARALLEL;

      if      ( (nparallel == 0) adc = adcs0[nserial][iframe];
      else if ( (nparallel == 1) adc = adcs1[nserial][iframe];
      else if ( (nparallel == 2) adc = adcs2[nserial][iframe];
      else if ( (nparallel == 3) adc = adcs3[nserial[iframe];

      // --------------------------------------------------------
      // Print ADC and a new line if one the last ADC in this row
      // --------------------------------------------------------
      std::cout << ' ' << std::setw (3) << std::hex << adc;
      if ((idx & 0xf) == 0xf) std::cout << std::endl;
   }
}
/* ---------------------------------------------------------------------- */
#endif  /* PROCESSS_PRINT_ADCS                                            */
/* ====================================================================== */


/* ====================================================================== */
#if PROCESS_PRINT
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Diagnostic print out of the 4 ADCs being processed
 *
 *  \param[in]  oidx  Effectively the time sample index, although
 *                    technically the index into the output packet.
 *  \param[in] ichan  The identifying channel number of the first sample
 *  \param[in]  adc0  The ADC of \a ichan + 0
 *  \param[in]  adc1  The ADC of \a ichan + 1
 *  \param[in]  adc2  The ADC of \a ichan + 2
 *  \param[in]  adc3  The ADC of \a ichan + 3
 *
\* ---------------------------------------------------------------------- */
static void process4_adcs_print (int     oidx,
                                 int    ichan,
                                 AdcIn_t adc0,
                                 AdcIn_t adc1,
                                 AdcIn_t adc2,
                                 AdcIn_t adc3)
{
   std::cout << "Process4[" << std::setw(5) << std::hex
            <<  oidx << ':' << ichan << ']'
            << ' ' << adc0
            << ' ' << adc1
            << ' ' << adc2
            << ' ' << adc3 << std::endl;
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Diagnostic print-out of the ADCs after processing
 *
 *  \param[in]  oidx  Effectively the time sample index, although
 *                    technically the index into the output packet.
 *  \param[in] ichan  The identifying channel number of the first sample
 *
 *  \param[in]  adc0  The current  ADC of \a ichan + 0
 *  \param[in]  prv0  The previous ADC of \a ichan + 0
 *  \param[in]  sym0  The symbol to be encoded for \a ichan + 0
 *
 *  \param[in]  adc1  The current  ADC of \a ichan + 1
 *  \param[in]  prv1  The previous ADC of \a ichan + 1
 *  \param[in]  sym1  The symbol to be encoded for \a ichan + 1
 *
 *  \param[in]  adc2  The current  ADC of \a ichan + 2
 *  \param[in]  prv2  The previous ADC of \a ichan + 2
 *  \param[in]  sym2  The symbol to be encoded for \a ichan + 2
 *
 *  \param[in]  adc3  The current  ADC of \a ichan + 3
 *  \param[in]  prv3  The previous ADC of \a ichan + 3
 *  \param[in]  sym3  The symbol to be encoded for \a ichan + 3
 *
\* ---------------------------------------------------------------------- */
static void process4_adcs_prv_sym_print
           (int     oidx, int    ichan,
            AdcIn_t adc0, AdcIn_t prv0, Histogram::Symbol_t sym0,
            AdcIn_t adc1, AdcIn_t prv1, Histogram::Symbol_t sym1,
            AdcIn_t adc2, AdcIn_t prv2, Histogram::Symbol_t sym2,
            AdcIn_t adc3, AdcIn_t prv3, Histogram::Symbol_t sym3)
{
   std::cout << "cur:prv->sym["
   << std::setw(3) << std::noshowbase << std::hex << ichan << ']'
   << "  " << std::setw(5) << adc0 << ':' << std::setw(5) << prv0 << "->" << std::setw(5) << sym0
   << "  " << std::setw(5) << adc1 << ':' << std::setw(5) << prv1 << "->" << std::setw(5) << sym1
   << "  " << std::setw(5) << adc2 << ':' << std::setw(5) << prv2 << "->" << std::setw(5) << sym2
   << "  " << std::setw(5) << adc3 << ':' << std::setw(5) << prv3 << "->" << std::setw(5) << sym3
   << std::endl;

   return;
}
/* ---------------------------------------------------------------------- */
#endif  /* PROCESS_PRINT                                                  */
/* ====================================================================== */

/* ---------------------------------------------------------------------- */
#endif  /* _WIBFRAME_PROCESS_H_                                           */
/* ====================================================================== */
