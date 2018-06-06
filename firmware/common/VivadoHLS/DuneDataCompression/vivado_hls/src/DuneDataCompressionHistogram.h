// -*-Mode: C++;-*-


#ifndef _DUNE_DATA_COMPRESSION_HISTOGRAM_H_
#define _DUNE_DATA_COMPRESSION_HISTOGRAM_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionHistogram.h
 *  @brief    Interface file for the Dune data compression to accumuluate
 *            and use the histograms necessary for statistical based 
 *            compressors
 *
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
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  2016.06.22
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
   2016.06.22 jjr Isolated from DuneDataCompresssion.cpp

\* ---------------------------------------------------------------------- */

#include "Parameters.h"

class BitStream64;

#ifndef __SYNTHESIS__
#define HISTOGRAM_DEBUG 1
#define HISTOGRAM_statement(_statement)  _statement
#else
#define HISTOGRAM_statement(_statement)
#endif


/* ====================================================================== */
/* DEFINITION: Histogram                                                  */
/* ---------------------------------------------------------------------- *//*!
 *
 *  \class Histogram
 *  \brief The histogram upon which the compression is based.
 *
 *   This only defines the number and width of the bins.  The meaning of
 *   each bin is defined by the application.  Of course there is some
 *   synergy between the number of bins and their usage.  The width of the
 *   bins is based on the maximum possible count that can be accumulated.
 *   This is determined by the number of samples that contribute to an
 *   output packet.  This is typically 1024 or 10 bits. Technically this
 *   means that the maximum entry is 1024 (0x400) or 11 bits.  However,
 *   to avoid adding an extra bit, which will likely only come up in a test
 *   situation (effectively 0 entropy. Since all bins will be 0, with the
 *   maximum bin rolling over to 0, this will be detected when the
 *   integrated histogram is formed.
\* ----------------------------------------------------------------------- */
#define HISTOGRAM_K_NBINS     32

#if     HISTOGRAM_K_NBINS == 128
#define HISTOGRAM_K_NBITS      7
#elif   HISTOGRAM_K_NBINS ==  64
#define HISTOGRAM_K_NBITS      6
#elif    HISTOGRAM_K_NBINS == 32
#define HISTOGRAM_K_NBITS      5
#else
#error HISTOGRAM_K_NBINS is not one of 32, 64 or 128
#endif



class Histogram
{
public:
   enum
   {
        NBins       = HISTOGRAM_K_NBINS,
        /*!< Number of bins to use                                         */

        NBits       = HISTOGRAM_K_NBITS,
        /*!< Number of bits needed to index a histogram bin                */

        MIdx       = (1 << HISTOGRAM_K_NBITS) - 1,
        /*!< Mask of bin index (\e e.g. 128 bins => 0x7f)                  */

        NGroups     = HISTOGRAM_K_NBINS/32,
        /*!< Number of 32-bit groups                                       */

        NETableBits = 20 - (10 - Histogram::NBits)
        /*!< Number of bits used in the entropy table entry                */
   };

public:
   typedef ap_uint<PACKET_B_NSAMPLES>                         Entry_t;
   typedef ap_uint<PACKET_B_NSAMPLES+1>                         Acc_t;
   typedef ap_uint<Histogram::NETableBits>                   ETable_t;
   typedef ap_uint<Histogram::NETableBits + Histogram::NBits> ESize_t;
   typedef ap_uint<Histogram::NBins>                        BitMask_t;
   typedef ap_uint<Histogram::NBits>                            Idx_t;
   typedef ap_uint<ADC_B_NBITS  + 1>                         Symbol_t;
   typedef Entry_t                                              Table;
public:
   Histogram ();

public:
   void         init               ();
   void         clear              ();
   static void  clear              (Histogram hists[], int nhists);
   void         bump               (Symbol_t sym);
   void         encode             (BitStream64 &bs64,
                                    Table table[NBins+1],
                                    AdcIn_t       first) const;

   uint32_t                   size () const;
   uint32_t     integrate_and_size ();

   static Symbol_t symbol          (AdcIn_t cur, AdcIn_t prv);
   static    Idx_t idx             (Symbol_t symbol, Symbol_t &ovr);

public:
   BitMask_t        m_omask; /*!< Bit mask of non-zero entries            */
   Entry_t         m_maxcnt; /*!< Maximum counts                          */
   Idx_t          m_lastgt0; /*!> Index of tje last entry with counts > 0 */
   Idx_t          m_lastgt1; /*!< Index of the last entry with counts > 1 */
   Idx_t          m_lastgt2; /*!< Index of the last entry with counts > 2 */
   Symbol_t           m_min; /*!< Minimum overflow difference             */
   Symbol_t           m_max; /*!< Maximum overflow difference             */
   Entry_t    m_bins[NBins]; /*!< The bins of the histogram.              */

                             /*!< Copy of the stuff                       */
   BitMask_t       m_comask; /*!< Bit mask of non-zero entries            */
   Entry_t        m_cmaxcnt; /*!< Maximum counts                          */
   Idx_t         m_clastgt0; /*!> Index of tje last entry with counts > 0 */
   Idx_t         m_clastgt1; /*!< Index of the last entry with counts > 1 */
   Idx_t         m_clastgt2; /*!< Index of the last entry with counts > 2 */
   Symbol_t          m_cmin; /*!< Minimum overflow difference             */
   ap_uint<4>     m_cnobits; /*!< Number of bits in m_cmax - m_cmin       */
   Entry_t   m_cbins[NBins]; /*!< Copy of m_bins                          */

   //----------------------------------------------
   // Diagnostic print routines.
   // These are neutered during the synthesymbosis stage
   //----------------------------------------------
   HISTOGRAM_statement
   (

    void         print                   ()          const;
    static void  print                   (Histogram  const  hists[],
                                          int                nhists);

    static void set_id                   (Histogram         hists[],
                                          int               nhists);
    static void  print_integration_title ();
    static void  print_integration_line  (int                   bin,
                                          Histogram::Entry_t    cnt,
                                          Histogram::Acc_t    total,
                                          Histogram::ETable_t     e,
                                          Histogram::ESize_t      b);

    public:
    int                 m_id; /*< Histogram identifier                    */
    bool             printit; /*< Flag whether to print or not            */

   )


};
/* ---------------------------------------------------------------------- */
/* END DEFINITION: Histogram                                              */
/* ====================================================================== */



/* ====================================================================== */
/* IMPLEMENTATION: Histogram                                              */
/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Dummy Histogram, needed to satisfy the array construction.
 *
\* ---------------------------------------------------------------------- */
inline Histogram::Histogram ()
{
   /*
   #pragma HLS RESET variable=m_bins    off
   #pragma HLS RESET variable=m_omask   off
   #pragma HLS RESET variable=m_lastgt0 off
   #pragma HLS RESET variable=m_lastgt1 off
   #pragma HLS RESET variable=m_lastgt2 off
   #pragma HLS RESET variable=m_min     off
   #pragma HLS RESET variable=m_max     off
   */

   /*
   #pragma HLS RESOURCE variable=m_omask   core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_lastgt0 core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_lastgt1 core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_lastgt2 core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_min     core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_max     core=RAM_2P_LUTRAM
   */

   /*
   #pragma HLS RESET variable=m_cbins    off
   #pragma HLS RESET variable=m_comask   off
   #pragma HLS RESET variable=m_clastgt0 off
   #pragma HLS RESET variable=m_clastgt1 off
   #pragma HLS RESET variable=m_clastgt2 off
   #pragma HLS RESET variable=m_cmin     off
   #pragma HLS RESET variable=m_cnobits off
   #pragma HLS RESET variable=m_cmax     off
   */

   /*
   #pragma HLS RESOURCE variable=m_comask   core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_clastgt0 core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_clastgt1 core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_clastgt2 core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_cmin     core=RAM_2P_LUTRAM
   #pragma HLS RESOURCE variable=m_cnobits  core=RAM_2P_LUTRAM
*/

   //#pragma HLS RESOURCE        variable=m_bins  core=RAM_2P_LUTRAM
   //#pragma HLS RESOURCE        variable=m_cbins core=RAM_2P_LUTRAM
   #pragma HLS ARRAY_PARTITION variable=m_bins cyclic  factor=2 dim=1
   #pragma HLS ARRAY_PARTITION variable=m_cbins cyclic factor=2 dim=1


   init ();
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Initialization of a histogram
 *
\* ---------------------------------------------------------------------- */
inline void Histogram::init ()
{
   ////clear ();
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Clear (zero) the contents of a histogram
 *
\* ---------------------------------------------------------------------- */
inline void Histogram::clear ()
{
   #pragma HLS INLINE
   #pragma HLS PIPELINE

   m_cmaxcnt  = m_cmaxcnt =  0;
   m_comask   = m_omask   =  0;
   m_clastgt0 = m_lastgt0 =  0;
   m_clastgt1 = m_lastgt1 =  0;
   m_clastgt2 = m_lastgt2 =  0;
   m_cmin     = m_min     = (1 << m_min.length () - 1);
                m_max     =  0;
   m_cnobits  = 0;

#if 0
   HISTOGRAM_CLEAR_LOOP:
   for (int idx = 0; idx < sizeof (m_bins) /sizeof (m_bins[0]); idx++)
   {
      #pragma HLS PIPELINE
      m_bins[idx] = 0;
   }
#endif

   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
HISTOGRAM_statement (
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Set the histogram identifier
 *
 *  \param[out]  hists  The  array of histograms to clear
 *  \param[ in[ nhists  The number of histograms to clear
 *
\* ---------------------------------------------------------------------- */
inline void  Histogram::set_id (Histogram hists[], int nhists)
{
   for (int idx = 0; idx < nhists; idx++)
   {
      hists[idx].m_id = idx;
   }

   return;
}
/* ---------------------------------------------------------------------- */
)
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Clear (zero) the contents of an array of histograms
 *
 *  \param[out]  hists  The  array of histograms to clear
 *  \param[ in[ nhists  The number of histograms to clear
 *
\* ---------------------------------------------------------------------- */
inline void  Histogram::clear (Histogram hists[], int nhists)
{
//#  pragma HLS INLINE

   HISTOGRAMS_CLEAR_LOOP:
   for (int idx = 0; idx < nhists; idx++)
   {
      #pragma HLS PIPELINE
      #pragma HLS UNROLL
      hists[idx].clear();
   }

   return;
}
/* ---------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  Calculates the symbol that will be used in the compression
 *  \return The symbol
 *
 *  \param[in]  cur  The current  ADC value
 *  \param[in]  prv  The previous ADC value
\* ---------------------------------------------------------------------- */
inline Histogram::Symbol_t Histogram::symbol (AdcIn_t cur, AdcIn_t prv)
{
#  pragma HLS inline

   Histogram::Symbol_t sym;

   int diff = ((cur - prv) << 1) | 1;

   // ----------------------------------------------
   // Massage the difference so that it is positive.
   // When the difference is 0, the bin will be 1
   // Bin = 0 is reserved for the overflow.
   // -------------------------------------------
   if (diff < 0) sym = -diff + 1;
   else          sym =  diff;

   return sym;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  Limits the symbol to be within the range of the histogram index
 *  \return The index of the histogram bin
 *
 *  \param[ in]  cur  The current  ADC value
 *  \param[ in]  prv  The previous ADC value
 *  \param[out]  ovr  If the resulting symbol cannot be contained in
 *                    the histogram, the count over the maximum bin
 *                    index. If it is, this value is undefined.
 *
\* ---------------------------------------------------------------------- */
inline Histogram::Idx_t Histogram::idx (Symbol_t symbol, Symbol_t &ovr)
{
#  pragma HLS inline

   Idx_t idx;

   // Confine to a legitimate histogram bin
   int diff = symbol - Histogram::NBins;
   if (diff >= 0)
   {
      idx =  static_cast<Histogram::Symbol_t>(0);
      ovr =  diff;
   }
   else
   {
      idx = symbol;
   }

   return idx;
}
/* ----------------------------------------------------------------------- */



#ifndef __SYNTHESIS__
static void bump_print (int                           bin,
                        Histogram::Entry_t const m_bins[],
                        int                         pbits,
                        int                         cbits,
                        int                         nbits)
{
   std::cout
      << std::hex << std::setw(5) << bin
      << "Bins " << m_bins[bin-1] << ':' << m_bins[bin] << ':' << m_bins[bin+1]
      << "Bits " << std::dec << pbits << ':' << cbits << ':' << nbits
      << std::endl;

   return;
}
#else
#define bump_print(_bin, _m_bins, _pbits, _cbits, _nbits)
#endif



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Increment the designated bin of the histogram
 *
 *  \param[in] sym The symbol to histogram.  This is not ore-checked for
 *                 histogram containment because, in addition to
 *                 histogramming, wish to keep track of the range of
 *                 the overflows
 *
\* ---------------------------------------------------------------------- */
inline void Histogram::bump (Symbol_t sym)
{
#  pragma HLS INLINE
#  pragma HLS PIPELINE II=2


   int bin;

   // ------------------------------------------------------
   // Check if this symbol is contained in the histogram
   // ------------------------------------------------------
   int ovr = sym - sizeof (m_bins) / sizeof (m_bins[0]);
   if (ovr >= 0)
   {
      // Keep track of the minimum and maximum overflow
      if (ovr > m_max)          m_max = ovr;
      if (ovr < m_min) m_cmin = m_min = ovr;

      m_cnobits = m_max.length () - m_max.countLeadingZeros ();
      bin       = 0;
   }
   else
   {
      bin = sym;
   }

   Entry_t cnts;

   // ---------------------------------------------------------
   // The following logic is structured to avoid a dependency
   // between the setting of 'cnts' and the bumping of it.
   // Timing is tight if the addition is done here, where it
   // most naturally would be. Deferring the addition until
   // after the checking of 'cnts' allows the timing to be met.
   // --------------------------------------------------------

   // ----------------------------------------------------------
   // Check if this is not the first entry for this bin.
   //
   // Doing it this way avoids an explicit clear of the
   // histogram which lumps the time and is quite costly in
   // logic. The clear is reduced to clearing the much smaller
   // bit mask (m_omask).  The price is that this mask must be
   // always checked for validity of the bin contents. However,
   // because this checking can almost always be absorbed or
   // hidden behind some other logic, the price is nearly 0.
   // ----------------------------------------------------------
   if (m_omask.test (bin))
   {
      cnts  = m_bins[bin];
   }
   else
   {
      // No, set cnts=0 and mark this bin as occupied
      cnts = 0;
      m_omask.set (bin);

   }

   m_comask = m_omask;


   // -------------------------------------------------------------------
   // Keep track of the last bins with counts > 1 and counts > 3 (2 bits)
   // the gtN refers to entries with greater than N bits.
   // -------------------------------------------------------------------
   if ( (cnts >= 0) && (bin > m_lastgt0)) m_clastgt0 = m_lastgt0 = bin;
   if ( (cnts >= 1) && (bin > m_lastgt1)) m_clastgt1 = m_lastgt1 = bin;
   if ( (cnts >= 3) && (bin > m_lastgt2)) m_clastgt2 = m_lastgt2 = bin;

    cnts        += 1;
    m_bins[bin]  = cnts;
    m_cbins[bin] = cnts;

    // Keep track of the maximum count
    if (cnts >= m_maxcnt) m_cmaxcnt = m_maxcnt = cnts;

   return;
}
/* ---------------------------------------------------------------------- */

/*
 *  The number below is  (log2(1024) - n log2 (n)) scaled by 1024.  The largest entry
 *  is 556519, just over 19 bits.so the table is made a width of 20 bits. With the
 *  1024 scaling this table should not introduce any rounding errors for summing up to
 *  1024 terms.  In fact, the maximum number of terms is fixed by the number of bins
 *  in the histogram, typically, 32-64 bins. This means that 4 or 5 bits are unnecessary.
 *
 *  There are two options.
 *    1) Create a multiple tables
 *    2) Create 1 table with its entries defined by a scaling macro determined by
 *       the number of bins.
 *
 *  I chose the later as being the more versatile.
 *    32 bins => width of 20 - (10 - 5) = 15
 *    64 bins => width of 20 - (10 - 6) = 16
 */
#define S(_x) (_x + (1024/Histogram::NBins)/2)/(1024/Histogram::NBins)
const Histogram::ETable_t ETable[1024] =
{
  S(     0),  S( 10240),  S( 18432),  S( 25851),//    0 -    3
  S( 32768),  S( 39312),  S( 45558),  S( 51557),//    4 -    7
  S( 57344),  S( 62946),  S( 68383),  S( 73673),//    8 -   11
  S( 78828),  S( 83860),  S( 88778),  S( 93590),//   12 -   15
  S( 98304),  S(102925),  S(107460),  S(111912),//   16 -   19
  S(116287),  S(120588),  S(124818),  S(128981),//   20 -   23
  S(133080),  S(137117),  S(141095),  S(145017),//   24 -   27
  S(148884),  S(152697),  S(156460),  S(160174),//   28 -   31
  S(163840),  S(167460),  S(171035),  S(174567),//   32 -   35
  S(178056),  S(181504),  S(184913),  S(188282),//   36 -   39
  S(191614),  S(194909),  S(198167),  S(201391),//   40 -   43
  S(204580),  S(207735),  S(210858),  S(213949),//   44 -   47
  S(217008),  S(220036),  S(223035),  S(226003),//   48 -   51
  S(228943),  S(231854),  S(234738),  S(237594),//   52 -   55
  S(240423),  S(243226),  S(246003),  S(248754),//   56 -   59
  S(251481),  S(254182),  S(256860),  S(259514),//   60 -   63
  S(262144),  S(264751),  S(267336),  S(269898),//   64 -   67
  S(272438),  S(274956),  S(277453),  S(279929),//   68 -   71
  S(282384),  S(284818),  S(287232),  S(289627),//   72 -   75
  S(292001),  S(294356),  S(296692),  S(299009),//   76 -   79
  S(301308),  S(303588),  S(305849),  S(308093),//   80 -   83
  S(310318),  S(312527),  S(314717),  S(316891),//   84 -   87
  S(319048),  S(321188),  S(323311),  S(325418),//   88 -   91
  S(327508),  S(329583),  S(331642),  S(333684),//   92 -   95
  S(335712),  S(337724),  S(339721),  S(341702),//   96 -   99
  S(343669),  S(345621),  S(347559),  S(349481),//  100 -  103
  S(351390),  S(353284),  S(355165),  S(357031),//  104 -  107
  S(358883),  S(360722),  S(362548),  S(364360),//  108 -  111
  S(366158),  S(367943),  S(369716),  S(371475),//  112 -  115
  S(373222),  S(374955),  S(376676),  S(378385),//  116 -  119
  S(380081),  S(381765),  S(383437),  S(385096),//  120 -  123
  S(386744),  S(388380),  S(390003),  S(391616),//  124 -  127
  S(393216),  S(394805),  S(396382),  S(397949),//  128 -  131
  S(399503),  S(401047),  S(402580),  S(404101),//  132 -  135
  S(405612),  S(407111),  S(408600),  S(410078),//  136 -  139
  S(411546),  S(413003),  S(414450),  S(415886),//  140 -  143
  S(417312),  S(418727),  S(420132),  S(421528),//  144 -  147
  S(422913),  S(424288),  S(425653),  S(427009),//  148 -  151
  S(428355),  S(429691),  S(431017),  S(432334),//  152 -  155
  S(433641),  S(434938),  S(436227),  S(437506),//  156 -  159
  S(438775),  S(440036),  S(441287),  S(442529),//  160 -  163
  S(443762),  S(444986),  S(446201),  S(447408),//  164 -  167
  S(448605),  S(449793),  S(450973),  S(452144),//  168 -  171
  S(453307),  S(454461),  S(455606),  S(456743),//  172 -  175
  S(457871),  S(458991),  S(460103),  S(461207),//  176 -  179
  S(462302),  S(463389),  S(464467),  S(465538),//  180 -  183
  S(466601),  S(467655),  S(468702),  S(469740),//  184 -  187
  S(470771),  S(471794),  S(472809),  S(473816),//  188 -  191
  S(474816),  S(475808),  S(476792),  S(477768),//  192 -  195
  S(478737),  S(479699),  S(480653),  S(481599),//  196 -  199
  S(482538),  S(483470),  S(484394),  S(485311),//  200 -  203
  S(486221),  S(487124),  S(488019),  S(488907),//  204 -  207
  S(489788),  S(490662),  S(491529),  S(492388),//  208 -  211
  S(493241),  S(494087),  S(494926),  S(495758),//  212 -  215
  S(496583),  S(497401),  S(498213),  S(499017),//  216 -  219
  S(499815),  S(500607),  S(501391),  S(502169),//  220 -  223
  S(502940),  S(503705),  S(504463),  S(505214),//  224 -  227
  S(505960),  S(506698),  S(507430),  S(508156),//  228 -  231
  S(508875),  S(509588),  S(510295),  S(510995),//  232 -  235
  S(511689),  S(512377),  S(513058),  S(513733),//  236 -  239
  S(514403),  S(515065),  S(515722),  S(516373),//  240 -  243
  S(517018),  S(517656),  S(518289),  S(518915),//  244 -  247
  S(519536),  S(520151),  S(520759),  S(521362),//  248 -  251
  S(521959),  S(522550),  S(523135),  S(523714),//  252 -  255
  S(524288),  S(524856),  S(525418),  S(525974),//  256 -  259
  S(526525),  S(527070),  S(527609),  S(528143),//  260 -  263
  S(528671),  S(529193),  S(529710),  S(530221),//  264 -  267
  S(530727),  S(531227),  S(531722),  S(532211),//  268 -  271
  S(532695),  S(533174),  S(533647),  S(534114),//  272 -  275
  S(534576),  S(535033),  S(535485),  S(535931),//  276 -  279
  S(536372),  S(536808),  S(537238),  S(537663),//  280 -  283
  S(538083),  S(538498),  S(538908),  S(539312),//  284 -  287
  S(539711),  S(540105),  S(540494),  S(540878),//  288 -  291
  S(541257),  S(541631),  S(541999),  S(542363),//  292 -  295
  S(542722),  S(543076),  S(543424),  S(543768),//  296 -  299
  S(544107),  S(544441),  S(544770),  S(545094),//  300 -  303
  S(545413),  S(545728),  S(546037),  S(546342),//  304 -  307
  S(546642),  S(546937),  S(547227),  S(547512),//  308 -  311
  S(547793),  S(548069),  S(548341),  S(548608),//  312 -  315
  S(548870),  S(549127),  S(549379),  S(549627),//  316 -  319
  S(549871),  S(550109),  S(550343),  S(550573),//  320 -  323
  S(550798),  S(551018),  S(551234),  S(551446),//  324 -  327
  S(551652),  S(551854),  S(552052),  S(552246),//  328 -  331
  S(552435),  S(552619),  S(552799),  S(552974),//  332 -  335
  S(553146),  S(553313),  S(553475),  S(553633),//  336 -  339
  S(553786),  S(553936),  S(554081),  S(554221),//  340 -  343
  S(554358),  S(554490),  S(554617),  S(554741),//  344 -  347
  S(554860),  S(554975),  S(555086),  S(555193),//  348 -  351
  S(555295),  S(555393),  S(555487),  S(555577),//  352 -  355
  S(555662),  S(555744),  S(555821),  S(555894),//  356 -  359
  S(555963),  S(556028),  S(556089),  S(556146),//  360 -  363
  S(556199),  S(556247),  S(556292),  S(556333),//  364 -  367
  S(556369),  S(556402),  S(556430),  S(556455),//  368 -  371
  S(556475),  S(556492),  S(556505),  S(556513),//  372 -  375
  S(556518),  S(556519),  S(556516),  S(556509),//  376 -  379
  S(556498),  S(556483),  S(556464),  S(556442),//  380 -  383
  S(556415),  S(556385),  S(556351),  S(556313),//  384 -  387
  S(556272),  S(556226),  S(556177),  S(556123),//  388 -  391
  S(556067),  S(556006),  S(555941),  S(555873),//  392 -  395
  S(555802),  S(555726),  S(555647),  S(555564),//  396 -  399
  S(555477),  S(555386),  S(555292),  S(555194),//  400 -  403
  S(555092),  S(554987),  S(554879),  S(554766),//  404 -  407
  S(554650),  S(554531),  S(554407),  S(554280),//  408 -  411
  S(554150),  S(554016),  S(553878),  S(553737),//  412 -  415
  S(553592),  S(553444),  S(553292),  S(553136),//  416 -  419
  S(552977),  S(552815),  S(552649),  S(552479),//  420 -  423
  S(552306),  S(552130),  S(551950),  S(551767),//  424 -  427
  S(551580),  S(551389),  S(551196),  S(550999),//  428 -  431
  S(550798),  S(550594),  S(550387),  S(550176),//  432 -  435
  S(549961),  S(549744),  S(549523),  S(549298),//  436 -  439
  S(549071),  S(548840),  S(548605),  S(548367),//  440 -  443
  S(548126),  S(547882),  S(547634),  S(547383),//  444 -  447
  S(547128),  S(546871),  S(546610),  S(546346),//  448 -  451
  S(546078),  S(545807),  S(545533),  S(545256),//  452 -  455
  S(544975),  S(544691),  S(544404),  S(544114),//  456 -  459
  S(543820),  S(543524),  S(543224),  S(542921),//  460 -  463
  S(542615),  S(542305),  S(541992),  S(541676),//  464 -  467
  S(541357),  S(541035),  S(540710),  S(540381),//  468 -  471
  S(540050),  S(539715),  S(539377),  S(539036),//  472 -  475
  S(538693),  S(538345),  S(537995),  S(537642),//  476 -  479
  S(537285),  S(536926),  S(536563),  S(536197),//  480 -  483
  S(535829),  S(535457),  S(535082),  S(534704),//  484 -  487
  S(534323),  S(533939),  S(533553),  S(533163),//  488 -  491
  S(532770),  S(532374),  S(531975),  S(531573),//  492 -  495
  S(531168),  S(530760),  S(530349),  S(529935),//  496 -  499
  S(529519),  S(529099),  S(528676),  S(528250),//  500 -  503
  S(527822),  S(527390),  S(526956),  S(526519),//  504 -  507
  S(526078),  S(525635),  S(525189),  S(524740),//  508 -  511
  S(524288),  S(523833),  S(523375),  S(522915),//  512 -  515
  S(522452),  S(521985),  S(521516),  S(521045),//  516 -  519
  S(520570),  S(520092),  S(519611),  S(519128),//  520 -  523
  S(518642),  S(518153),  S(517661),  S(517167),//  524 -  527
  S(516669),  S(516169),  S(515666),  S(515160),//  528 -  531
  S(514652),  S(514140),  S(513627),  S(513109),//  532 -  535
  S(512590),  S(512068),  S(511542),  S(511015),//  536 -  539
  S(510484),  S(509951),  S(509414),  S(508876),//  540 -  543
  S(508335),  S(507790),  S(507243),  S(506693),//  544 -  547
  S(506141),  S(505586),  S(505028),  S(504468),//  548 -  551
  S(503905),  S(503339),  S(502771),  S(502200),//  552 -  555
  S(501625),  S(501049),  S(500470),  S(499888),//  556 -  559
  S(499304),  S(498717),  S(498127),  S(497535),//  560 -  563
  S(496940),  S(496343),  S(495743),  S(495139),//  564 -  567
  S(494534),  S(493926),  S(493316),  S(492703),//  568 -  571
  S(492087),  S(491468),  S(490848),  S(490224),//  572 -  575
  S(489598),  S(488970),  S(488338),  S(487705),//  576 -  579
  S(487069),  S(486430),  S(485788),  S(485144),//  580 -  583
  S(484498),  S(483849),  S(483198),  S(482543),//  584 -  587
  S(481887),  S(481228),  S(480567),  S(479902),//  588 -  591
  S(479236),  S(478567),  S(477895),  S(477221),//  592 -  595
  S(476545),  S(475866),  S(475184),  S(474500),//  596 -  599
  S(473814),  S(473125),  S(472434),  S(471740),//  600 -  603
  S(471044),  S(470345),  S(469644),  S(468940),//  604 -  607
  S(468234),  S(467526),  S(466815),  S(466102),//  608 -  611
  S(465386),  S(464668),  S(463948),  S(463225),//  612 -  615
  S(462499),  S(461771),  S(461041),  S(460309),//  616 -  619
  S(459574),  S(458837),  S(458097),  S(457355),//  620 -  623
  S(456611),  S(455864),  S(455115),  S(454364),//  624 -  627
  S(453610),  S(452854),  S(452095),  S(451334),//  628 -  631
  S(450571),  S(449805),  S(449037),  S(448267),//  632 -  635
  S(447494),  S(446720),  S(445943),  S(445163),//  636 -  639
  S(444381),  S(443597),  S(442811),  S(442022),//  640 -  643
  S(441231),  S(440437),  S(439642),  S(438844),//  644 -  647
  S(438044),  S(437242),  S(436437),  S(435630),//  648 -  651
  S(434820),  S(434009),  S(433195),  S(432379),//  652 -  655
  S(431561),  S(430740),  S(429917),  S(429092),//  656 -  659
  S(428265),  S(427435),  S(426603),  S(425770),//  660 -  663
  S(424933),  S(424095),  S(423254),  S(422411),//  664 -  667
  S(421566),  S(420719),  S(419869),  S(419017),//  668 -  671
  S(418163),  S(417307),  S(416449),  S(415588),//  672 -  675
  S(414725),  S(413861),  S(412993),  S(412125),//  676 -  679
  S(411253),  S(410379),  S(409504),  S(408625),//  680 -  683
  S(407746),  S(406863),  S(405979),  S(405092),//  684 -  687
  S(404203),  S(403313),  S(402420),  S(401524),//  688 -  691
  S(400627),  S(399728),  S(398826),  S(397922),//  692 -  695
  S(397016),  S(396108),  S(395198),  S(394287),//  696 -  699
  S(393372),  S(392455),  S(391537),  S(390616),//  700 -  703
  S(389694),  S(388769),  S(387842),  S(386913),//  704 -  707
  S(385982),  S(385049),  S(384113),  S(383176),//  708 -  711
  S(382236),  S(381295),  S(380351),  S(379406),//  712 -  715
  S(378458),  S(377508),  S(376557),  S(375602),//  716 -  719
  S(374647),  S(373689),  S(372728),  S(371766),//  720 -  723
  S(370803),  S(369836),  S(368868),  S(367898),//  724 -  727
  S(366926),  S(365951),  S(364974),  S(363997),//  728 -  731
  S(363016),  S(362033),  S(361049),  S(360062),//  732 -  735
  S(359075),  S(358084),  S(357091),  S(356097),//  736 -  739
  S(355100),  S(354102),  S(353101),  S(352099),//  740 -  743
  S(351095),  S(350088),  S(349080),  S(348069),//  744 -  747
  S(347057),  S(346042),  S(345026),  S(344008),//  748 -  751
  S(342988),  S(341966),  S(340941),  S(339915),//  752 -  755
  S(338887),  S(337857),  S(336825),  S(335792),//  756 -  759
  S(334756),  S(333718),  S(332678),  S(331636),//  760 -  763
  S(330593),  S(329547),  S(328500),  S(327450),//  764 -  767
  S(326399),  S(325346),  S(324290),  S(323233),//  768 -  771
  S(322174),  S(321113),  S(320051),  S(318985),//  772 -  775
  S(317919),  S(316851),  S(315780),  S(314708),//  776 -  779
  S(313633),  S(312557),  S(311479),  S(310399),//  780 -  783
  S(309317),  S(308234),  S(307148),  S(306060),//  784 -  787
  S(304971),  S(303880),  S(302787),  S(301692),//  788 -  791
  S(300595),  S(299496),  S(298395),  S(297293),//  792 -  795
  S(296189),  S(295082),  S(293975),  S(292865),//  796 -  799
  S(291753),  S(290639),  S(289524),  S(288407),//  800 -  803
  S(287288),  S(286167),  S(285044),  S(283920),//  804 -  807
  S(282793),  S(281665),  S(280534),  S(279403),//  808 -  811
  S(278269),  S(277134),  S(275996),  S(274857),//  812 -  815
  S(273716),  S(272573),  S(271429),  S(270282),//  816 -  819
  S(269134),  S(267984),  S(266832),  S(265679),//  820 -  823
  S(264523),  S(263366),  S(262207),  S(261046),//  824 -  827
  S(259884),  S(258719),  S(257554),  S(256385),//  828 -  831
  S(255216),  S(254045),  S(252871),  S(251696),//  832 -  835
  S(250519),  S(249341),  S(248160),  S(246978),//  836 -  839
  S(245794),  S(244609),  S(243422),  S(242232),//  840 -  843
  S(241042),  S(239849),  S(238655),  S(237459),//  844 -  847
  S(236261),  S(235061),  S(233860),  S(232657),//  848 -  851
  S(231452),  S(230246),  S(229037),  S(227827),//  852 -  855
  S(226616),  S(225402),  S(224187),  S(222970),//  856 -  859
  S(221752),  S(220531),  S(219309),  S(218085),//  860 -  863
  S(216860),  S(215633),  S(214404),  S(213173),//  864 -  867
  S(211941),  S(210707),  S(209471),  S(208234),//  868 -  871
  S(206994),  S(205753),  S(204512),  S(203267),//  872 -  875
  S(202022),  S(200774),  S(199525),  S(198274),//  876 -  879
  S(197021),  S(195767),  S(194511),  S(193254),//  880 -  883
  S(191994),  S(190733),  S(189471),  S(188206),//  884 -  887
  S(186940),  S(185673),  S(184403),  S(183132),//  888 -  891
  S(181860),  S(180586),  S(179310),  S(178032),//  892 -  895
  S(176753),  S(175472),  S(174189),  S(172905),//  896 -  899
  S(171620),  S(170332),  S(169043),  S(167752),//  900 -  903
  S(166460),  S(165166),  S(163870),  S(162573),//  904 -  907
  S(161274),  S(159973),  S(158671),  S(157367),//  908 -  911
  S(156062),  S(154755),  S(153447),  S(152136),//  912 -  915
  S(150824),  S(149510),  S(148196),  S(146879),//  916 -  919
  S(145560),  S(144241),  S(142919),  S(141596),//  920 -  923
  S(140271),  S(138945),  S(137618),  S(136288),//  924 -  927
  S(134957),  S(133624),  S(132290),  S(130954),//  928 -  931
  S(129616),  S(128277),  S(126936),  S(125595),//  932 -  935
  S(124250),  S(122905),  S(121558),  S(120210),//  936 -  939
  S(118860),  S(117508),  S(116155),  S(114800),//  940 -  943
  S(113444),  S(112086),  S(110726),  S(109366),//  944 -  947
  S(108003),  S(106638),  S(105273),  S(103905),//  948 -  951
  S(102537),  S(101166),  S( 99795),  S( 98421),//  952 -  955
  S( 97046),  S( 95669),  S( 94291),  S( 92912),//  956 -  959
  S( 91530),  S( 90147),  S( 88763),  S( 87377),//  960 -  963
  S( 85990),  S( 84601),  S( 83211),  S( 81819),//  964 -  967
  S( 80425),  S( 79030),  S( 77634),  S( 76236),//  968 -  971
  S( 74836),  S( 73435),  S( 72032),  S( 70628),//  972 -  975
  S( 69223),  S( 67816),  S( 66407),  S( 64996),//  976 -  979
  S( 63585),  S( 62172),  S( 60757),  S( 59341),//  980 -  983
  S( 57923),  S( 56504),  S( 55083),  S( 53661),//  984 -  987
  S( 52238),  S( 50813),  S( 49386),  S( 47957),//  988 -  991
  S( 46528),  S( 45096),  S( 43664),  S( 42230),//  992 -  995
  S( 40794),  S( 39357),  S( 37918),  S( 36478),//  996 -  999
  S( 35037),  S( 33594),  S( 32149),  S( 30703),// 1000 - 1003
  S( 29256),  S( 27807),  S( 26356),  S( 24905),// 1004 - 1007
  S( 23452),  S( 21997),  S( 20540),  S( 19083),// 1008 - 1011
  S( 17624),  S( 16162),  S( 14701),  S( 13238),// 1012 - 1015
  S( 11772),  S( 10306),  S(  8838),  S(  7369),// 1016 - 1019
  S(  5898),  S(  4425),  S(  2952),  S(  1477) // 1020 - 1023
};

inline uint32_t Histogram::size () const
{
   ESize_t bsize = 0;
   int     bin0  = m_bins[0];
   int     bin1  = m_bins[1];
   Histogram::Acc_t total = 0;


   //print_integration_title ();

   // Need 1 more bit so the total does not wrap around
   SIZE_LOOP:
   for (int idx = 0;; idx++)
   {
      // Select the correct parameters for the L—OOP_TRIPCOUNT
      #if   HISTOGRAM_K_NBINS==32
            #pragma HLS LOOP_TRIPCOUNT min=10 max=32 avg=16
      #elif HISTOGRAM_K_NBINS==64
            #pragma HLS LOOP_TRIPCOUNT min=10 max=64 avg=32
      #elif HISTOGRAM_K_NBINS==128
            #pragma HLS LOOP_TRIPCOUNT min=10 max=128 avg=64
      #else
            #error "NBINS not one of 16,32,64"
      #endif

      #pragma HLS PIPELINE
      Histogram::Entry_t cnt = m_bins[idx];
      ETable_t             e = ETable[cnt];
      bsize    += e;
      total    += cnt;
      //print_integration_line (idx, cnt, total, e, bsize);

      if (total == PACKET_K_NSAMPLES - 1) break;
   }

   // Remove the contribution from the first ADC
   //  e = ETable[cnt] * (cnt - 1) / cnt
   //    = ETable[cnt] * 1 - ETable[cnt]/cnt
   // with rounding
   //    = ETable[cnt] - (ETable[cnt] + cnt/2)/cnt;
   //// !!!! KLUDGE !!! -- not using the bsize yet so ignore the correction
   ///Histogram::Entry_t cnt = bin1;
   ///bsize -= (ETable[cnt] + cnt/2)/cnt;

   /* Round up to the nearest bit */
   bsize = (bsize >> Histogram::NBits) + 1;

   return bsize;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *
 * \brief Performs two functions.
 *           -# Replaces the bins with the integrated count
 *           -# Computes the size, in bits, of the encoded values that
 *              went into the accumulation of this histogram.
 *
 * \par
 *  The contents of each bin is replaced with the integrated sum of the
 *  previous bins. This results in a table that can be used for doing
 *  the arithimetic probability encoding.
 *
 * \par
 *  The size of the encoded data is computed from the entropy of the
 *  distribution. This is just the sum of bin[i] * log2 (bin[i]). With
 *  the number of entries fixed at 1024, a table, bin[i] must have a
 *  value from 0-1024. To avoid computing log2 in the FPGA, a precomputed
 *  table is used.
 *
 * \note
 *  Because the width of a bin is fixed at 10 bits (or more precisely, the
 *  number of bits to hold PACKET_K_NSAMPLES), the maximum value of 1024
 *  will rollover to 0.  In this case, the returned value in bits will be
 *  0, which, is in fact the correct answer. The integrated histogram will
 *  be all zeros.  In this case all the values are the same and the
 *  \e encoded data will just be that value.
 *
\* ---------------------------------------------------------------------- */
inline uint32_t Histogram::integrate_and_size ()
{
   ESize_t bsize = 0;
   int     bin0  = m_bins[0];
   int     bin1  = m_bins[1];

   //print_integration_title ();

   // Need 1 more bit so the total does not wrap around
   Histogram::Acc_t total = 0;
   INTEGRATION_LOOP:
   for (int idx = 0;; idx++)
   {
      // Select the correct parameters for the L—OOP_TRIPCOUNT
      #if   HISTOGRAM_K_NBINS==32
            #pragma HLS LOOP_TRIPCOUNT min=10 max=32 avg=16
      #elif HISTOGRAM_K_NBINS==64
            #pragma HLS LOOP_TRIPCOUNT min=10 max=64 avg=32
      #elif HISTOGRAM_K_NBINS==128
            #pragma HLS LOOP_TRIPCOUNT min=10 max=128 avg=64
      #else
            #error "NBINS not one of 16,32,64"
      #endif

      #pragma HLS PIPELINE
      Histogram::Entry_t cnt = m_bins[idx];
      ETable_t             e = 0; ///// ETable[cnt];
      bsize    += e;
      ///print_integration_line (idx, cnt, total, e, bsize);
      m_bins[idx] = total;
      total      += cnt;
      if (total == PACKET_K_NSAMPLES) break;
   }

   // Remove the contribution from the first ADC
   //  e = ETable[cnt] * (cnt - 1) / cnt
   //    = ETable[cnt] * 1 - ETable[cnt]/cnt
   // with rounding
   //    = ETable[cnt] - (ETable[cnt] + cnt/2)/cnt;
   //// !!!! KLUDGE !!! -- not using the bsize yet so ignore the correction
   ///Histogram::Entry_t cnt = bin1;
   ///bsize -= (ETable[cnt] + cnt/2)/cnt;

   /* Round up to the nearest bit */
   bsize = (bsize >> Histogram::NBits) + 1 + bin0 * 12;

   return bsize;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Prints the histogram
 *
\* ---------------------------------------------------------------------- */
void inline Histogram::print () const
{
   #define COLUMNS 0x20

   uint32_t bit_size = size();
   std::cout << "Channel: " << m_id << " bit size: " << std::hex << bit_size
             << " lastgt2:1:0 "  << std::hex << std::setw ( 5)
             << m_lastgt2 << ':' << m_lastgt1 << '"' << m_lastgt0
             << " overflow: " << std::hex << m_min << ':' << m_max
             << " OMask: "<< std::hex << std::setw ((Histogram::NBins+1)/4) << std::setfill ('0')
             << m_omask
 ///            << " DMask: " << std::hex << std::setw ((Histogram::NBins+1)/4) << std::setfill ('0')
 ///            << m_dmask
            << std::endl;

   HISTOGRAM_PRINT_LOOP:
   for (int idx = 0; idx < Histogram::NBins; idx++)
   {
      int col = idx % COLUMNS;
      if (col == 0) printf ("%2.2x:", idx);
      int val = m_bins[idx];
      char c = (idx & 0xf) == 0 ? ':'
             : (idx & 0x7) == 0 ? '.' : ' ';

      printf ("%c%3.3x", c, val);
      if (col == (COLUMNS-1)) putchar ('\n');
   }

   #undef COLUMNS
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Prints an array of histograms
 *
 *  \param[in]  hists The  array of histograms to print
 *  \param[in] nhists The number of histograms to print
 *
\* ---------------------------------------------------------------------- */
inline void Histogram::print (Histogram const hists[], int nhists)
{
   HISTOGRAMS_PRINT_LOOP:
   for (int idx = 0; idx < nhists; idx++)
   {
      hists[idx].print ();
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Prints the title line used when displaying the results of the
 *         integrated and size method
 *
\* ---------------------------------------------------------------------- */
inline void Histogram::print_integration_title ()
{
   std::cout << "Idx  Bin Total         ESize           BSize " << std::endl;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*@
 *
 *  \brief Prints the result of the integration and size method for each
 *         bin.
 *
 *  \param[in]   bin  Which bin
 *  \param[in]   cnt  The number of entries in the bin
 *  \param[in] total  The integrated count up to, but not including, this
 *                    bin
 *  \param[in]     e  The number of bits that this entry will contribute to
 *                    the encoded data
 *  \param[in]     b  The integrated number of bits, including this bin
 *
\* ---------------------------------------------------------------------- */
void Histogram::print_integration_line (int                 bin,
                                        Histogram::Entry_t  cnt,
                                        Histogram::Acc_t  total,
                                        Histogram::ETable_t   e,
                                        Histogram::ESize_t    b)
{

#  if   HISTOGRAM_K_NBINS<10
       #define FILL 1
#  elif HISTOGRAM_K_NBINS<100
      #define FILL  2
#  else
      #define FILL  3
#  endif

   using namespace std;
   int efull     = e >> Histogram::NBits;
   int efraction = e %  (1<<Histogram::NBits);
   int bfull     = b >> Histogram::NBits;
   int bfraction = b %  (1<<Histogram::NBits);

   cout << setw (3) << bin
        << setw (6) << cnt
        << setw (6) << total
        << setfill (' ')
        << setw (9) << e << setw (5) << efull << '.' << setw(FILL) << setfill('0') << efraction
        << setfill (' ')
        << setw (9) << b << setw (5) << bfull << '.' << setw(FILL) << setfill('0') << bfraction
        << setfill (' ')
        << endl;

#  undef FILL
}
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */
/* END IMPLEMENTATION: Histogram                                          */
/* ---------------------------------------------------------------------- */


#endif
