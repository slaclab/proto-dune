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


/* ======================================================================== */
/* OPTIONAL PRINT METHODS                                                   */
/* These are neutered if doing SYNTHESIS                                    */
/* ------------------------------------------------------------------------ */
#if     !defined(__SYNTHESIS__)
#define PROCESS_PRINT      0
#define PROCESS_PRINT_ADCS 1
#else
#define PROCESS_PRINT_ADCS 0
#define PROCESS_PRINT      0
#endif


#if    PROCESS_PRINT_ADCS
static void print_adcs (Symbol_t const syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                        int                                                iframe);
#else
#define print_adcs(_syms, _iframe)
#endif
/* ======================================================================== */




 /* ----------------------------------------------------------------------- *//*!
  *
  *  \class  ProcessFrame
  *  \brief  Class to hold the munged version of a WIB frame.
  *
 \* ----------------------------------------------------------------------- */
class ProcessFrame
 {

 public:
    typedef uint32_t Status;

    uint64_t                  hdrs[6]; /*!< The frame's 6 header words      */
    Status                     status; /*!< The frame's read status         */
 };
/* ------------------------------------------------------------------------ */


typedef ap_uint<48>           Adc48_t;




/* ======================================================================== */
/* Local Prototypes                                                         */
/* ------------------------------------------------------------------------ */

static void       process_colddata (Symbol_t syms[64][PACKET_K_NSAMPLES],
                                    Histogram                  hists[64],
                                    AdcIn_t                      prv[64],
                                    ReadFrame::Data const             *d,
                                    int                           iframe,
                                    int                         beg_chan);

static inline void        process4 (Symbol_t syms[ 4][PACKET_K_NSAMPLES],
                                    Histogram                   hists[4],
                                    AdcIn_t                       prv[4],
                                    Adc48_t                       adcs48,
                                    int                           iframe,
                                    int                            ichan);

static inline AdcIn_t extract_adc0 (Adc48_t                       adcs4);
static inline AdcIn_t extract_adc1 (Adc48_t                       adcs4);
static inline AdcIn_t extract_adc2 (Adc48_t                       adcs4);
static inline AdcIn_t extract_adc3 (Adc48_t                       adcs4);

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
 *  \param[out]     hists  The per channel histograms
 *  \param[out[      syms  The values  that will be compressed.  These
 *                         are result of a reversible function of an
 *                         ADC. (It must be reversible in order to
 *                         recover the original ADC.
 *
\* ----------------------------------------------------------------------- */
static void process_frame
            (ReadFrame        const                        &frame,
             ap_uint<PACKET_B_NSAMPLES>                    iframe,
             ModuleConfig     const                       &config,
             PacketContext                                &pktCtx,
             Histogram                  hists[MODULE_K_NCHANNELS],
             Symbol_t syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES])
{
   #pragma HLS INLINE off

   static AdcIn_t Prv[MODULE_K_NCHANNELS];
   #pragma HLS RESET           variable=Prv off
   #pragma HLS ARRAY_PARTITION variable=Prv cyclic factor=16 dim=1


   ReadStatus_t      status = frame.m_status;
   ReadFrame::Data const *d = frame.m_dat.d;
   ProcessFrame         out;


   static ap_int<PACKET_B_NSAMPLES> NFrames;

#if 0
   // --------------------------------------------------------------------
   // Check is have a valid frame
   //    1. It must have not protocol errors
   //    2.  The flush is only honored on the first frame of a new packet.
   // --------------------------------------------------------------------
   pktCtx.valid = ReadStatus::isGoodFrame (status)
            && ! ((iframe == 0) && status.test (static_cast<unsigned>(ReadStatus::Offset::Flush)));


   if (!pktCtx.valid)
   {
      pktCtx.nframes = iframe;
      return;
   }


   #pragma HLS RESOURCE        variable=out.syms core=RAM_2P_BRAM
   #pragma HLS ARRAY_PARTITION variable=out.syms cyclic factor=32


   if (iframe == 0)
   {
      pktCtx.wibId     = d[0] >> 8;
      pktCtx.timestamp = d[1];
   }
   pktCtx.nframes = iframe;

#endif

   uint64_t              w0 = d[0];
   std::cout << "W0[" << std::hex << NFrames << ']' << w0 << std::endl;

   int      idx = 2;

   uint64_t *hdrs = out.hdrs;

   // --------------------
   // The WIB header words
   // --------------------
   hdrs[0] = d[0];
   hdrs[1] = d[1];

   // ---------------------------------
   // Process the two cold data streams.
   // ---------------------------------
   hdrs[2] = d[2];
   hdrs[3] = d[3];
   process_colddata (&syms[ 0], &hists[ 0], &Prv[ 0], d+ 4, iframe,  0);

   hdrs[4] = d[14];
   hdrs[5] = d[15];
   process_colddata (&syms[64], &hists[64], &Prv[64], d+18, iframe, 64);

   print_adcs (syms, iframe);


   return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Process 1 colddata stream, extracting the cold data headers
 *           and ADCs.  The ADCs are processed into the symbols which will
 *           be compressed. Per channel histgrams are updated. These
 *           histograms are the basis of the compression tables.
 *
 *  \param[in:out]  hists  The per channel histograms
 *  \param[out]      syms  The values  that will be compressed.  These
 *                         are result of a reversible function of an
 *                         ADC. (It must be reversible in order to
 *                         recover the original ADC.
 *  \param[in:out]    prv  The ADC value of the previous sample. This
 *                         is used to form the symbol by subtracting
 *                         it from the ADC value, \i i.e. sym=cur-prv
 *  \param[in]          d  The cold data. This consists of 2 64-bit
 *                         header words followed by 12 64-bits containing
 *                         the ADCs.
 *  \param[in]     iframe  The frame number within the packet.  This
 *                         is used as the last index into \a syms.
 *  \param[in]    beg_chan The beginning channel number.  This is for
 *                         debugging purposes only and is either
 *                            -  0 for cold data stream 0
 *                            - 64 for cold data stream 1
 *
\* ----------------------------------------------------------------------- */
static void process_colddata (Symbol_t        syms[64][PACKET_K_NSAMPLES],
                              Histogram                         hists[64],
                              AdcIn_t                             prv[64],
                              ReadFrame::Data const                    *d,
                              int                                  iframe,
                              int                                beg_chan)
{
   #pragma HLS INLINE

   int idx = 0;
   PROCESS_COLDDATA_LOOP:
   for (int ichan = 0; ichan < MODULE_K_NCHANNELS/2;)
   {
      // ------------------------------------------------------------
      // The reading of the 5 axi stream limits the II to 5 and is
      // is the best one can do. This rids the output of carried
      // dependency warnings.
      // ------------------------------------------------------------
      #pragma HLS PIPELINE II=1

      HISTOGRAM_statement (hists[ichan].printit = ichan == 0 ? true : false;)


      // --------
      // ADCs 0-3
      // --------
      ap_uint<64>   a0 = d[idx++];
      Adc48_t    adcs4 = a0;
      process4 (&syms[ichan], &hists[ichan], &prv[ichan], adcs4, iframe, ichan);
      ichan += 4;

      // --------
      // ADCs 4-7
      // --------
      ap_uint<64> a1 = d[idx++];
      adcs4 = (a1(31,0), a0(63,48));
      process4 (&syms[ichan], &hists[ichan], &prv[ichan], adcs4,  iframe, ichan);
      ichan += 4;

      // ---------
      // ADCs 8-11
      // ---------
      ap_uint<64> a2 = d[idx++];
      adcs4 = (a2 (15,0), a1(63,32));
      process4 (&syms[ichan], &hists[ichan], &prv[ichan], adcs4, iframe, ichan);
      ichan += 4;

      // ----------
      // ADCs 12-15
      // ----------
      adcs4 = a2 (63,16);
      process4 (&syms[ichan], &hists[ichan], &prv[ichan], adcs4, iframe, ichan);
      ichan += 4;
   }

   return;

}
/* ---------------------------------------------------------------------- */



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
static void process4 (Symbol_t syms[4][PACKET_K_NSAMPLES],
                      Histogram                  hists[4],
                      AdcIn_t                      prv[4],
                      Adc48_t                       adcs4,
                      int                          iframe,
                      int                           ichan)
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
      hists[0].clear ();
      hists[1].clear ();
      hists[2].clear ();
      hists[3].clear ();

      // -----------------------------------------------------
      // Store the ADC as the first symbol
      // A symbol will always have enough bits to hold an ADC
      // ----------------------------------------------------
      syms[0][iframe] = adc0;
      syms[1][iframe] = adc1;
      syms[2][iframe] = adc2;
      syms[3][iframe] = adc3;

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
      hists[0].bump (sym0);
      hists[1].bump (sym1);
      hists[2].bump (sym2);
      hists[3].bump (sym3);

      // ----------------------------------
      // Store the symbols to be compressed
      // ----------------------------------
      syms[0][iframe] = sym0;
      syms[1][iframe] = sym1;
      syms[2][iframe] = sym2;
      syms[3][iframe] = sym3;

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
 *  \param[in]  syms  The symbols which are derived from the ADCs
 *
\* ---------------------------------------------------------------------- */
static void print_adcs (Symbol_t const syms[MODULE_K_NCHANNELS][PACKET_K_NSAMPLES],
                        int                                                 iframe)
{
   // -------------------------------
   // Holds onto the last set of ADCs
   // -------------------------------
   static uint16_t Last[MODULE_K_NCHANNELS];

   // ----------
   // Title line
   // ----------
   std::cout << "Output Adcs[" << std::setw (3) << std::hex << iframe << ']' << std::endl;


   // ----------------------
   // Print ADCs 16 per line
   // ----------------------
   for (int idx = 0; idx < 128; idx++)
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
      int adc  = syms[idx][iframe];
      if (iframe != 0)
      {
         int dif = adc;
         dif     = dif/2;
         if ((adc & 1) == 0) dif = -dif;
         adc = Last[idx] + dif;
      }
      Last[idx] = adc;

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
#else
/* ---------------------------------------------------------------------- */
static inline void process4_adcs_print(int     oidx,
                                       int    ichan,
                                       AdcIn_t adc0,
                                       AdcIn_t adc1,
                                       AdcIn_t adc2,
                                       AdcIn_t adc3)
{ 
  return;
}

static void process4_adcs_prv_sym_print
           (int     oidx, int    ichan,
            AdcIn_t adc0, AdcIn_t prv0, Histogram::Symbol_t sym0,
            AdcIn_t adc1, AdcIn_t prv1, Histogram::Symbol_t sym1,
            AdcIn_t adc2, AdcIn_t prv2, Histogram::Symbol_t sym2,
            AdcIn_t adc3, AdcIn_t prv3, Histogram::Symbol_t sym3)
{
   return;
}
/* ---------------------------------------------------------------------- */
#endif  /* PROCESS_PRINT                                                  */
/* ====================================================================== */


/* ---------------------------------------------------------------------- */
#endif  /* _WIBFRAME_PROCESS_H_                                           */
/* ====================================================================== */
