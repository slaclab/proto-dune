
/* ---------------------------------------------------------------------- */
/* Local methods used by the update read class method                     */
/* ---------------------------------------------------------------------- */
static void update_read (MonitorRead        &lcl,
                         ModuleConfig const &cfg,
                         ReadStatus       status);
/* ---------------------------------------------------------------------- */



 void MonitorRead::update (ModuleConfig const  cfg,
                           MonitorRead        &gbl,
                           ReadStatus       status)
 {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE
    update_read (*this, cfg, status);
    gbl = *this;
    return;
}

 void MonitorRead::update (ModuleConfig const cfg,
                       ReadStatus      status)
{
   #pragma HLS INLINE off
    update_read (*this, cfg, status);
    return;
}





/* ---------------------------------------------------------------------- */
/* Local methods used by the update read method                           */
/* ---------------------------------------------------------------------- */
static void update_frame    (MonitorRead    &lcl,
                             ReadStatus   status);

static void update_wib      (MonitorRead    &lcl,
                             ReadStatus   status);

static void update_colddata (MonitorRead    &lcl,
                             ReadStatus   status,
                             int            sOff,
                             int            cOff);

static void printReadResults (ap_uint<2>          status,
                              MonitorRead const &monitor);

/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Update the read portion of the status block
 *
 *  \param[in:out]    lcl The locally maintained copy of the read status
 *                        block
 * \param[in]         cfg The configuration block
 * \param[in]      status The read status flags.  This is used to increment
 *                        a counter corresponding to this status.
 *
\* ---------------------------------------------------------------------- */
static void update_read (MonitorRead        &lcl,
                         ModuleConfig const &cfg,
                         ReadStatus       status)
{
   #pragma HLS PIPELINE
   ///static MonitorRead lcl;


   //std::cout << "status(flush): " << status << " user:" << in.user << std::endl;
#if 0
   static bool First = true;
   if ( (cfg.init == 1)|| (cfg.init >= 0 && First))
   ///if (First)
   {
      printf ("Initializing lcl\n");
      First = false;
      lcl.init ();
   }
   else
#endif
   {
      // Always update the status mask and count the total number of frames seen
      lcl.summary.mask     = status;
      lcl.summary.nframes += 1;

      if (status.test (ReadStatus::Offset::ErrSofM))
      {
         // Count frames missing SOF
         lcl.count(MonitorRead::FrameCounter::ErrSofM);
      }
      else
      {
         // SOF   was set
         update_frame (lcl, status);

         // A good frame is one with SOF and EOF set only on the proper words and no Eof Err
         bool goodFrame = ReadStatus::isGoodFrame (status);
         if  (goodFrame)
         {
            // Only count if both SOF and EOF were properly seen
            lcl.count (ReadStatus::state (status));

            // Only increment the counters associated with checking the integrity
            // of the frame contents if this is not a FLUSH frame.
            if (!status.test (static_cast<unsigned>(ReadStatus::Offset::Flush)))
            {
               // Wib Header word
               update_wib (lcl, status);

               // ColdData link 0 and 1
               update_colddata (lcl,status, 0, 0);
               update_colddata (lcl,status, 8, 8);
            }
         }
      }
   }

   printReadResults (status, lcl);

   return;
}
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Update the counters that monitor the status of the reading
 *           of the frame. By contrast, these are counters that have nothing
 *           to do with the data contents, only the integrity of the data
 *           transfer.
 *
 *  \param[in:out]    mon  The monitor class to update
 *  \param[in]     status  The status mask
 *
\* ----------------------------------------------------------------------- */
static void update_frame (MonitorRead &mon, ReadStatus status)
{
   #pragma HLS INLINE

   // Unexpected SOF seen?
   mon.count (status.test (ReadStatus:: Offset::      ErrSofU),
                           MonitorRead::FrameCounter::ErrSofU);

   // Unexpected EOF seen?
   mon.count (status.test (ReadStatus:: Offset::      ErrEofU),
                           MonitorRead::FrameCounter::ErrEofU);

   // EOF missing on last word?
   mon.count (status.test (ReadStatus:: Offset::      ErrEofM),
                           MonitorRead::FrameCounter::ErrEofM);

   // EOF err set in the frame?
   mon.count (status.test (ReadStatus:: Offset::      ErrEofE),
                           MonitorRead::FrameCounter::ErrEofE);

   return;
}
/* ----------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Update the counters that monitor the status of the reading
 *           of the cold data by the WIB
 *
 *  \param[in:out]    mon  The monitor class to update
 *  \param[in]     status  The status mask
 *
\* ----------------------------------------------------------------------- */
static void update_wib (MonitorRead &mon, ReadStatus status)
{
   #pragma HLS INLINE

   // Wib Header word 0
   mon.count (status.test (ReadStatus:: Offset::    ErrWibComma),
                           MonitorRead::WibCounter::ErrWibComma);

   mon.count (status.test (ReadStatus:: Offset::    ErrWibId),
                           MonitorRead::WibCounter::ErrWibId);

   mon.count (status.test (ReadStatus:: Offset::    ErrWibVersion),
                           MonitorRead::WibCounter::ErrWibVersion);

   mon.count (status.test (ReadStatus:: Offset::    ErrWibRsvd),
                           MonitorRead::WibCounter::ErrWibRsvd);

   mon.count (status.test (ReadStatus:: Offset::    ErrWibErrors),
                           MonitorRead::WibCounter::ErrWibErrors);


   // Wib Header word 1
   mon.count (status.test (ReadStatus:: Offset::    ErrWibTimestamp),
                           MonitorRead::WibCounter::ErrWibTimestamp);

   return;
}
/* ----------------------------------------------------------------------- */





/* ----------------------------------------------------------------------- *//*!
 *
 *   \brief  Update the counters that monitor the status of the reading
 *           of the front end electronics
 *
 *  \param[in:out]    mon  The monitor class to update
 *  \param[in]     status  The status mask
 *
\* ----------------------------------------------------------------------- */
static void  update_colddata (MonitorRead    &mon,
                              ReadStatus   status,
                              int            soff,
                              int            coff)
{
   //#pragma HLS PIPELINE
   #pragma HLS INLINE

   // ------------------------------------------------------------
   // ColdData Header Word 0
   //
   mon.count (status.test (ReadStatus:: Offset::    ErrCd0StrErr1 + soff),
                           MonitorRead::WibCounter::ErrCd0StrErr1,  coff);

   mon.count (status.test (ReadStatus:: Offset::    ErrCd0StrErr2 + soff),
                           MonitorRead::WibCounter::ErrCd0StrErr2,  coff);

   mon.count (status.test (ReadStatus:: Offset::    ErrCd0Rsvd0   + soff),
                           MonitorRead::WibCounter::ErrCd0Rsvd0,    coff);

   mon.count (status.test (ReadStatus:: Offset::    ErrCd0ChkSum  + soff),
                           MonitorRead::WibCounter::ErrCd0ChkSum,   coff);

   mon.count (status.test (ReadStatus:: Offset::    ErrCd0CvtCnt  + soff),
                           MonitorRead::WibCounter::ErrCd0CvtCnt,   coff);
   //
   // ------------------------------------------------------------


   // ------------------------------------------------------------
   // ColdData Header Word 1
   //
   mon.count (status.test (ReadStatus:: Offset::    ErrCd0ErrReg + soff),
                           MonitorRead::WibCounter::ErrCd0ErrReg,  coff);

   mon.count (status.test (ReadStatus:: Offset::    ErrCd0Rsvd1  + soff),
                           MonitorRead::WibCounter::ErrCd0Rsvd1,   coff);

   mon.count (status.test (ReadStatus:: Offset::    ErrCd0Hdrs   + soff),
                           MonitorRead::WibCounter::ErrCd0Hdrs,    coff);
   //
   // ------------------------------------------------------------

   return;
}
/* ----------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Print the status results of the readFrame method
 *
 *  \param[in]  status  The 2-bit readFrame status word
 *  \param[in] monitor  The read monitor record
\* ---------------------------------------------------------------------- */
static void printReadResults (ap_uint<2> status, MonitorRead const &monitor)
{
   // If the readStatus was not successsful
   if (status)
   {
      std::cout << "ReadStatus: " << status << std::endl
                << "Types:      " << monitor[MonitorRead::StateCounter::Normal]
                <<            ' ' << monitor[MonitorRead::StateCounter::RunDisabled]
                <<            ' ' << monitor[MonitorRead::StateCounter::Flush]
                <<            ' ' << monitor[MonitorRead::StateCounter::DisFlush]
                <<       "Err SofM     " << monitor[MonitorRead::FrameCounter::ErrSofM]
                <<       "Err SofU     " << monitor[MonitorRead::FrameCounter::ErrSofU]
                <<       "Err EofM     " << monitor[MonitorRead::FrameCounter::ErrEofM]
                <<       "Err EofU     " << monitor[MonitorRead::FrameCounter::ErrEofU]
                <<       "Err EofE     " << monitor[MonitorRead::FrameCounter::ErrEofE]
                <<   "Err_WibComma     " << monitor[MonitorRead::WibCounter  ::ErrWibComma]
                <<   "Err WibTimestamp " << monitor[MonitorRead::WibCounter  ::ErrWibTimestamp]
                << std::endl;
  }

  return;
}
/* ----------------------------------------------------------------------- */

