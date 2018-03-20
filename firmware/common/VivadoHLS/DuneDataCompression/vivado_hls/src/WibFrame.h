// -*-Mode: C++;-*-

#ifndef WIB_FRAME_H
#define WIB_FRAME_H

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     WibFrame.h
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
   2017.04.19 jjr Updated to the format Eric Hazen published on 2017.03.13
   2016.10.18 jjr Corrected K28.5 definition, was 0xDC -> 0xBC
   2016.06.14 jjr Created

\* ---------------------------------------------------------------------- */



/*
     WORD     Contents
        0     Err [16] | Rsvd[24] | Slot [3] | Crate[5] | Fiber[3] | Version[5] | K28.5[8]
        1     Timestamp [64]

              ColdData Stream 1
        2     CvtCnt[16] | ChkSums_hi[16] | ChkSums_lo[16] | Rsvd[8] | Err2_1[8]
        3              Hdrs[32]           |       Rsvd[16] |    ErrReg[16]
        4-15  ColdData.Adcs

              ColdData Stream 2
        16    CvtCnt[16] | ChkSums_hi[16] | ChkSums_lo[16] | Rsvd[8] | Err2_1[8]
        17             Hdrs[32]           |       Rsvd[16] |    ErrReg[16]
        18-29 ColdData.Adcs


     K28.1 = 0x3c
     K28.2 = 0x5c
     K28.5 = 0xDC
     +----------------+----------------+----------------+----------------+
     |3333333333333333|2222222222222222|1111111111111111|                |
     |fedcba9876543210|fedcba9876543210|fedcba9876543210|fedcba9876543210| 
     +----------------+----------------+----------------+----------------+
 0   |      Error     |    Reserved[24]         SltCrate|FbrVersn   K28.5|
 1   |                    GPS TImestamp[64]                              |
     +================+================+================+================+
     |                            Channels 0 - 127                       |
     +----------------+----------------+----------------+----------------+
 2   |          Timestamp              |   CheckSums    |  Rsvd   SErr   |
 3   |           Hdrs[7-0]             |    Reserved    |  Error Register|
     +----------------+----------------+----------------+----------------+
 4   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S1
 5   |aaaaaaaa99999999 9999888888888888 7777777777776666 6666666655555555|  &
 6   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S2
     +----------------+----------------+----------------+----------------+
 7   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S3
 8   |aaaaaaa999999999 9999888888888888 7777777777776666 6666666655555555|  &
 9   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S4
     +----------------+----------------+----------------+----------------+
10   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S5
11   |aaaaaaa999999999 9999888888888888 7777777777776666 6666666655555555|  &
12   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S6
     +----------------+----------------+----------------+----------------+
13   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S7
14   |aaaaaaa999999999 9999888888888888 7777777777776666 6666666655555555|  &
15   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S8
     +================+================+================+================+
     |                            Channels 128 - 255                     |
     +----------------+----------------+----------------+----------------+
16   |          Timestamp              |    CheckSums   |  Rsvd   SErr   |
17   |           Hdrs[7-0]             |    Reserved    |  Error Register|
     +----------------+----------------+----------------+----------------+
18   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S1
19   |aaaaaaaa99999999 9999888888888888 7777777777776666 6666666655555555|  &
20   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S2
     +----------------+----------------+----------------+----------------+
21   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S3
22   |aaaaaaa999999999 9999888888888888 7777777777776666 6666666655555555|  &
23   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S4
     +----------------+----------------+----------------+----------------+
24   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S5
25   |aaaaaaa999999999 9999888888888888 7777777777776666 6666666655555555|  &
26   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S6
     +----------------+----------------+----------------+----------------+
27   |5555444444444444 3333333333332222 2222222211111111 1111000000000000| S7
28   |aaaaaaa999999999 9999888888888888 7777777777776666 6666666655555555|  &
29   |ffffffffffffeeee eeeeeeeedddddddd ddddcccccccccccc bbbbbbbbbbbbaaaa| S8
     +----------------+----------------+----------------+----------------+

*/


#include <ap_int.h>
#include <stdint.h>

#define WIBFRAME_COMPOSE 1

#if !(__SYNTHESIS__) || !(WIBFRAME_COMPOSE)
#define WIBFRAME_COMPOSE 1
#endif


#if WIBFRAME_COMPOSE
/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief This is the data as it appears coming into the data handling
 *         module. This class is provided for the mainly for the purpose
 *         of composing simulated data frames
 *
 * \par
 *  The utility of expressing this as a class is a bit dubious.  It does
 *  provide many static routines to combine the intrinsic 16-bit data
 *  of the COLDDATA and WIB into 64-bit values which are transmitted to
 *  the FPGA data handling module via a 64-bit AXI stream.
 *
 *  So while this class is likely every to be instantiated, it does provide
 *  a somewhat formal definition of the data layout.
 *
\* ---------------------------------------------------------------------- */
class WibFrame
{
public:

   static const int VersionNumber = 1;

   enum K28
   {
      K28_1 = 0x3c,
      K28_2 = 0x5C,
      K28_5 = 0xBC
   };

   typedef uint64_t         DataWord;

   // Typedefs: Data word 0
   typedef ap_uint< 8>     CommaChar;

   typedef ap_uint< 5>       Version;

   typedef ap_uint< 3>         Fiber;
   typedef ap_uint< 5>         Crate;
   typedef ap_uint< 3>          Slot;
   typedef ap_uint<11>            Id;

   typedef ap_uint<24>          Rsvd;

   typedef ap_uint<4>     ErrCapture;
   typedef ap_uint<4>        ErrAsic;
   typedef ap_uint<8>        ErrBits;
   typedef ap_uint<16>       ErrWord;


   // Typedefs: Data word 1
   typedef ap_uint<32>     Timestamp;

   /* ------------------------------------------------------------------- *//*!
    *
    * \brief Describes the cold data streams from the front-ends.
    *
    * \par
    *  There are two cold data streams. Each stream is composed of two
    *  separate links. Each link has the same basic structure,
    *     -#  2 x 8-bit error words
    *     -#  1 x 8-bit reserved word
    *     -#  8 x 8-bit header words
    *     -# 64 x 12-bit ADCs densely packed.
    *
    *  This class is not quite pure. The first 16-bit word is provided
    *  by the WIB and contains information about any errors encountered
    *  when reading the 2 links.  Unfortunately, the nomenclature used
    *  to identify the links differs between the COLDDATA and WIB. The
    *  COLDDATA identifies the 2 links as A & B, whereas the WIB uses
    *  1 & 2.
    *
    *  Rather than adopting one or the other, the naming used here uses
    *  the names assigned by the originator. So A & B for COLDDATA and
    *  1 & 2 for the WIB
    *
   \* ------------------------------------------------------------------- */
   struct ColdData
   {
      // WORD 0 fields
      typedef ap_uint<4>        ErrBits;
      typedef ap_uint<8>        ErrWord;  // Stream 0 & 1 ErrBits

      typedef ap_uint<8>          Rsvd0;

      typedef ap_uint<8>   ChecksumByte;  // Checksum lo | hi, streams 1,2
      typedef ap_uint<16>  ChecksumWord;  // Checksum [1,2]Lo or [1,2] Hi
      typedef ap_uint<32>     Checksums;  // Checksums Lo[16] | Hi[16]

      typedef ap_uint<16>  ConvertCount;  // 2MHz AD Convert, resets with synch


      static ErrWord         error (ErrBits   erra, ErrBits   errb);

      // Assemble lo or hi byte of the checksum A and B into 16 bit word
      static ChecksumWord checksum (ChecksumByte a, ChecksumByte b);

      // Assemble lo or hi word of the checksum A and B into 32 bit word
      static Checksums   checksums (ChecksumWord a, ChecksumWord b);

      static uint64_t s0 (ErrWord          err,
                          ChecksumWord    cs_a,
                          ChecksumWord    cs_b,
                          ConvertCount cvt_cnt);

      static uint64_t s0 (ErrWord          err,
                          Rsvd0          rsvd0,
                          ChecksumWord    cs_a,
                          ChecksumWord    cs_b,
                          ConvertCount cvt_cnt);

      static uint64_t s0 (ErrBits         err1,
                          ErrBits         err2,
                          Rsvd0           rsvd,
                          ChecksumByte  csA_lo,
                          ChecksumByte  csB_lo,
                          ChecksumByte  csA_hi,
                          ChecksumByte  csB_hi,
                          ConvertCount cvt_cnt);

      // WORD 1 fields
      typedef ap_uint<16>        ErrReg;
      typedef ap_uint<16>         Rsvd1;
      typedef ap_uint<4>            Hdr;
      typedef ap_uint<32>          Hdrs;

      static Hdrs   hdrs (Hdr const hdr[8]);
      static uint64_t s1 (ErrReg errReg, Rsvd1 rsvd1, Hdrs hdrs);
      static uint64_t s1 (ErrReg errReg,              Hdrs hdrs);

      uint64_t      m_s0;  /*!< ColdData header word 0                    */
      uint64_t      m_s1;  /*!< ColdData header word 1                    */
      uint64_t m_adcs[12]; /*!< The 64 x 12-bit adcs                      */
   };
   /* ------------------------------------------------------------------- */


public:
   uint64_t           m_w0; /*!< W16  0 -  3                */
   uint64_t           m_w1; /*!< W16  4 -  7                */
   ColdData  m_coldData[2]; /*!< WIB 2 cold data streams    */


   // Word 0 construction
   static Id           id (Crate               crate,
                           Slot                 slot,
                           Fiber               fiber);

   static ErrWord errWord (ErrCapture     errCapture,
                           ErrAsic           errAsic,
                           ErrBits           errBits);

   static DataWord     w0 (CommaChar       commaChar,
                           Id                     id,
                           Rsvd                 rsvd,
                           ErrWord            errWrd);

   static DataWord     w0 (Id                     id,
                           ErrWord            errWrd);

   // Word 1 construction
   static DataWord     w1 (Timestamp       timestamp);
};
/* ---------------------------------------------------------------------- */

   



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Constructs the concatenated error word
 *  \return The concantenated WIB count
 *
 *  \param[in]   errCapture Errors that happened whil capturing the
 *                          COLDDATA ColdData.
 *  \param[in]     errAsic  Any error bit that was set in the ith
 *                          COLDDATA ColdData.
 *  \param[in]     errBits  ???
 *
\* ---------------------------------------------------------------------- */
inline WibFrame::ErrWord WibFrame::errWord (WibFrame::ErrCapture errCapture,
                                            WibFrame::ErrAsic       errAsic,
                                            WibFrame::ErrBits       errBits)

{
   WibFrame::ErrWord err = (errBits, errAsic, errCapture);
   return err;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief   Constructs data word 0 of the WIB frame in its most general
 *           form.
 *  \return  Word 0 of the WibFrame
 *
 *  \param[in[   commaChar  The initial 8b/10b comma character. Normally this
 *                          is the K28.5 character, but this allows the user
 *                          to specify it for testing purposes.
 *  \param[in]          id  The packed Slot #, Crate # and Fiber #
 *  \param[in]         rsvd Nominally 0, but can be specified for testing
 *                          purposes
 *  \param[in]      errWord The packet error word
 *
\* ---------------------------------------------------------------------- */
inline WibFrame::DataWord WibFrame::w0 (CommaChar       commaChar,
                                        Id                     id,
                                        Rsvd                 rsvd,
                                        ErrWord            errWrd)
{
   static const WibFrame::Version  versionNumber (VersionNumber);

   WibFrame::DataWord w = (errWrd, rsvd, id, versionNumber, commaChar);
   return             w;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Constructs the WIB identifier from the WIB crate, slot and
 *           fiber number
 *   \return The WIB identifier
 *
 *   \param[in]  crate  The crate number
 *   \param[in]   slot  The slot  number within the crate
 *   \param[in]  fiber  The fiber number within the card's slot
 *
 \* ---------------------------------------------------------------------- */
inline WibFrame::Id WibFrame::id (WibFrame::Crate  crate,
                                  WibFrame::Slot    slot,
                                  WibFrame::Fiber  fiber)
{
   WibFrame::Id id = (slot, crate, fiber);
   return       id;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Constructs WIB frame data word 1 from the WIB timestamp
 *
 *   \return WIB frame data word 1
 *
 *   \param[in]  timestamp  The WIB timestamp
 *
\* ---------------------------------------------------------------------- */
inline WibFrame::DataWord WibFrame::w1 (WibFrame::Timestamp timestamp)
{
   WibFrame::DataWord w = (timestamp);
   return             w;
}
/* ---------------------------------------------------------------------- */



/* ColdData WORD 0*/


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes the concatenation of the two 4-bit ColdData error
 *          fields
 *  \return The concatenation of the two 4-bit ColdData error fields
 *
 *  \param[in] err1  ColdData 1 errors
 *  \param[in] err2  ColdData 2 errors
 *
\* ---------------------------------------------------------------------- */
inline WibFrame::ColdData::ErrWord
       WibFrame::ColdData::error (WibFrame::ColdData::ErrBits err1,
                                  WibFrame::ColdData::ErrBits err2)
{
   ErrWord w = (err2, err1);
   return w;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes the concatenation of the lo or hi bits of the A and
 *          B checksums
 * \return  The concatenation of the lo or hi bits of the A and B checksums
 *
 *  \param[in] a  The lo (or hi) 8-bits of checksum A
 *  \param[in] b  The lo (or hi) 8-bits of checksum B
 *
\* ---------------------------------------------------------------------- */
inline WibFrame::ColdData::ChecksumWord
       WibFrame::ColdData::checksum (WibFrame::ColdData::ChecksumByte a,
                                     WibFrame::ColdData::ChecksumByte b)
{
   ChecksumWord w = (b, a);
   return  w;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes the 32-bit field from the A amd B checksums
 * \return  The composed 32-bit field
 *
 *  \param[in] a  The 16-bit checksum A
 *  \param[in] b  The 16-bit checksum B
 *
\* ---------------------------------------------------------------------- */
inline WibFrame::ColdData::Checksums
       WibFrame::ColdData::checksums (WibFrame::ColdData::ChecksumWord a,
                                      WibFrame::ColdData::ChecksumWord b)
{
   Checksums w = (b, a);
   return w;
}


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes word 0 of a ColdData
 *  \return The composed word
 *
 *  \param[in]      err  The contanation of the 2 4 bit ColdData1 and ColdData2
 *                       err fields
 *  \param[in]     rsvd0 A reserved field. This is normally 0, but is exposed
 *                       for testing reasons
 *  \param[in]      cs_a The 16-bit checksum for A
 *  \param[in]      cs_b The 16-bit checksum for B
 *  \param[in]   cvt_cnt The COLDDATA convert count.
 *
\* ---------------------------------------------------------------------- */
inline uint64_t WibFrame::ColdData::s0 (WibFrame::ColdData::ErrWord          err,
                                        WibFrame::ColdData::Rsvd0          rsvd0,
                                        WibFrame::ColdData::ChecksumWord    cs_a,
                                        WibFrame::ColdData::ChecksumWord    cs_b,
                                        WibFrame::ColdData::ConvertCount cvt_cnt)
{
   // Rearrange the checksums into hi lo pieces
   ChecksumWord cs_lo = (cs_b ( 7,0), cs_a( 7,0));
   ChecksumWord cs_hi = (cs_b (15,7), cs_a(15,7));

   uint64_t w = (cvt_cnt, cs_hi, cs_lo, err);
   return   w;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes word 0 of a ColdData
 *  \return The composed word
 *
 *  \param[in]      err  The contanation of the 2 4 bit ColdData1 and ColdData2
 *                       err fields
 *  \param[in]      cs_a The 16-bit checksum for A
 *  \param[in]      cs_b The 16-bit checksum for B
 *  \param[in] timestamp The COLDDATA timestamp. This must match the WIB's
 *                       convert count
\* ---------------------------------------------------------------------- */
inline uint64_t WibFrame::ColdData::s0 (WibFrame::ColdData::ErrWord          err,
                                        WibFrame::ColdData::ChecksumWord    cs_a,
                                        WibFrame::ColdData::ChecksumWord    cs_b,
                                        WibFrame::ColdData::ConvertCount cvt_cnt)
{
   uint64_t w = s0 (err, 0, cs_a, cs_b, cvt_cnt);
   return w;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes word 0 of a ColdData
 *  \return The composed word
 *
 *  \param[in]      err1 ColdData 1 error field
 *  \param[in]      err2 ColdData 2 error field
 *  \param[in]     rsvd0 A reserved field. This is normally 0, but is exposed
 *                       for testing reasons
 *  \param[in]    csA_lo The lo 8 bits of checksum A
 *  \param[in]    csB_lo the lo 8 bits of checksum B
 *  \param[in]    csA_hi The hi 8 bits of checksum A
 *  \param[in]    csB_hi The hi 8 bits of checksum B
 *  \param[in]   cvt_cnt The COLDDATA convert count
 *
\* ---------------------------------------------------------------------- */
 inline uint64_t WibFrame::ColdData::s0 (WibFrame::ColdData::ErrBits         err1,
                                         WibFrame::ColdData::ErrBits         err2,
                                         WibFrame::ColdData::Rsvd0           rsvd,
                                         WibFrame::ColdData::ChecksumByte  csA_lo,
                                         WibFrame::ColdData::ChecksumByte  csB_lo,
                                         WibFrame::ColdData::ChecksumByte  csA_hi,
                                         WibFrame::ColdData::ChecksumByte  csB_hi,
                                         WibFrame::ColdData::ConvertCount cvt_cnt)
{
    uint64_t w = s0 (cvt_cnt, csB_hi, csA_hi, csB_lo, csA_lo, rsvd, err2, err1);
    return w;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Composes the concatenation of all 8 of the 4 bit headers
 *  \return The concatentation of all 8 of the 4-bit headers
 *
 *  \param[in] hdr The 8 4-bit ADC ColdData headers
 *
\* ---------------------------------------------------------------------- */
 inline WibFrame::ColdData::Hdrs hdrs (WibFrame::ColdData::Hdr const hdr[8])
 {
    WibFrame::ColdData::Hdrs hdrs = (hdr[7], hdr[6], hdr[5], hdr[4],
                                     hdr[3], hdr[2], hdr[1], hdr[0]);

    return hdrs;
 }
 /* ---------------------------------------------------------------------- */



 /* ---------------------------------------------------------------------- *//*!
  *
  *  \brief  Composes the ColdData header word 1
  *  \return The composed ColdData header word 1
  *
  *  \param[in]  errReg  The 16-bit error register
  *  \param[in]   rsvd1  A reserved field. It should be 0, but is exposed
  *                      for testing purposes
  *  \param[in]    hdrs  The concatenation of the 8 ADC 4-bit ColdData headers
  *
 \* ---------------------------------------------------------------------- */
 inline uint64_t WibFrame::ColdData::s1 (WibFrame::ColdData::ErrReg errReg,
                                         WibFrame::ColdData::Rsvd1   rsvd1,
                                         WibFrame::ColdData::Hdrs     hdrs)
{
    uint64_t w = (hdrs, rsvd1, errReg);
    return w;
}
 /* ---------------------------------------------------------------------- */



 /* ---------------------------------------------------------------------- *//*!
   *
   *  \brief  Composes the ColdData header word 1
   *  \return The composed ColdData header word 1
   *
   *  \param[in]  errReg  The 16-bit error register
   *  \param[in]    hdrs  The concatenation of the 8 ADC 4-bit ColdData headers
   *
  \* ---------------------------------------------------------------------- */
inline uint64_t WibFrame::ColdData::s1 (WibFrame::ColdData::ErrReg errReg,
                                        WibFrame::ColdData::Hdrs     hdrs)
{
   uint64_t w = (hdrs, 0, errReg);
   return w;
}
/* ---------------------------------------------------------------------- */

#endif  /* WIBFRAME_COMPOSE */

#endif
   
