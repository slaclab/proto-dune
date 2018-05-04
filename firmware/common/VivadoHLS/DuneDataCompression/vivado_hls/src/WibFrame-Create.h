#ifndef _WIBFRAMECREATE_H_
#define _WIBFRAMECREATE_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     WibFrame-Create.h
 *  @brief    Creates/populates a WIB frame.  This is generally done when
 *            composing an frame with emulated data.
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
 *  2018.04.26
 *
 *  @par Credits:
 *  SLAC
 *
\* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\
   HISTORY
   -------
   DATE       WHO WHAT
   ---------- --- ---------------------------------------------------------
   2018.04.36 jjr Created/extracted from DuneCompressionCore_test
\* ---------------------------------------------------------------------- */


#include "WibFrame.h"


/* ---------------------------------------------------------------------- *//*!
 * 
 *   \brief Class used to compose a WIB frame from its various pieces.
 *
\* ---------------------------------------------------------------------- */ 
class WibFrameCreate
{
public:
      WibFrameCreate (int crate, int slot, int fiber);
      WibFrameCreate (WibFrame::Id                id);

public:
     void      fill (uint64_t                     timestamp,
                     uint16_t const adcs[MODULE_K_NCHANNELS]);
     
     void    print () const;

public:
   WibFrame::Id         m_id;  /*!< The packed Crate.Slot.Fiber           */
   WibFrame::Rsvd     m_rsvd;  /*!< The header word's reserved field      */
   WibFrame::ErrWord m_error;  /*!< The header word's error    field      */


   uint16_t        m_cvtcnt0;  /*!< The convert count, cold data stream 0 */
   uint16_t        m_cvtcnt1;  /*!< The convert count, cold data stream 1 */
   
   uint64_t        m_w64[30];  /*!< The composed WIB frame                */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief WibFrame constructor
 *  
 *  \param[in]   id  The precomposed WIB idenitier, i.e Crate.Slot.Fiber
 *  
\* ---------------------------------------------------------------------- */
WibFrameCreate::WibFrameCreate (WibFrame::Id id) :
         m_id      (   id),
         m_rsvd    (    0),
         m_error   (    0),
         m_cvtcnt0 (0x100),
         m_cvtcnt1 (0x100)
{
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief WibFrame constructor
 *  
 *  \param[in] crate The crate number of this WIB frame
 *  \param[in]  slot The slot  number of this WIB frame
 *  \param[in] fiber The fiber number of this WIB frame
 *  
\* ---------------------------------------------------------------------- */
WibFrameCreate::WibFrameCreate (int crate, int slot, int fiber) :
                m_id      (WibFrame::id (WibFrame::Crate (crate), 
                                         WibFrame::Slot  ( slot),
                                         WibFrame::Fiber (fiber))),
                m_rsvd    (    0),
                m_error   (    0),
                m_cvtcnt0 (0x100),
                m_cvtcnt1 (0x100)
{
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
/* Local Prototypes                                                       */
/* ---------------------------------------------------------------------- */
static void fill_colddata (uint64_t        w64[14], 
                           uint16_t         cvtcnt, 
                           uint16_t const adcs[64]);
;
static void pack64 (uint64_t                  *dst,
                    uint16_t const           *adcs);
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Creates a Wib Frame
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
void WibFrameCreate::fill (uint64_t                     timestamp,
                           uint16_t const adcs[MODULE_K_NCHANNELS])
 
{
   // Fill the 2 WIB header words
   m_w64[0] = WibFrame::w0 (WibFrame::K28_5, m_id, m_rsvd, m_error);
   m_w64[1] = WibFrame::w1 (timestamp);

   // Fill the 2 cold data streams
   fill_colddata (m_w64 +  2,  m_cvtcnt0, adcs +  0);
   fill_colddata (m_w64 + 16,  m_cvtcnt1, adcs + 64);
   
   // Increment the convert counts
   m_cvtcnt0 += 1;
   m_cvtcnt1 += 1;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief Does a very primive hex dump of the WIB frame
 *  
 *
\* ---------------------------------------------------------------------- */
void WibFrameCreate::print () const
 {
   // --------------
   // Dump the frame
   // --------------
   for (int idx = 0; idx < 30; idx++)
   {
      std::cout << "Frame[" << std::setw (2) << idx << "] = "
                            << std::setw(16) << std::hex << m_w64[idx]
                << std::endl;
   }
   
   return;
 }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief  Fills one cold data stream with the specifiec convert count
 *          and ADCs
 *          
 *  \param[out]    w64  The array of 64-bit words to accept the filled
 *                      cold data stream data
 *  \param[ in] cvtcnt  The cold data stream's convert count.
 *  \param[ in]   adcs  The cold data stream's ADCs.      
 *
 *
\* ---------------------------------------------------------------------- */
static void fill_colddata (uint64_t        w64[14], 
                           uint16_t         cvtcnt,
                           uint16_t const adcs[64])
{   
   // ------------------
   // Cold header word 0
   // ------------------
   WibFrame::ColdData::ErrWord      err (0);
   WibFrame::ColdData::ChecksumWord csA (0);
   WibFrame::ColdData::ChecksumWord csB (0);
   w64[0] = WibFrame::ColdData::s0 (err, csA, csB, cvtcnt);

   
   // ------------------
   // Cold header word 1
   // ------------------
   WibFrame::ColdData::ErrReg errReg (0);
   WibFrame::ColdData::Hdrs     hdrs (0);
   w64[1] = WibFrame::ColdData::s1 (errReg, hdrs);

    
    // ---------------------------
    // Pack the colddata ADC words
    // ----------------------------
    pack64 (&w64[2], adcs);
    
    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/* PACK ADCS INTO THE COLDDATA STREAM                                     */
/* ---------------------------------------------------------------------- */
static inline uint64_t packA (uint16_t v0,
                              uint16_t v1,
                              uint16_t v2,
                              uint16_t v3,
                              uint16_t v4,
                              uint16_t v5);


static inline uint64_t packB (uint16_t v4,
                              uint16_t v5,
                              uint16_t v6,
                              uint16_t v7,
                              uint16_t v8,
                              uint16_t v9,
                              uint16_t vA,
                              uint16_t vB);


static inline uint64_t packC (uint16_t vA,
                              uint16_t vB,
                              uint16_t vC,
                              uint16_t vD,
                              uint16_t vE,
                              uint16_t vF);



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Packs 64 adcs into the cold data stream
 *
 *   \param[out]  dst  The destination array
 *   \param[ in] adcs  The adcs to pack
 *
\* ---------------------------------------------------------------------- */
static void pack64 (uint64_t *dst, uint16_t const *adcs)
{
   uint16_t const *p = adcs;

   for (int idx = 0; idx < 4; idx++)
   {
      dst[0] = packA (p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5]);
      dst[1] = packB (p[ 4], p[ 5], p[ 6], p[ 7], p[ 8], p[ 9], p[10], p[11]);
      dst[2] = packC (p[10], p[11], p[12], p[13], p[14], p[15]);

      p   += 16;
      dst += 3;
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static inline uint64_t packA (uint16_t v0,
                              uint16_t v1,
                              uint16_t v2,
                              uint16_t v3,
                              uint16_t v4,
                              uint16_t v5)

{
/*
          7        6        5        4        3        2        1        0
   +--------+--------+--------+--------+--------+--------+--------+--------+
   |2.3[7:0]|1.3[7:0]|2.2[b:4]|1.2[b.4]|2.2[3:0]|1.2[3:0]|2.1[7:0]|1.1[7:0]|
   |                                   |2.1[b:8]|1.1[b:8]|        |        |
   +--------+--------+--------+--------+--------+--------+--------+--------+

   +--------+--------+--------+--------+--------+--------+--------+--------+
   | v5[7:0]| v4[7:0]| v3[b:4]| v2[b.4]| v3[3:0]| v2[3:0]| v1[7:0]| v0[7:0]|
   |                                   | v1[b:8]| v0[b:8]|                 |
   +--------+--------+--------+--------+--------+--------+--------+--------+

*/
   uint64_t s;


   // ----   Byte 7,6
             s   = (v5 & 0xff);  // s5[7:0]
   s <<= 8;  s  |= (v4 & 0xff);  // s4[7:0]

   // ----   Byte 5,4
   s <<= 8;  s  |= (v3 >>   4);  // s3[b:4]
   s <<= 8;  s  |= (v2 >>   4);  // s2[b:4]

   // ----   Byte 3
   s <<= 4;  s  |= (v3 &  0xf);  // s3[3:0]
   s <<= 4;  s  |= (v1 >>   8);  // s1[b:8]

   // ----   Byte 2
   s <<= 4;  s  |= (v2 &  0xf);  // s2[3:0]
   s <<= 4;  s  |= (v0 >>   8);  // s0[b:8]

   // ----   Byte 1,0
   s <<= 8;  s  |= (v1 & 0xff);  // s1{7:0]
   s <<= 8;  s  |= (v0 & 0xff);  // s0[7:0]


   return s;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static inline uint64_t packB (uint16_t v4,
                              uint16_t v5,
                              uint16_t v6,
                              uint16_t v7,
                              uint16_t v8,
                              uint16_t v9,
                              uint16_t vA,
                              uint16_t vB)
{
/*
         7        6        5        4        3        2        1        0
  +--------+--------+--------+--------+--------+--------+--------+--------+
  |2.6[3:0]|1.6[3:0]|2.5[7:0]|1.5[7:0]|2.4[b:4]|1.4[b:4]|2.4[3:0]|1.4[3:0]|
  |2.5[b:8]|1.5[b:8]|                 |        |        |2.3[b:8]|1.3[b:8]|
  +--------+--------+--------+--------+--------+--------+--------+--------+

  +--------+--------+--------+--------+--------+--------+--------+--------+
  |  B[3:0]|  A[3:0]|  9[7:0]|  8[7:0]|  7[b:4]|  6[b:4]|  7[3:0]|  6[3:0]|
  |  9[b:8]|  8[b:8]|                 |        |        |  5[b:8]|  4[b:8]|
  +--------+--------+--------+--------+--------+--------+--------+--------+

*/
   uint64_t s;

   // ----    Byte 7
              s   = vB &  0xf;  // vB[3:0]
   s <<= 4;   s  |= v9 >>   8;  // v9[b:8]

   // ----    Byte 6
   s <<= 4;   s  |= vA &  0xf;  // vA[3:0]
   s <<= 4;   s  |= v8 >>   8;  // v8[b:8]

   // ----    Byte 5,4
   s <<= 8;   s  |= v9 & 0xff;  // v9[7:0]
   s <<= 8;   s  |= v8 & 0xff;  // v8[7:0]

   // ----    Byte 3,2
   s <<= 8;   s  |= v7 >>   4;  // v7[b:4]
   s <<= 8;   s  |= v6 >>   4;  // v6[b:4]

   // ----    Byte 1
   s <<= 4;   s  |= v7 &  0xf;  // v7[3:0]
   s <<= 4;   s  |= v5 >>   8;  // v5[b:8]

   // ----    Byte 0
   s <<= 4;   s  |= v6 &  0xf;  // v6[3:0]
   s <<= 4;   s  |= v4 >>   8;  // v4[b:8]


   return s;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static inline uint64_t packC (uint16_t vA,
                              uint16_t vB,
                              uint16_t vC,
                              uint16_t vD,
                              uint16_t vE,
                              uint16_t vF)
{

/*

         7        6        5        4        3        2        1        0
  +--------+--------+--------+--------+--------+--------+--------+--------+
  |2.8[b:4]|1.8[b:4]|2.8[3:0]|1.8[3.0]|2.7[7:0]|1.7[7:0]|2.6[b:4]|1.6[b:4]|
  |                 |2.7[b:8||1.7[b:8]|                                   |
  +--------+--------+--------+--------+--------+--------+--------+--------+

  +--------+--------+--------+--------+--------+--------+--------+--------+
  |  F[b:4]|  E[b:4]|  F[3:0]|  E[3.0]|  D[7:0]|  C[7:0]|  B[b:4]|  A[b:4]|
  |                 |  D[b:8||  C[b:8]|                                   |
  +--------+--------+--------+--------+--------+--------+--------+--------+

*/
   uint64_t s;

   // ----    Byte 7,6
              s   = vF >>   4;  // vF[b:4]
   s <<= 8;   s  |= vE >>   4;  // vE[b:4]

   // ----    Byte 5
   s <<= 4;   s  |= vF &  0xf;  // vF[3:0]
   s <<= 4;   s  |= vD >>   8;  // vD[b:8]

   // ----    Byte 4
   s <<= 4;   s  |= vE &  0xf;  // vE[3:0]
   s <<= 4;   s  |= vC >>   8;  // vC[b:8]

   // ----    Byte 3,2
   s <<= 8;   s  |= vD & 0xff;  // vD[7:0]
   s <<= 8;   s  |= vC & 0xff;  // vC[7:0]

   // ----    Byte 1,0
   s <<= 8;   s  |= vB >>   4;  // vB[b:4]
   s <<= 8;   s  |= vA >>   4;  // vA[b:4]

   return s;
}
/* ---------------------------------------------------------------------- */


#endif
