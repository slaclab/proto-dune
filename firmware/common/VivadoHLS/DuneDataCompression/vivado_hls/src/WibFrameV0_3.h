// -*-Mode: C++;-*-

#ifndef WIB_FRAMEV0_3_H
#define WIB_FRAMEV0_3_H

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     WibFrameV0_3.h
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
 *  2016.12.05
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
   2016.12.06 jjr Created Version 0.3

\* ---------------------------------------------------------------------- */



/*
     WORD     Contents
        0                        GPS Time[40]  | Slot [3] | Crate[5]  | Fiber[3] | Format[5]| K28.5[8]
        1     Stream ID[16]    | Trigger ID[16] | Erro[3]| ASIC[4] |  Capture[4] |        GPS Time[16]

              ColdData1
        2     ConvertCount[16] | CS B HI[8] | CS A HI[8] | CS B LO[8] CS A[8] | StreamErr2[4] | StreamErr1[4]
        3     Hdrs[32]                      | Reserved[16]                    |       Error Register[16]
        4-15  ColdData.Adcs

              ColdData2
        16    ConvertCount[16] | CS B HI[8] | CS A HI[8] | CS B LO[8] CS A[8] | StreamErr2[4] | StreamErr1[4]
        `7    Hdrs[32]                      | Reserved[16]                    |       Error Register[16] 
        18-28ColdData.Adcs




     K28.1 = 0x3c
     K28.2 = 0x5c
     K28.5 = 0xDC
     +----------------+----------------+----------------+----------------+
     |3333333333333333|2222222222222222|1111111111111111|                |
     |fedcba9876543210|fedcba9876543210|fedcba9876543210|fedcba9876543210| 
     +----------------+----------------+----------------+----------------+
 0   | GPS Timestamp  | GPS TimeStamp  |GPS TimeSltCrate|Fib Vers K28.5  |
 1   |    Stream ID   |   Trigger ID   | Error  AsicCapt| GPS Timestamp  |
     +================+================+================+================+
     |                            Channels 0 - 127                       |
     +----------------+----------------+----------------+----------------+
 2   |ColdDataCvtCount|ChkSumBhChkSumAh|ChkSumBlChkSumAl|ReservedStE2StE1|
 3   |           Hdrs[7-0]             |    Reserved    | Error Register |
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
16   |ColdDataCvtCOunt|ChkSUmBhChkSumAh|ChkSumBlChkSUmAl|ReservedStE2StE1|
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

#define WIBFRAMEV0_3_COMPOSE 1

#if !(__SYNTHESIS__) || !(WIBFRAME_COMPOSEV0_3)
#define WIBFRAMEV0_3_COMPOSE 1
#endif


#if WIBFRAMEV0_3_COMPOSE
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
class WibFrameV0_3
{
public:
   enum K28
   {
      K28_1 = 0x3c,
      K28_2 = 0x5C,
      K28_5 = 0xBC
   };
   
   typedef uint64_t          DataWord;   /*!< Unit of WibFrame is 64 bits */

public:
   class Header
   {
   public:
      // Typedefs: Data word 0 
      typedef ap_uint<8>       CommaChar; /*!< Initial Comma Characeter   */
      typedef ap_uint<5>          Format; /*!< Wib Frame Format Version   */
      typedef ap_uint< 3>          Fiber; /*!< Wib Fiber Number           */
      typedef ap_uint< 5>          Crate; /*!< Wib Crate Number           */
      typedef ap_uint< 3>           Slot; /*!< Wib Slot  Number           */
      typedef ap_uint<40> GpsTimestampLo; /*!< GPS Timestamp, low 40 bita */

      // Id:a convenient name for the Crate, Slot Fiber
      typedef ap_uint<11>             Id; /*!< Wib ID = Slot.Crate.Fiber  */

   
      // Typedefs: Data word 1
      typedef ap_uint<16> GpsTimestampHi; /*!< GPS Timestamp, hi 16 bits  */
      typedef ap_uint<4>      ErrCapture; /*!< Capture error bits         */
      typedef ap_uint<4>         ErrAsic; /*!< Asic error bits            */
      typedef ap_uint<8>         ErrBits; /*!< Other error bits           */
      typedef ap_uint<16>      TriggerId; /*!< Trigger Id                 */
      typedef ap_uint<16>       StreamId; /*!< Stream  Id                 */

      //   ErrWord: a convenient name for the front end error bits  
      typedef ap_uint<16>        ErrWord; /*!< ErrBits|ErrAsic|ErrCapture */
   
      typedef ap_uint<56>   GpsTimestamp; /*!< Full GpsTimestamp          */
      
      Header (CommaChar    commaChar,
              Crate            crate,
              Slot              slot,
              Fiber            fiber,
              GpsTimestamp timestamp,
              ErrCapture  errCapture,
              ErrAsic        errAsic,
              ErrBits        errBits,
              TriggerId    triggerId,
              StreamId      streamId);
      
      Header (CommaChar    commaChar,
              Id                  id,
              GpsTimestamp timestamp,
              ErrWord        errWord,
              TriggerId    triggerId,
              StreamId      streamId);
      
      Header (Id                  id,
              GpsTimestamp timestamp,
              ErrWord        errWord,
              TriggerId    triggerId,
              StreamId      streamId);
      

      // Extract the hi an lo fields of the GPS time
       static GpsTimestampLo gpsTimestampLo (GpsTimestamp timestamp);
       static GpsTimestampHi gpsTimestampHi (GpsTimestamp timestamp);
       
       // Word 0 construction
       static Id            id (Crate               crate,
                                Slot                 slot,
                                Fiber               fiber);

       static DataWord      w0 (Id                    id,
                                GpsTimestamp   timestamp);
       
       static DataWord
                            w0 (CommaChar      commaChar,
                                Id                    id,
                                GpsTimestamp   timestamp);

       static DataWord      w0 (CommaChar       commaChar,
                                Crate               crate,
                                Slot                 slot,
                                Fiber               fiber,
                                GpsTimestamp    timestamp);


       // Word 1 construction
       static ErrWord errWord (ErrCapture     errCapture,
                               ErrAsic           errAsic,
                               ErrBits           errBits);


       static DataWord     w1 (GpsTimestamp    timestamp,
                               ErrWord           errWord,
                               TriggerId       triggerId,
                               StreamId         streamId);
       
       static DataWord     w1 (GpsTimestamp    timestamp,
                               TriggerId       triggerId,
                               StreamId         streamId);
      
       static DataWord     w1 (GpsTimestamp    timestamp,
                               ErrCapture     errCapture,
                               ErrAsic           errAsic,
                               ErrBits           errBits,
                               TriggerId       triggerId,
                               StreamId         streamId);
   public:
       DataWord           m_w0; /*!< W16  0 -  3 */
       DataWord           m_w1; /*!< W16  4 -  7 */
   };
   
   /* ------------------------------------------------------------------- *//*!
    *
    * \brief Describes the cold data streams from the front-ends.
    *
    * \par
    *  There are two cold data streams. Each stream is composed of two
    *  separate links. Each link has the same basic structure, 3 16-bit
    *  header words and 64 x 12-bit ADCs densely packed.
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
      typedef ap_uint<8>        ErrWord;
      typedef ap_uint<8>          Rsvd0;
      typedef ap_uint<8>   ChecksumByte;
      typedef ap_uint<16>  ChecksumWord;
      typedef ap_uint<32>      Checksum;
      typedef ap_uint<16>  ConvertCount;

      static ErrWord         error (ErrBits err1,   ErrBits   err2);
      static ChecksumWord checksum (ChecksumByte a, ChecksumByte b);
      static Checksum     checksum (ChecksumWord a, ChecksumWord b);

      static DataWord s0 (ErrWord          err,
                          ChecksumWord    cs_a,
                          ChecksumWord    cs_b,
                          ConvertCount   count);

      static DataWord s0 (ErrWord          err,
                          Rsvd0          rsvd0,
                          ChecksumWord    cs_a,
                          ChecksumWord    cs_b,
                          ConvertCount   count);

      static DataWord s0 (ErrBits        err1,
                          ErrBits        err2,
                          Rsvd0          rsvd,
                          Checksum     csA_lo,
                          Checksum     csB_lo,
                          Checksum     csA_hi,
                          Checksum     csB_hi,
                          ConvertCount  count);

      // WORD 1 fields
      typedef ap_uint<16>        ErrReg;
      typedef ap_uint<16>         Rsvd1;
      typedef ap_uint<4>            Hdr;
      typedef ap_uint<32>          Hdrs;

      static Hdrs   hdrs (Hdr const hdr[8]);
      static DataWord s1 (ErrReg errReg, Rsvd1 rsvd1, Hdrs hdrs);
      static DataWord s1 (ErrReg errReg,              Hdrs hdrs);

   public:
      DataWord       m_s0;  /*!< ColdData header word 0                   */
      DataWord       m_s1;  /*!< ColdData header word 1                   */
      DataWord m_adcs[12]; /*!< The 64 x 12-bit adcs                      */
   };
   /* ------------------------------------------------------------------- */


public:
   Header           m_hdr;  /*!< Information common to the WIB frame data */
   ColdData  m_coldData[2]; /*!< WIB reserved word + 2 cold data streams  */
};
/* ---------------------------------------------------------------------- */

   

/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Selects low bits of the 56 bit GPS time that goes into WIB
 *          word 0
 *  \return The low bits of the 56-bit GPS time that goes into WIB word 0
 *
 *  \param[in]  timestamp The GPS timestamp
 *
\* ---------------------------------------------------------------------- */
inline  WibFrameV0_3::Header::GpsTimestampLo 
        WibFrameV0_3::Header::gpsTimestampLo (WibFrameV0_3::Header::GpsTimestamp timestamp)
{
   // This will just pick off the lo bits
   GpsTimestampLo lo = timestamp;
   return lo;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Selects high bits of the 56 bit GPS time that goes into WIB
 *          word 1
 *  \return The hig bits of the 56-bit GPS time that goes into WIB word 1
 *
 *  \param[in]  timestamp The GPS timestamp
 *
\* ---------------------------------------------------------------------- */
inline  WibFrameV0_3::Header::GpsTimestampHi 
        WibFrameV0_3::Header::gpsTimestampHi (WibFrameV0_3::Header::GpsTimestamp timestamp)
{
   // This will just pick off the lo bits
   GpsTimestampHi hi = timestamp >> GpsTimestampLo::width;
   return hi;
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
inline WibFrameV0_3::Header::Id 
       WibFrameV0_3::Header::id (WibFrameV0_3::Header::Crate  crate,
                                 WibFrameV0_3::Header::Slot    slot,
                                 WibFrameV0_3::Header::Fiber  fiber)
{
   Id idWord = (crate, slot, fiber);
   return idWord;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief   Constructs data word 0 of the WIB frame in its most general
 *           form from the concatenated count and error words
 *  \return  Word 0 of the WibFrame
 *
 *  \param[in] commaChar  The comma character to use. This is usually
 *                        the K28.5 character, but is here for testing
 *                        purposes.
 *  \param[in]        id  The Crate.Slot.Fiber concatenated identified word
 *  \parampin] timestamp  The GPS timestamp
 *
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord
       WibFrameV0_3::Header::w0 (WibFrameV0_3::Header::CommaChar    commaChar,
                                 WibFrameV0_3::Header::Id                  id,
                                 WibFrameV0_3::Header::GpsTimestamp timestamp)
{
   GpsTimestampLo timestampLo = gpsTimestampLo (timestamp);
   DataWord  w = (timestampLo, id, commaChar);
   return w;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief   Constructs data word 0 of the WIB frame in its most general
 *           form.
 *  \return  Word 0 of the WibFrame
 *
 *  \param[in]  count  The concatenated count word
 *  \parampin]  error  The concatenated error word
 *
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord 
       WibFrameV0_3::Header::w0 (WibFrameV0_3::Header::Id                  id,
                                 WibFrameV0_3::Header::GpsTimestamp timestamp)
{
   static const CommaChar K28_5C (WibFrame::K28_5);
   DataWord  w = w0 (K28_5C, id, timestamp);
   return w;
}
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
inline WibFrameV0_3::Header::ErrWord
       WibFrameV0_3::Header::errWord (WibFrameV0_3::Header::ErrCapture errCapture,
                                      WibFrameV0_3::Header::ErrAsic       errAsic,
                                      WibFrameV0_3::Header::ErrBits       errBits)

{
   ErrWord err = (errBits, errAsic, errCapture);
   return err;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Constructs WIB frame data word 1 from the WIB timestamp, the
 *           WIB error word and the trigger and stream id.
 *   \return WIB frame data word 1
 *
 *   \param[in]  timestamp  The WIB GPS timestamp
 *   \param[in]    errWord  The WIB error word
 *   \param[in]  triggerId  The WIB trigger ID
 *   \param[in]   streamId  The WIB stream ID
 *
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord
       WibFrameV0_3::Header::w1 (WibFrameV0_3::Header::GpsTimestamp timestamp,
                                 WibFrameV0_3::Header::ErrWord        errWord,
                                 WibFrameV0_3::Header::TriggerId    triggerId,
                                 WibFrameV0_3::Header::StreamId      streamId)
{
   GpsTimestampHi timestampHi = gpsTimestampHi (timestamp);
   DataWord                 w = (timestamp, errWord, triggerId, streamId);
   return w;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Constructs WIB frame data word 1 from its primitive pieces
 *   \return WIB frame data word 1
 *
 *   \param[in]  timestamp  The WIB GPS timestamp
 *   \param[in] errCapture  The WIB error capture word
 *   \param[in[    errAsic  The WIB error Asic word
 *   \param[in]    errBits  THe WIB error bits 
 *   \param[in]  triggerId  The WIB trigger ID
 *   \param[in]   streamId  The WIB stream ID
 *
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord
       WibFrameV0_3::Header::w1 (WibFrameV0_3::Header::GpsTimestamp    timestamp,
                                 WibFrameV0_3::Header::ErrCapture     errCapture,
                                 WibFrameV0_3::Header::ErrAsic           errAsic,
                                 WibFrameV0_3::Header::ErrBits           errBits,
                                 WibFrameV0_3::Header::TriggerId       triggerId,
                                 WibFrameV0_3::Header::StreamId         streamId)
{
   ErrWord err = errWord (errCapture, errAsic, errBits);
   DataWord  w = w1 (timestamp, err, triggerId, streamId);
   return    w;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Constructs WIB frame data word 1 with the error word = 0
 *   \return WIB frame data word 1
 *
 *   \param[in]  timestamp  The WIB timestamp
 *   \param[in]  triggerId  The WIB trigger ID
 *   \param[in]   streamId  The WIB stream ID
 *
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord 
       WibFrameV0_3::Header::w1 (WibFrameV0_3::Header::GpsTimestamp  timestamp,
                                 WibFrameV0_3::Header::TriggerId     triggerId,
                                 WibFrameV0_3::Header::StreamId       streamId)
{       
   DataWord w = w1 (timestamp, 0, triggerId, streamId);
   return   w;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief Most general form to a V0.3 WibFrame header constructor
 *  
 *  \param[in]  commaChar  The comma character to use. This is usually
 *                         the K28.5 character, but is here for testing
 *                         purposes.
 *  \param[in]      crate  The crate number
 *  \param[in]       slot  The slot  number within the crate
 *  \param[in]      fiber  The fiber number within the card's slot *                        
 *  \parampin]  timestamp  The GPS timestamp
 *  \param[in] errCapture  Errors that happened whil capturing the
 *                         COLDDATA ColdData.
 *  \param[in]     errAsic Any error bit that was set in the ith
 *                         COLDDATA ColdData.
 *  \param[in]     errBits  ???
 *  \param[in]   triggerId  The WIB trigger ID
 *  \param[in]    streamId  The WIB stream ID
 *  
\* ---------------------------------------------------------------------- */
WibFrameV0_3::Header::Header (CommaChar    commaChar,
                              Crate            crate,
                              Slot              slot,
                              Fiber            fiber,
                              GpsTimestamp timestamp,
                              ErrCapture  errCapture,
                              ErrAsic        errAsic,
                              ErrBits        errBits,
                              TriggerId    triggerId,
                              StreamId      streamId) :
              m_w0 (w0 (commaChar, id (crate, slot, fiber), timestamp)),
              m_w1 (w1 (timestamp,
                        errWord (errCapture, errAsic, errBits),
                        triggerId,
                        streamId))
{
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief Intermiate form to a V0.3 WibFrame header constructor
 *  
 *  \param[in]  commaChar  The comma character to use. This is usually
 *                         the K28.5 character, but is here for testing
 *                         purposes.
 *  \param[in]        id  The Crate.Slot.Fiber concatenated identified word                    
 *  \param[in]  timestamp  The GPS timestamp
 *  \param[in]    errWord  The WIB error word
 *  \param[in]   triggerId  The WIB trigger ID
 *  \param[in]    streamId  The WIB stream ID
 *  
\* ---------------------------------------------------------------------- */
WibFrameV0_3::Header::Header (CommaChar    commaChar,
                              Id                  id,
                              GpsTimestamp timestamp,
                              ErrWord        errWord,
                              TriggerId    triggerId,
                              StreamId      streamId) :
              m_w0 (w0 (commaChar, id, timestamp)),
              m_w1 (w1 (timestamp, errWord, triggerId, streamId))
{
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief Simplified form of a V0.3 WibFrame header constructor
 * 
 *  \param[in]        id  The Crate.Slot.Fiber concatenated identified word                    
 *  \param[in]  timestamp  The GPS timestamp
 *  \param[in]    errWord  The WIB error word
 *  \param[in]   triggerId  The WIB trigger ID
 *  \param[in]    streamId  The WIB stream ID
 *  
\* ---------------------------------------------------------------------- */
WibFrameV0_3::Header::Header (Id                  id,
                              GpsTimestamp timestamp,
                              ErrWord        errWord,
                              TriggerId    triggerId,
                              StreamId      streamId) :
              m_w0 (w0 (id, timestamp)),
              m_w1 (w1 (timestamp, errWord, triggerId, streamId))
{
   return;
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
inline WibFrameV0_3::ColdData::ErrWord
       WibFrameV0_3::ColdData::error (WibFrameV0_3::ColdData::ErrBits err1,
                                      WibFrameV0_3::ColdData::ErrBits err2)
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
inline WibFrameV0_3::ColdData::ChecksumWord
       WibFrameV0_3::ColdData::checksum (WibFrameV0_3::ColdData::ChecksumByte a,
                                         WibFrameV0_3::ColdData::ChecksumByte b)
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
inline WibFrameV0_3::ColdData::Checksum
       WibFrameV0_3::ColdData::checksum (WibFrameV0_3::ColdData::ChecksumWord a,
                                         WibFrameV0_3::ColdData::ChecksumWord b)
{
   Checksum w = (b, a);
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
 *  \param[in]     count The COLDDATA convert count
 *  
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord
       WibFrameV0_3::ColdData::s0 (WibFrameV0_3::ColdData::ErrWord          err,
                                   WibFrameV0_3::ColdData::Rsvd0          rsvd0,
                                   WibFrameV0_3::ColdData::ChecksumWord    cs_a,
                                   WibFrameV0_3::ColdData::ChecksumWord    cs_b,
                                   WibFrameV0_3::ColdData::ConvertCount   count)
{
   // Rearrange the checksums into hi lo pieces
   ChecksumWord cs_lo = (cs_b ( 7,0), cs_a( 7,0));
   ChecksumWord cs_hi = (cs_b (15,7), cs_a(15,7));

   DataWord w = (count, cs_hi, cs_lo, err);
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
 *  \param[in] timestamp The COLDDATA convert count
\* ---------------------------------------------------------------------- */
inline WibFrameV0_3::DataWord
       WibFrameV0_3::ColdData::s0 (WibFrameV0_3::ColdData::ErrWord          err,
                                   WibFrameV0_3::ColdData::ChecksumWord    cs_a,
                                   WibFrameV0_3::ColdData::ChecksumWord    cs_b,
                                   WibFrameV0_3::ColdData::ConvertCount   count)
{       
   DataWord w = s0 (err, 0, cs_a, cs_b, count);
   return   w;
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
 *  \param[in] timestamp The COLDDATA convert count
 *  
\* ---------------------------------------------------------------------- */
 inline WibFrameV0_3::DataWord
        WibFrameV0_3::ColdData::s0 (WibFrameV0_3::ColdData::ErrBits        err1,
                                    WibFrameV0_3::ColdData::ErrBits        err2,
                                    WibFrameV0_3::ColdData::Rsvd0          rsvd,
                                    WibFrameV0_3::ColdData::Checksum     csA_lo,
                                    WibFrameV0_3::ColdData::Checksum     csB_lo,
                                    WibFrameV0_3::ColdData::Checksum     csA_hi,
                                    WibFrameV0_3::ColdData::Checksum     csB_hi,
                                    WibFrameV0_3::ColdData::ConvertCount  count)
{
    DataWord w = s0 (count, csB_hi, csA_hi, csB_lo, csA_lo, rsvd, err2, err1);
    return   w;
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
 inline WibFrameV0_3::ColdData::Hdrs 
        WibFrameV0_3::ColdData::hdrs (WibFrameV0_3::ColdData::Hdr const hdr[8])
 {
    Hdrs hdrs = (hdr[7], hdr[6], hdr[5], hdr[4],
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
 inline WibFrameV0_3::DataWord 
        WibFrameV0_3::ColdData::s1 (WibFrameV0_3::ColdData::ErrReg errReg,
                                    WibFrameV0_3::ColdData::Rsvd1   rsvd1,
                                    WibFrameV0_3::ColdData::Hdrs     hdrs)
{
    DataWord w = (hdrs, rsvd1, errReg);
    return   w;
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
inline WibFrameV0_3::DataWord
       WibFrameV0_3::ColdData::s1 (WibFrameV0_3::ColdData::ErrReg errReg,
                                   WibFrameV0_3::ColdData::Hdrs     hdrs)
{
   DataWord w = (hdrs, 0, errReg);
   return   w;
}
/* ---------------------------------------------------------------------- */

#endif  /* WIBFRAMEV0_3_COMPOSE */

#endif
   
