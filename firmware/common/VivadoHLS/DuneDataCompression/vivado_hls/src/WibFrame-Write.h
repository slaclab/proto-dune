
/* ----------------------------------------------------------------------- *//*!
 *
 *  \struct WriteContext
 *  \brief  Structure to bind the write context with the output stream
 *
\* ----------------------------------------------------------------------- */
class WibFrameWrite
{
public:
   WibFrameWrite () :
      m_odx     (0),
      m_fdx     (0)
   {
      return;
   }

   void reset ()
   {
      m_odx = 0;
      m_fdx = 0;
      return;
   }
   
   void write (AxisOut     &axis,
               ReadFrame  &frame,
               MonitorWrite &gbl,
               MonitorWrite &lcl);

public:
    int  m_odx;  /*!< The output word index (count of output words)        */
    int  m_fdx;  /*!< Frame index (count of frames)                        */
};
/* ----------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Writes the frame's data to the output packet. The packet will
 *           be flushed if
 *              -# Full
 *              -# The run was disabled with output pending
 *              -# A flush was requested
 *
 *   \param[   out]       axis  The output AXI stream
 *   \param[in:out]        ctx  The output context
 *   \param[in    ]      frame  The frame's data + status
 *   \param[in:out]        gbl  The global write monitor
 *   \param[in:out]        lcl  The local  write nonitor
 *
\* ---------------------------------------------------------------------- */
void write_frame (AxisOut           &axis,
                  ReadFrame        &frame,
                  MonitorWrite       &gbl,
                  MonitorWrite       &lcl)
{
   static WibFrameWrite ctx;
   static bool First = true;
   if (First)
   {
      lcl.nbytes    = 0;
      lcl.ndropped  = 0;
      lcl.npromoted = 0;
      lcl.npackets  = 0;
      First         = false;
   }

   static ReadStatus           summary;
   ReadStatus status = frame.m_wstatus;

   // --------------------------------------------------------------------
   // If the neither the run is disabled nor this frame should be flushed
   // the frame is considered writeable
   // --------------------------------------------------------------------
   bool writeIt = !(status.test (static_cast<unsigned>(ReadStatus::Offset::RunDisable))
                |   status.test (static_cast<unsigned>(ReadStatus::Offset::Flush)));

   if (writeIt) { lcl.npromoted += 1;  ctx.m_fdx += 1;    }
   else         { lcl.ndropped  += 1;  gbl = lcl; return; }


   // -------------------------------------------------
   // OUTPUT HEADER
   // -------------
   // Add output header to the output frame iff this is
   //   1. the first output word of this packet ..and..
   //   2. the processFlag is true.
   // --------------------------------------------------
   int axisFirst;
   if (ctx.m_odx == 0)
   {
       //prologue (axis, ctx.m_odx, writeIt,0);
       if (writeIt) summary = status;
       else         summary =      0;

       axisFirst  = 1 << (int)AxisUserFirst::Sof;
   }
   else
   {
      if (writeIt) summary |= status;
      axisFirst  = 0;
   }


   // -------------------------------------------------------------------
   // OUTPUT BODY
   // -----------
   // While the data is not copied to the output packet is procesFlag is
   // false, the copy loop always runs to ensure that the data stream is
   // drained.
   // -------------------------------------------------------------------
   COPYFRAME_LOOP:
   for (int idx = 0; idx < sizeof (WibFrame)/sizeof (uint64_t); idx++)
   {
      #pragma HLS PIPELINE
      uint64_t          d;

      if (axisFirst)
      {
         printf ("Setting first on %x.%x", idx, ctx.m_odx);
      }
      d = frame.m_dat.d[idx];
      commit (axis, ctx.m_odx, writeIt, d, axisFirst, 0);
      axisFirst = 0;
   }



   // --------------------------------------------
   // OUTPUT TRAILER
   // --------------
   // Add the trailer and flush packet if both
   //   1) Have some data to flush
   //   2) Instructed that this is the last packet
   // --------------------------------------------
   bool output;
   if (writeIt && ctx.m_odx && ((ctx.m_fdx & 0x3ff)  == 0))
   {
      printf ("Outputting packet frame.status = %8.8x ctx.m_fdx = %8.8x\n",
               status.to_uint(), lcl.npromoted);
      epilogue (axis, ctx.m_odx, summary);

      lcl.nbytes   += ctx.m_odx * 8;
      lcl.npackets += 1;
      ctx.m_odx     = 0;
      ctx.m_fdx     = 0;
   }

   gbl = lcl;

   return;
}
/* ---------------------------------------------------------------------- */
