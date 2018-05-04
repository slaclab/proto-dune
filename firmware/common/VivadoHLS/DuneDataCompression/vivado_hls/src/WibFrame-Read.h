#ifndef _WIBFRAME_READ_H_
#define _WIBFRAME_READ_H_

#include "ap_int.h"

/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Typedef for the read frame status
 *
 *  \par
 *   This is a bit mask of conditions that would invalidate the frame
 *   data.  Currently there are
 *      - Bit 0: RunDisable
 *      - Bit 1: Flush
 *      - Bit 2: Start of frame marker missing
 *      - Bit 3: End   of frame marker missing
 *      - Bit 4: K28_5 character not seen
 *      - Bit 5: WIB sequence error
 *
 *   If either of these 4 bits  are set
 *      -# The data for this frame is consider invalid
 *       # Any in-progress packet should be flushed.
\* ----------------------------------------------------------------------- */
typedef ap_uint<32> ReadStatus_t;

class ReadStatus : public ReadStatus_t
{
public:
   explicit ReadStatus ()
   {
      #pragma HLS INLINE
      return;
   }

   explicit ReadStatus (ReadStatus_t x) :
      ReadStatus_t (x)
   {
      #pragma HLS INLINE
      return;
   }

   explicit ReadStatus (uint32_t x) :
        ReadStatus_t (x)
   {
      #pragma HLS INLINE
      return;
   }
   enum class WibOffset
   {
      ErrWibBeg        = 0x00,

      // Errors from the WIB header words
      ErrWibBeg0      = 0x00,
      ErrWibComma     = 0x00, /*!< Comma character does not match         */
      ErrWibVersion   = 0x01, /*!< Version number incorrect               */
      ErrWibId        = 0x02, /*!< WibId (Crate.Slot.Fiber) incorrect     */
      ErrWibRsvd      = 0x03, /*!< Reserved header bits are not 0         */
      ErrWibErrors    = 0x04, /*!< Wib Errors field is not 0              */
      ErrWibEnd0      = 0x04,
      ErrWibCnt0      = ErrWibEnd0 - ErrWibBeg0 + 1,

      ErrWibBeg1      = 0x05,
      ErrWibTimestamp = 0x05, /*!< Unused bit                             */
      ErrWibUnused6   = 0x06, /*!< Unused bit                             */
      ErrWibUnused7   = 0x07, /*!< Unused bit                             */
      ErrWibEnd1      = 0x07,
      ErrWibCnt1      = ErrWibEnd1 - ErrWibBeg1 + 1,

      ErrWibEnd       = 0x07,
      ErrWibCnt       = ErrWibEnd  - ErrWibBeg  + 1
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  CdOffset
    *  \brief Maps out the bits of t
    *  he cold data link status bits
    *
    */
   enum class CdOffset
   {
      // Errors from the Colddata stream 0 header words
      ErrCdBeg0      = 0x00,
      ErrCdStrErr1   = 0x00,  /*!< Colddata link, stream err1 is not 0    */
      ErrCdStrErr2   = 0x01,  /*!< Colddata link, stream err2 is not 0    */
      ErrCdRsvd0     = 0x02,  /*!< Colddata link, reserved field is not 0 */
      ErrCdChkSum    = 0x03,  /*!< Colddata link, checksum incorrect      */
      ErrCdCvtCnt    = 0x04,  /*!< Colddata link, convert count mismatch  */
      ErrCdEnd0      = 0x04,
      ErrCdCnt0      = ErrCdEnd0 - ErrCdBeg0 + 1,

      ErrCdBeg1      = 0x05,
      ErrCdErrReg    = 0x05,  /*!< Colddata link, error register is not 0 */
      ErrCdRsvd1     = 0x06,  /*!< Colddata link, reserved field is not 0 */
      ErrCdHdrs      = 0x07,  /*!< Colddata link, error in hdr words      */
      ErrCdEnd1      = 0x07,
      ErrCdCnt1      = ErrCdEnd1 - ErrCdBeg1 + 1,
      ErrCdEnd       = ErrCdEnd1,

      ErrCdCnt       = ErrCdEnd1 - ErrCdBeg1 + 1

   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  StateOffset
    *  \brief Maps out the bits of the frame state status bits
    *
    */
   enum class StateOffset
   {
      StateBeg        = 0x00,
      RunDisable      = 0x01, /*!< Run is disable                         */
      Flush           = 0x02, /*!< Flush the packet data                  */
      StateEnd        = 0x02,
      StateCnt        = StateEnd - StateBeg + 1
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  FrameSOffet
    *  \brief Maps out the bits of the frame state status bits
    *
    */
   enum class FrameOffset
   {
      ErrFrameBeg     = 0x00,
      ErrSofM         = 0x01, /*!< Start of frame error, error            */
      ErrSofU         = 0x02, /*!< Start of frame error, unexpected       */
      ErrEofM         = 0x03, /*!< End   of frame error, missing          */
      ErrEofU         = 0x04, /*!< End   of frame error, unexpected       */
      ErrEofE         = 0x05, /*!< End   of frame error, error bit set    */
      ErrFrameEnd     = 0x05,
      ErrFrameCnt     = ErrFrameEnd - ErrFrameBeg + 1
   };
   /* ------------------------------------------------------------------- */

public:
   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum Offset
    *  \brief Maps out the bits in the status mask
    */
   enum Offset
   {
      // Errors from the WIB header words
      ErrWibBeg0      = 0x00,
      ErrWibComma     = 0x00, /*!< Comma character does not match         */
      ErrWibVersion   = 0x01, /*!< Version number incorrect               */
      ErrWibId        = 0x02, /*!< WibId (Crate.Slot.Fiber) incorrect     */
      ErrWibRsvd      = 0x03, /*!< Reserved header bits are not 0         */
      ErrWibErrors    = 0x04, /*!< Wib Errors field is not 0              */
      ErrWibEnd0      = 0x04,

      ErrWibBeg1      = 0x05,
      ErrWibTimestamp = 0x05, /*!< Unused bit                             */
      ErrWibUnused6   = 0x06, /*!< Unused bit                             */
      ErrWibUnused7   = 0x07, /*!< Unused bit                             */
      ErrWibEnd1      = 0x07,

      // Errors from the Colddata stream 0 header words
      ErrCd0Beg0      = 0x08,
      ErrCd0StrErr1   = 0x08,  /*!< Colddata link 0, stream err1 is not 0   */
      ErrCd0StrErr2   = 0x09,  /*!< Colddata link 0, stream err2 is not 0   */
      ErrCd0Rsvd0     = 0x0a,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd0ChkSum    = 0x0b,  /*!< Colddata link 0, checksum incorrect     */
      ErrCd0CvtCnt    = 0x0c,  /*!< Colddata link 0, convert count mismatch */
      ErrCd0End0      = 0x0c,

      ErrCd0Beg1      = 0x0d,
      ErrCd0ErrReg    = 0x0d,  /*!< Colddata link 0, error register is not 0*/
      ErrCd0Rsvd1     = 0x0e,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd0Hdrs      = 0x0f,  /*!< Colddata link 0, error in hdr words     */
      ErrCd0End1      = 0x0f,

      // Errors from the Colddata stream 1 header words
      ErrCd1Beg0      = 0x10,
      ErrCd1StrErr1   = 0x10,  /*!< Colddata link 0, stream err1 is not 0   */
      ErrCd1StrErr2   = 0x11,  /*!< Colddata link 0, stream err2 is not 0   */
      ErrCd1Rsvd0     = 0x12,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd1ChkSum    = 0x13,  /*!< Colddata link 0, checksum incorrect     */
      ErrCd1CvtCnt    = 0x14,  /*!< Colddata link 0, convert count mismatch */
      ErrCd1End0      = 0x14,

      ErrCd1Beg1      = 0x15,
      ErrCd1ErrReg    = 0x15,  /*!< Colddata link 0, error register is not 0*/
      ErrCd1Rsvd1     = 0x16,  /*!< Colddata link 0, reserved field is not 0*/
      ErrCd1Hdrs      = 0x17,  /*!< Colddata link 0, error in hdr words     */
      ErrCd1End1      = 0x17,

      // Frame state
      StateBeg        = 0x18,
      RunDisable      = 0x18, /*!< Run is disable                           */
      Flush           = 0x19, /*!< Flush the packet data                    */
      StateEnd        = 0x19,
      StateCnt        = StateEnd - StateBeg + 1,

      // Frame errors
      ErrFrameBeg     = 0x1a,
      ErrSofM         = 0x1a, /*!< Start of frame error, error              */
      ErrSofU         = 0x1b, /*!< Start of frame error, unexpected         */
      ErrEofM         = 0x1c, /*!< End   of frame error, missing            */
      ErrEofU         = 0x1d, /*!< End   of frame error, unexpected         */
      ErrEofE         = 0x1e, /*!< End   of frame error, error bit set      */
      ErrFrameEnd     = 0x1e
   };
   /* ---------------------------------------------------------------------- */

   ReadStatus &operator = (int x)
   {
      this->ReadStatus_t::operator =(x); ////const_cast<ReadStatus_t &>(x));
      return *this;
   }

   ReadStatus &operator = (ReadStatus_t x)
   {
      this->ReadStatus_t::operator =(x);
      return *this;
   }

   /* ---------------------------------------------------------------------- *//*!
    *
    *  \brief  Extract the state bits from the status word
    *  \return The extracted state bits
    *
    *  \param[in]  status The target status word.
   \*----------------------------------------------------------------------- */
   static ap_uint<StateCnt> state (ReadStatus_t status)
   {
      #pragma HLS INLINE
      return status (Offset::StateEnd, Offset::StateEnd);
   }
   /* ---------------------------------------------------------------------- */
   ap_uint<StateCnt> state () const
   {
      #pragma HLS INLINE
      return state (*this);
   }
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
     *
     *  \brief Tests whether the frame was correctly read
     *  \retval true, if it was correctly read
     *  \retval false, if not
     *
     *  \param[in] status The read status
     *                                                                        */
    /* ---------------------------------------------------------------------- */
    static bool isGoodFrame (ap_uint<32> status)
    {
       #pragma HLS INLINE
       int sb = status >> static_cast<unsigned>(Offset::ErrFrameBeg);
       return sb == 0;
    }
    /* ---------------------------------------------------------------------- */
    bool isGoodFrame () const
    {
       #pragma HLS INLINE
       return isGoodFrame (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- *//*!
     *
     *  \brief Tests whether this frame is flushes the packet
     *  \retval true,  if so
     *  \retval false, if not
     *
     *  \param[in] status The read status
     *
     *  If either the run is disabled nor the frame flush is set, the
     *  frame is considered the last frame in the packet.
     *                                                                        */
    /* ---------------------------------------------------------------------- */
    static bool flushesPacket (ReadStatus_t status)
    {
       #pragma HLS INLINE
       bool wf = status.test (static_cast<unsigned>(Offset::RunDisable))
              |  status.test (static_cast<unsigned>(Offset::Flush));
       return wf;
    }
    /* ---------------------------------------------------------------------- */
    bool flushesPacket () const
    {
        #pragma HLS INLINE
       return flushesPacket (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- */
    static bool isWibHdr0Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrWibEnd0),
                      static_cast<unsigned>(Offset::ErrWibBeg0));
    }
    /* ---------------------------------------------------------------------- */
    bool isWibHdr0Bad () const
    {
       #pragma HLS INLINE
       return isWibHdr0Bad (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- */
    static bool isWibHdr1Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrWibEnd1),
                      static_cast<unsigned>(Offset::ErrWibBeg1));
    }
    /* ---------------------------------------------------------------------- */
    bool isWibHdr1Bad () const
    {
       #pragma HLS INLINE
       return isWibHdr1Bad (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- */
    static bool isCd0Hdr0Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd0End0),
                      static_cast<unsigned>(Offset::ErrCd0Beg0));
    }
    /* ---------------------------------------------------------------------- */
    bool isCdHdr0Bad () const
    {
       #pragma HLS INLINE
       return isCd0Hdr0Bad (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- */
    static bool isCd0Hdr1Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd0End1),
                      static_cast<unsigned>(Offset::ErrCd0Beg1));
    }
    /* ---------------------------------------------------------------------- */
    bool isCd0Hdr1Bad () const
    {
       #pragma HLS INLINE
       return isCd0Hdr1Bad (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- */
    static bool isCd1Hdr0Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd1End0),
                      static_cast<unsigned>(Offset::ErrCd1Beg0));
    }
    /* ---------------------------------------------------------------------- */
    bool isCd1Hdr0Bad () const
    {
       #pragma HLS INLINE
       return isCd1Hdr0Bad (*this);
    }
    /* ---------------------------------------------------------------------- */


    /* ---------------------------------------------------------------------- */
    static bool isCd1Hdr1Bad (ReadStatus_t status)
    {
       #pragma HLS INLINE
       return status (static_cast<unsigned>(Offset::ErrCd1End1),
                      static_cast<unsigned>(Offset::ErrCd1Beg1));
    }
    /* ---------------------------------------------------------------------- */
    bool isCd1Hdr1Bad () const
    {
       #pragma HLS INLINE
       return isCd1Hdr1Bad (*this);
    }
    /* ---------------------------------------------------------------------- */
};
/* ----------------------------------------------------------------------- */




/* ----------------------------------------------------------------------- *//*!
 *
 *  \struct ReadFrame
 *  \brief  Structure to bind the read status with the frame's data
 *
\* ----------------------------------------------------------------------- */
class ReadFrame
{
public:
   ReadFrame () :
      m_status (0)
  {
      return;
  }

   void read (AxisIn &axis);

public:
    typedef uint64_t Data;

    struct ReadData
    {
       Data d[MODULE_K_MAXSIZE_IB];
    };


   ReadData                  m_dat;  /*!< The frame's data                 */
   ReadStatus             m_status;  /*!< The frame's read status          */
   ReadStatus            m_wstatus;  /*!< Copy for the write routine       */

};
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 * \struct ReadContext
 * \brief  Structure to contain the read context.
 *
 *  The context are variables that maintained across WIB frames
 */
class ReadContext
{
public:
   ReadContext () :
      first (true),
      begPkt(true),
      mode     (0),
      wibId    (0),
      cvtCnt0  (0),
      cvtCnt1  (0)
  {
      return;
  }

public:
   bool           first; /*!< First time flag                                */
   bool          begPkt; /*!< Beginning of a packet                          */
   ap_uint<2>      mode; /*!< The processing mode                            */
   ap_uint<11>    wibId; /*!< The crate, slot, fiber number                  */
   AxisWord_t   hdrs[6]; /*!< The 6 header words from the previous frame     */
   uint16_t     cvtCnt0; /*!< The previous convert count from stream 0       */
   uint16_t     cvtCnt1; /*!< The previous convert count from stream 1       */
};
/* ------------------------------------------------------------------------- */

static        void read_frame (ReadFrame      *frame,
                               AxisIn          &axis);

static void         read_wib0 (ReadStatus    &status,
                               ReadFrame::Data    *d,
                               AxisIn          &axis,
                               AxisWord_t      *hdrs,
                               bool           &first,
                               ap_uint<11>    &wibId);

static void          read_wib1 (ReadStatus   &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                AxisWord_t     *hdrs);

static void      read_colddata (ReadStatus   &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                AxisWord_t     *hdrs,
                                uint16_t     &cvtCnt,
                                int              off);

static void read_colddata_hdrs (ReadStatus   &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                AxisWord_t     *hdrs,
                                uint16_t     &cvtCnt,
                                int              off);

static void read_colddata_data (ReadStatus   &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                bool       checkLast);


static void         eval_wib0  (ReadStatus   &status, 
                                uint64_t         hdr,
                                uint16_t       wibId);


static void         eval_wib1 (ReadStatus    &status,
                               uint64_t          hdr,
                               uint64_t          prv);

static void    eval_colddata0 (ReadStatus    &status,
                               uint64_t          hdr,
                               uint16_t    gotCvtCnt,
                               uint16_t    expCvtCnt,
                               int               off);

static void    eval_colddata1 (ReadStatus    &status,
                               uint64_t          hdr,
                               uint64_t          prv,
                               int               off);


static void            update (ReadStatus          &status,
                               bool                    val,
                               enum ReadStatus::Offset bit);

static void           update (ReadStatus           &status,
                              bool                     val,
                              enum ReadStatus::Offset  bit,
                              int                      off);


static void print_read_frame (ReadFrame const *frame);




inline void ReadFrame::read (AxisIn &axis)
{
    #pragma HLS INLINE off
    read_frame (this, axis);
}


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Read and buffer the Wib Frame data
 *
 *  \param[   out]  frame  A copy of the WIB data + status
 *  \param[   out] stream  The output stream
 *  \param[    in]   axis  The input WIB data
 *
 *
 *  \par
 *   The data is always copied but is valid iff the run enable bit in
 *   the user field of the first word is set and the flush flag in
 *   the user field of the last word is not set
 *
\* ---------------------------------------------------------------------- */
static inline void read_frame (ReadFrame     *frame,
                               AxisIn         &axis)
{
   #pragma HLS INLINE

   static ReadContext            ctx;
   ReadStatus              status(0);


   /////#pragma HLS STREAM variable=ctx off

   // Wib Header Words 0 & 1
   read_wib0     (status, frame->m_dat.d, axis, ctx.hdrs, ctx.first, ctx.wibId);
   read_wib1     (status, frame->m_dat.d, axis, ctx.hdrs);

   // Colddata links 0 & 1
   read_colddata (status, frame->m_dat.d +  2, axis, ctx.hdrs + 2, ctx.cvtCnt0, 0);
   read_colddata (status, frame->m_dat.d + 16, axis, ctx.hdrs + 4, ctx.cvtCnt1, 8);

   // Return the frame status
   frame->m_status  = status;
   frame->m_wstatus = status;

   print_read_frame (frame);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads and evaluates the first WIB header word
 *
 *  \param[in:out] status The read status word to be updated by an abnormal
 *                        conditions
 *  \param[out]         d Array to capture the header word
 *  \param[in]       axis The AXI stream to read the data from
 *  \param[in:out]   hdrs On input, this is the expected header value based
 *                        on the previous set. THis value is updated
 *                        based on the read header word
 * \param[in:out]   first A first time flag used to capture the WIB identity
 * \param[in:out]   wibId The WIB identity.  This is set on the first frame
 *                        and used as comparison value from then on.
 *
\* ---------------------------------------------------------------------- */
static void read_wib0 (ReadStatus   &status,
                       ReadFrame::Data   *d,
                       AxisIn         &axis,
                       AxisWord_t     *hdrs,
                       bool          &first,
                       ap_uint<11>   &wibId)
{
   #pragma HLS INLINE
   ////#pragma HLS PIPELINE

   AxisIn_t in = axis.read ();

   // Check the state of Start and End Of Frame bits
   bool    sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   bool    eof = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));

   // Check
   //  1) Start Of Frame is not missing and
   //  2) End   Of Frame is not present
   if (!sof) status.set (ReadStatus::Offset::ErrSofM);
   if ( eof) status.set (ReadStatus::Offset::ErrEofU);


   // Check the run enable bit
   // Note: The flush bit is only present on the last word in the frame
   bool  runEnable = in.user.test (static_cast<uint8_t>(AxisUserFirst::RunEnable));
   if  (!runEnable) status.set (ReadStatus::Offset::RunDisable);

   // !!! KLUDGE !!!
   // Seed the Wib Id with the first scene
   // This is not a great idea
   if (first)
   {
      first   = false;
      wibId   = WibFrame::id (in.data);
   }

   //  Check that all the fields are as expected
   eval_wib0 (status, in.data, wibId);

   // Capture the header and use as the seed for the next frame
   hdrs[0]   = in.data;
   d   [0]   = in.data;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads and evaluates the second WIB header word, this is
 *          the timestamp.
 *
 *  \param[in:out] status The read status word to be updated by an abnormal
 *                        conditions
 *  \param[out]         d Array to capture the header word
 *  \param[in]       axis The AXI stream to read the data from
 *  \param[in:out]   hdrs On input, this is the expected header value based
 *                        on the previous set. THis value is updated
 *                        based on the read header word
 *
\* ---------------------------------------------------------------------- */
static void read_wib1 (ReadStatus   &status,
                       ReadFrame::Data   *d,
                       AxisIn         &axis,
                       AxisWord_t     *hdrs)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   // This is the timestamp
   AxisIn_t in = axis.read ();

   // Check the state of Start and End Of Frame bits
   bool    sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   bool    eof = in.last.test (static_cast<uint8_t>(AxisLast     ::Eof));


   // Should not see the SOF or EOF marker
   if (sof) status.set (ReadStatus::Offset::ErrSofU);
   if (eof) status.set (ReadStatus::Offset::ErrEofU);


   //  Check that all the fields are as expected
   eval_wib1 (status, in.data, hdrs[1]);

   // Capture the header and update for use as the seed for the next frame
   hdrs[1] = in.data + 25;
   d   [1] = in.data;
   //d.write_nb (in.data);

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
static void read_colddata_hdrs (ReadStatus   &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                AxisWord_t     *hdrs,
                                uint16_t  &expCvtCnt,
                                int              off)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE II=2

   // ---------------------------------------------------------------------
   // Colddata link, word 0
   //
   AxisIn_t in = axis.read ();


   // Check the state of Start and End Of Frame bits
   bool    sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   bool    eof = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));


   // Should not see the SOF or EOF marker
   if (sof) status.set (ReadStatus::Offset::ErrSofU);
   if (eof) status.set (ReadStatus::Offset::ErrEofU);


   // Extract the convert count,
   // Check that all the fields are as expected
   // Update the convert count to the new value
   uint16_t gotCvtCnt = WibFrame::ColdData::cvtCnt (in.data);
   eval_colddata0 (status, in.data, gotCvtCnt, expCvtCnt, off);
   expCvtCnt  = gotCvtCnt + 1;


   // Capture the header and update for use as the seed for the next frame
   hdrs[0] = in.data;
   d   [0] = in.data;
   //
   // ---------------------------------------------------------------------



   // ---------------------------------------------------------------------
   // Colddata link, word 1
   //
   in   = axis.read ();


   // Check the state of Start and End Of Frame bits
   sof  = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
   eof  = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));


   // Should not see the SOF or EOF marker
   if (sof) status.set (ReadStatus::Offset::ErrSofU);
   if (eof) status.set (ReadStatus::Offset::ErrEofU);


   // Check that all the fields are as expected
   eval_colddata1 (status, in.data, hdrs[1], off);


   // Capture the header and update for use as the seed for the next frame
   hdrs[1] = in.data;
   d   [1] = in.data;
   // ---------------------------------------------------------------------
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void read_colddata_data (ReadStatus   &status,
                                ReadFrame::Data   *d,
                                AxisIn         &axis,
                                bool       checkLast)
{
    #pragma HLS INLINE

    AxisIn_t in;

   // Capture the data
   READCOLDDATA_LOOP:
   for (int idx = 0; idx < 12; idx++)
   {
       //#pragma HLS PIPELINE

      // Get the input word, always copy the data
      in     = axis.read ();
      d[idx] = in.data;


      // Check the state of Start and End Of Frame bits
      bool sof = in.user.test (static_cast<uint8_t>(AxisUserFirst::Sof));
      bool eof = in.last.test (static_cast<uint8_t>(AxisLast::     Eof));


      // Should not see the SOF marker
      if (sof) status.set (ReadStatus::Offset::ErrSofU);


      // Check if this is not the last word
      if (idx != 11 || !checkLast)
      {
         // Not the last word, should not see EOF marker
         if (eof) status.set (ReadStatus::Offset::ErrEofU);
      }
      else
      {
         // Handle the very last word
         // Check the presence of flush and eof on the last input word
         bool flush  = in.user.test (static_cast<unsigned>(AxisUserLast::Flush));
         bool eoferr = in.user.test (static_cast<unsigned>(AxisUserLast::EofErr));

         if (flush ) status.set (ReadStatus::Offset::Flush);
         if (!eof  ) status.set (ReadStatus::Offset::ErrEofM);
         if (eoferr) status.set (ReadStatus::Offset::ErrEofE);
         break;
      }
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Reads and evaluates the two WIB cold data header words,
 *
 *  \param[in:out] status The read status word to be updated by an abnormal
 *                        conditions
 *  \param[out]         d Array to capture the header word
 *  \param[in]       axis The AXI stream to read the data from
 *  \param[in:out]   hdrs On input, this is the expected header value based
 *                        on the previous set. This value is updated
 *                        based on the read header word.
 *  \param[in:out] cvtCnt On input, the expect convert count, on output,
 *                        the updated convert count
 *  \param[in]        off  This is the bit offset to the base of the
 *                         appropriate cold data link; basically selects
 *                         the target cold data link.
 *
\* ---------------------------------------------------------------------- */
static void read_colddata (ReadStatus   &status,
                           ReadFrame::Data   *d,
                           AxisIn         &axis,
                           AxisWord_t     *hdrs,
                           uint16_t     &cvtCnt,
                           int              off)
{
   #pragma HLS INLINE

   // ---------------------------------------------------------------------
   // For some reason this routine must be INLINE'd
   // Failure to do so results in very odd results whereby the input
   // stream seems to repeatedly just return the very first word in it.
   // And by very first, it is the very first word, not just of the
   // frame, but the very first word ever placed in it.
   // I tested this my putting an incrementing pattern in the first
   // word, and it was always the same word, never the incremented word.
   //
   // This took more than 2 days to track down.  It would pass the
   // the C-simuluation, but not the co-simulation.
   // -------------------------------------------------------------------
   read_colddata_hdrs (status, d,     axis, hdrs, cvtCnt, off);
   read_colddata_data (status, d + 2, axis, off == 8);
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the first WIB header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The first WIB header word
 *  \param[in]      wibId  The WIB identity, \e i.e. the Crate.Slot.Fiber
 *                         number.  This is compared with the identity in
 *                         the \a hdr.
 *
\* ---------------------------------------------------------------------- */
static void eval_wib0 (ReadStatus &status, uint64_t hdr, uint16_t wibId)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   // Check that the first word is as expected
   //    The Wib Comma    field must be WibFrame:K28_5
   //    The Wib Version  field must be WibFrame::VersionNumber
   //    The Wib Id       field must be wibId
   //    The Wib Reserved field must be 0
   //    The Wib Error field must be 0
   bool wibErrCommaChar = WibFrame::commaChar(hdr) != WibFrame::K28_5;
   bool wibErrVersion   = WibFrame::version  (hdr) != WibFrame::VersionNumber;
   bool wibErrId        = WibFrame::id       (hdr) != wibId;
   bool wibErrRsvd      = WibFrame::rsvd     (hdr) != 0;
   bool wibErrErrors    = WibFrame::wibErrors(hdr) != 0;

   update (status, wibErrCommaChar, ReadStatus::Offset::ErrWibComma);
   update (status, wibErrVersion,   ReadStatus::Offset::ErrWibVersion);
   update (status, wibErrId,        ReadStatus::Offset::ErrWibId);
   update (status, wibErrRsvd,      ReadStatus::Offset::ErrWibRsvd);
   update (status, wibErrErrors,    ReadStatus::Offset::ErrWibErrors);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the second WIB header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The second WIB header word
 *  \param[in]        exp  The expected header word value
 *
 *
 *   Except for one mode 1, the header contains the timestamp.  The
 *   expected value, \a exp, has had the timestamp updated by the
 *   number of ticks between timesamples.  With this update, this it
 *   must match \a hdr.
 *
\* ---------------------------------------------------------------------- */
static void eval_wib1 (ReadStatus &status, uint64_t hdr, uint64_t prv)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   bool wibErrTimestamp = hdr != prv;
   update (status, wibErrTimestamp, ReadStatus::Offset::ErrWibTimestamp);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the first cold data header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The first WIB cold data link header word
 *  \param[in]  gotCvtCnt  The convert count that was previously extracted
 *                         from the header
 *  \param[in]  expCvtCnt  The expected convert count. This is based on
 *                         incrementing the previous convert count by 1.
 *  \param[in]        off  This is the bit offset to the base of the
 *                         appropriate cold data link; basically selects
 *                         the target cold data link.
 *
\* ---------------------------------------------------------------------- */
static void eval_colddata0 (ReadStatus   &status,
                            uint64_t         hdr,
                            uint16_t   gotCvtCnt,
                            uint16_t   expCvtCnt,
                            int              off)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   // Check that the word is as expected
   //       The Stream Err1 field must be 0
   //       The Stream Err2 field must be 0
   //       The Reserved    field must be 0
   //       The Convert Count field must be expect convert count
   //
   // Note: This word also contains the checksums. Because the data that
   //       went into forming the check sums is no longer present, this
   //       field cannot be checked.
   bool cdErrStreamErr1 = WibFrame::ColdData::streamErr1 (hdr) != 0;
   bool cdErrStreamErr2 = WibFrame::ColdData::streamErr2 (hdr) != 0;
   bool cdErrRsvd0      = WibFrame::ColdData::rsvd0      (hdr) != 0;
   bool cdErrCvtCnt     = gotCvtCnt                            != expCvtCnt;

   update (status, cdErrStreamErr1, ReadStatus::Offset::ErrCd0StrErr1, off);
   update (status, cdErrStreamErr2, ReadStatus::Offset::ErrCd0StrErr2, off);
   update (status, cdErrRsvd0,      ReadStatus::Offset::ErrCd0Rsvd0,   off);
   update (status, cdErrCvtCnt,     ReadStatus::Offset::ErrCd0CvtCnt,  off);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Evaluates any abnormal values in the second cold data header word
 *          and sets an appropriate flag bit in the read \a status.
 *
 *  \param[in:out] status  The read status bit mask
 *  \param[in]        hdr  The second WIB cold data link header word
 *  \param[in]        prv  The previous header word.
 *  \param[in]        off  This is the bit offset to the base of the
 *                         appropriate cold data link; basically selects
 *                         the target cold data link.
 *
\* ---------------------------------------------------------------------- */
static void eval_colddata1 (ReadStatus &status, uint64_t hdr, uint64_t prv, int off)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   bool cdErrErrReg = WibFrame::ColdData::errReg (hdr) != 0;
   bool cdErrHdrs   = WibFrame::ColdData::hdrs   (hdr) != WibFrame::ColdData::hdrs (prv);
   bool cdErrRsvd1  = WibFrame::ColdData::rsvd1  (hdr) != 0;

   update (status, cdErrErrReg, ReadStatus::Offset::ErrCd0ErrReg, off);
   update (status, cdErrRsvd1,  ReadStatus::Offset::ErrCd0Rsvd1,  off);
   update (status, cdErrHdrs,   ReadStatus::Offset::ErrCd0Hdrs,   off);
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief Convenience method to launder a ReadFrame::Offset to an
 *          unsigned int for use in the bit set method.
 *
 *   \param[in:out] status  The read status bit mask
 *   \param[in]        val  Flag to set the bit to a 0 or a 1
 *   \param[in]        bit  The bit number to modify
 *
\* ---------------------------------------------------------------------- */
static void update (ReadStatus           &status,
                    bool                     val,
                    enum ReadStatus::Offset  bit)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   status.set (static_cast<unsigned>(bit), val);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Convenience method to clear of set the appropriate state bit.
 *
 *   \param[in:out] status  The read status bit mask
 *   \param[in]        val  Flag to set the bit to a 0 or a 1
 *   \param[in]        bit  The beginning state bitb
 *   \param[in]        off  The offset (0-3) to the appropriate state bit.
 *
 *    The four states are
 *       -0.  Normal, run in progress
 *        1.  Run disabled
 *        2.  Flush, the frame should be read and disposed of (not written
 *        3.  Disabled and Flush, the run is disabled and the frame should
 *            be flushed.
 *
\* ---------------------------------------------------------------------- */
static void update (ReadStatus           &status,
                    bool                     val,
                    enum ReadStatus::Offset  bit,
                    int                      off)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   status |= val << static_cast<unsigned>(bit) + off;

   /////  This was tried, but the synthesizer issues a warning that this
   ////// construct may result in non-optimal results.
   /////  status.set (static_cast<unsigned>(bit) + off, val);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define READ_FRAME_PRINT 1
#endif
/* ---------------------------------------------------------------------- */

#undef READ_FRAME_PRINT

/* ---------------------------------------------------------------------- */
#if READ_FRAME_PRINT
/* ---------------------------------------------------------------------- */
static void print_read_frame (ReadFrame const *frame)
 {
   for (int i = 0; i < 30; i++)
   {
      std::cout << "Frame.d[" << std::setw(5) << i << "] = " << std::setw(16) << std::hex << frame->d[i] << std::endl;
   }
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
static void print_read_frame (ReadFrame const *frame)
{
   return;
}
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */

#endif
