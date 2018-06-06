#ifndef _AXISIO_TEST_H_
#define _AXISIO_TEST_H_

#include "Axis.h"
#include "WibFrame-Create.h"
#include <inttypes.h>

class MyStreamIn : public AxisIn
{
public:
   MyStreamIn () :                 AxisIn ("input stream") { return; }
   MyStreamIn (const char *name) : AxisIn (name)           { return; }
  ~MyStreamIn ()         { puts ("Deleting input stream"); return; }
};

class MyStreamOut : public AxisOut
{
public:
   MyStreamOut () :                 AxisIn ("output stream") { return; }
   MyStreamOut (const char *name) : AxisIn (name)            { return; }
  ~MyStreamOut ()           { puts ("Deleting output stream"); return; }
  
   static int check (MyStreamOut              &mAxis,
                     MyStreamOut              &sAxis,
                     int                   packetNum);
   void print ();
};


class Source
{
public:
   Source ();

   //void                    add_header (uint32_t      id,
   //                                    uint32_t version);

   void                   add_trailer (uint32_t version,
                                       uint32_t      id,
                                       uint32_t  status,
                                       uint32_t  nbytes);

   void                    fill_frame (uint64_t                      timestamp,
                                       uint16_t  const adcs[MODULE_K_NCHANNELS],
                                       int                           runEnable,
                                       int                               flush,
                                       int                              iframe);
   void              fill_dummy_frame (uint64_t timestamp,
                                       int          first,
                                       int          flush,
                                       int         iframe);
   void              fill_empty_frame (uint64_t timestamp,
                                       int      runEnable,
                                       int          flush,
                                       int         iframe);
   void         fill_runDisable_frame (uint64_t timestamp,
                                       int         iframe);
   void              fill_flush_frame (uint64_t timestamp,
                                       int         iframe);
   void fill_runDisableAndFlush_frame (uint64_t timestamp,
                                       int         iframe);
   
   void                        commit (uint64_t         w,
                                       int           user,
                                       int           last);
   
   static void                 commit (MyStreamIn  &stream,
                                       uint64_t          w,
                                       int            user,
                                       int            last);
   
   static void                 commit (MyStreamOut &stream,
                                       uint64_t          w,
                                       int            user,
                                       int            last);

public:
   MyStreamIn  m_src;  /*!< The stream used by the HLS module    */
   MyStreamOut m_chk;  /*!< The stream used to check the results */
};


Source::Source () :
      m_src ("In"),
      m_chk ("Check")
{
   return;
}

inline void Source::commit (MyStreamIn &stream, uint64_t w, int user, int last)
{
   AxisIn_t in;
   in.data = w;
   in.keep = 0xff;
   in.strb = 0;
   in.user = user;
   in.last = last;
   in.id   = 0;
   in.dest = 0;

   stream.write (in);
}


inline void Source::commit (MyStreamOut &stream, uint64_t w, int user, int last)
{
   AxisIn_t out;
   out.data = w;
   out.keep = 0xff;
   out.strb = 0;
   out.user = user;
   out.last = last;
   out.id   = 0;
   out.dest = 0;

   stream.write (out);
}



inline void Source::commit (uint64_t    w,
                            int      user,
                            int      last)
{
   AxisIn_t in;
   in.data = w;
   in.keep = 0xff;
   in.strb = 0;
   in.user = user;
   in.last = last;
   in.id   = 0;
   in.dest = 0;

   m_src.write (in);

   in.user = 0;
   m_chk.write (in);

   return;
}

#if 0
void Source::add_header (uint32_t id, uint32_t version)
{
   uint64_t header  = Header::header (id, version);

   commit (m_chk, header, (1 << (int)AxisUserFirst::Sof), 0);

   return;
}
#endif


// Add the trailer words to the check stream
void Source::add_trailer (uint32_t version, uint32_t id, uint32_t status, uint32_t nbytes)
{
   uint64_t statusId;

   // Two trailer words
   nbytes     +=  sizeof (statusId) + sizeof (Trailer);
   id         |=  nbytes;

    statusId   = status;
    statusId <<= 32;
    statusId  |= id;


   uint64_t tlr = Trailer::trailer (version);

   commit (m_chk, statusId, 0, 0);
   commit (m_chk, tlr,      0, (1 << (int)AxisLast::Eof));
   return;
}



void Source::fill_dummy_frame (uint64_t timestamp,
                               int          first,
                               int          flush,
                               int         iframe)
{
   int     iaxis = 0;
   int      last = (1 << (int)AxisLast::Eof);
   uint16_t data = 0xfff;


   uint16_t adcs[MODULE_K_NCHANNELS];

   for (int idx = 0; idx < MODULE_K_NCHANNELS; idx++)
   {
      adcs[idx] = 0xf00 | idx;
   }

   fill_frame (timestamp, adcs, 0, flush, iframe);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes a dummy WIB frame to the AXI input stream when doing
 *         a run disable.
 *                                                                        */
/* ---------------------------------------------------------------------- */
inline void Source::fill_runDisable_frame (uint64_t timestamp,
                                           int         iframe)
{
   int first = (1 << (int)AxisUserFirst::Sof);
   int flush = 0;
   fill_dummy_frame (timestamp, first, flush, iframe);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes a dummy WIB frame to the AXI input stream when doing
 *         a frame flush
 *                                                                                 */
/* ---------------------------------------------------------------------- */
inline void Source::fill_flush_frame (uint64_t timestamp,
                                      int         iframe)
{
   int first = (1 << (int)AxisUserFirst::Sof)
             | (1 << (int)AxisUserFirst::RunEnable);
   int flush = (1 << (int)AxisUserLast ::Flush);

   fill_dummy_frame (timestamp, first, flush, iframe);
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes a dummy WIB frame to the AXI input stream when doing
 *         a run disable and flush
 *                                                                        */
/* ---------------------------------------------------------------------- */
inline void Source::fill_runDisableAndFlush_frame (uint64_t timestamp,
                                                   int         iframe)
{
   int first = (1 << (int)AxisUserFirst::Sof);
   int flush = (1 << (int)AxisUserLast::Flush);

   fill_dummy_frame (timestamp, first, flush, iframe);
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes an empty WIB frame to the AXI input stream
 *
 *  \param[ in] timestamp   The timestamp of the frame
 *  \param[ in] runEnable   The run enable flag, if 0, data is pitched
 *  \param[ in]     flush   Flush flag
 *  \param[ in]    iframe   The frame number
 *                                                                        */
/* ---------------------------------------------------------------------- */
inline void Source::fill_empty_frame (uint64_t timestamp,
                                      int      runEnable,
                                      int          flush,
                                      int         iframe)

{
   int first = (1 << (int)AxisUserFirst::Sof)
             | (runEnable ? (1 << (int)AxisUserFirst::RunEnable) : 0);
   flush     = flush      ? (1 << (int)AxisUserLast ::Flush)     : 0;

   fill_dummy_frame (timestamp, first, flush, iframe);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Writes the data to the AXI input stream
 *
 *  \param[out]      axis   The target AXI stream
 *  \param[ount]    xaxis   Duplicate copy of the target AXI stream
 *                          used in checking the output
 *  \param[ in]      nova   The nova timestamp + status bits
 *  \param[ in]       adc   The ADC value
 *  \param[ in] runEnable   The run enable flag, if 0, data is pitched
 *  \param[ in]     flush   Flush flag
 *  \param[ in]    iframe   The frame number
 *                                                                        */
/* ---------------------------------------------------------------------- */
void Source::fill_frame (uint64_t                      timestamp,
                         uint16_t  const adcs[MODULE_K_NCHANNELS],
                         int                           runEnable,
                         int                               flush,
                         int                              iframe)
{
   static int Odx = 0;

   // -------------------------------------------------
   // Establish the Wib identity, i.e. Crate.Slot.Fiber
   // -------------------------------------------------
   static WibFrame::Id Id = WibFrame::id (1, 2, 3);


   // -------------------------
   // Create and fill the frame
   // -------------------------
   static WibFrameCreate frame (Id);
   frame.fill (timestamp, adcs);


   // ----------------------------
   // Setup the AXIS control flags
   // ----------------------------
   int first =                    (1 << (int)AxisUserFirst::Sof)
             |  (runEnable      ? (1 << (int)AxisUserFirst::RunEnable) : 0);
   flush     = flush ? (1 << (int)AxisUserLast::Flush) : 0;
   if (flush) printf ("Nonzero flush = %d\n", flush);


   // -------------------------------------------------
   // Commit this frame to both output and check stream
   // Note the check stream is really an output stream
   // so its control fields are only set on the very
   // first and very last words.
   //
   // The first word is the first word of the first frame
   // The last  word is the terminator word;
   // -------------------------------------------------
   commit (m_src, frame.m_w64[0], first, 0);
   commit (m_chk, frame.m_w64[0], (iframe == 0) ? first : 0, 0);
   for (int idx = 1; idx < 30-1; idx++)
   {
      uint64_t d = frame.m_w64[idx];
      commit (d, 0, 0);
   }
   commit (m_src, frame.m_w64[29], flush, 1 << (int)AxisLast::Eof);
   commit (m_chk, frame.m_w64[29], 0,     0);




   //print_adcs  (adcs, iframe);
   {
      static int NPrint = 0;
      if (NPrint > 0)
      {
         frame.print ();
         NPrint -= 1;
      }
   }

   if (flush) Odx = 0;
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
static void print_adcs (uint16_t const *adcs, int iframe)
{
   printf ("Input Adcs[%3.3x]\n", iframe);

   for (int idx = 0; idx < 128; idx++)
   {
      if ((idx & 0xf) ==    0) printf ("Adcs[%3.3x.%2.2x] =", iframe, idx);

      printf (" %4.4"PRIx16 , adcs[idx]);

      if ((idx & 0xf) == 0xf) putchar ('\n');
   }
}
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
inline void MyStreamOut::print ()
{
   std::cout << "OUTPUT DATA\n";
   int frame = 0;
   int odx   = 0;

   AxisOut &mAxis = *this;
   
   {
      AxisOut_t out = mAxis.read ();
      uint64_t data = out.data;
      int      user = out.user;
      int      last = out.last;
      odx += 1;
      std::cout << "Hdr: " << std::setw(16) << std::hex << data
                << "user:" << std::setw (4) << std::hex << user
                << "last:" << std::setw (4) << std::hex << last << std::endl;
   }


   while (1)
   {
      // There are 30 64-bit words in a WIB frame
      for (int widx = 0; widx < 30; widx++)
      {
         if ((widx % 4) == 0) std::cout << std::setw (5) << std::hex << odx << ':';

         AxisOut_t out = mAxis.read ();
         uint64_t data = out.data;
         int      user = out.user;
         int      last = out.last;

         // Check if this is the last word
         if (last)
         {
             if ((widx % 4) == 3) std::cout << std::endl;

             odx += 1;
             std::cout << "Tlr: " << std::setw(16) << std::hex << data
                          << "user:" << std::setw (4) << std::hex << user
                          << "last:" << std::setw (4) << std::hex << last << std::endl;

             goto EXIT;
         }
         else if (user != 0)
         {
            if ((widx % 4) == 3) std::cout << std::endl;

            std::cout << "Error: " << std::setw(16) << std::hex << data
                             << "user:" << std::setw (4) << std::hex << user
                             << "last:" << std::setw (4) << std::hex << last << std::endl;

         }
         else
         {
            std::cout << ' ' << std::setw (16) << std::hex << data;
         }

         if ((widx % 4) == 3) std::cout << std::endl;

         odx += 1;
      }


      std::cout << " Frame: " << std::setw(4) << std::hex << frame << std::endl;
      frame += 1;
      std::cout << std::endl;
      if (odx > 1 + 30*PACKET_K_NSAMPLES + 1)
      {
         std::cout << "Overrun\n";
         break;
      }
   }


   EXIT:


   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Check that the output stream matches the input stream
 *   \return == 0, okay
 *   \return != 0, failure
 *
 *   \param[in]      mAxis  The output stream
 *   \param[in]      sAxis  The source input stream
 *   \param[in]    ipacket  The packet number
 *
 *
\* ---------------------------------------------------------------------- */
inline int MyStreamOut::check (MyStreamOut              &mAxis,
                               MyStreamOut              &sAxis,
                               int                   packetNum)
{
   int   idx = 0;
   int nerrs = 0;

   while (1)
   {
      bool dst_empty = mAxis.empty ();
      bool src_empty = sAxis.empty ();


      if (!dst_empty && !src_empty)
      {
         AxisOut_t  dst = mAxis.read  ();
         AxisIn_t   src = sAxis.read  ();

         int error = dst.data != src.data;

         if (error)
         {
            std::cout << std::dec << std::setw ( 5) << "Err: Data @" << idx
                      << std::dec << std::setw ( 2) << " User: " << dst.user
                                                    << " Last: " << dst.last
                      << std::hex << std::setw ( 4) << " Keep: " << dst.keep
                      << std::hex << std::setw (16) << " Data: " << dst.data << " : " << src.data
                      << std::endl;
            nerrs+= 1;
            if (nerrs >= 100) return -1;
         }

         // Bit 1 of the user field must be set on the first word
         if ( (idx == 0) && (((dst.user & 2) == 0) || ((src.user & 2) == 0)) )
         {
            std::cout << std::dec << std::setw ( 5) << "Err: User 1 bit clear @" << idx
                      << std::dec << std::setw ( 2) << " User: " << dst.user
                                                    << " Last: " << dst.last
                      << std::hex << std::setw ( 4) << " Keep: " << dst.keep
                      << std::hex << std::setw (16) << " Data: " << dst.data
                      << std::endl;
            return  -1;
         }

         // Bit 1 of the user field must be clear all but the first word
         if ( (idx != 0) && ( (dst.user & 2) || (src.user & 2)) )
         {
            std::cout << std::dec << std::setw ( 5) << "Err: User 1 bit set @" << idx
                      << std::dec << std::setw ( 2) << " User: " << dst.user
                                                    << " Last: " << dst.last
                      << std::hex << std::setw ( 4) << " Keep: " << dst.keep
                      << std::hex << std::setw (16) << " Data: " << dst.data
                      << std::endl;
            return -1;
         }

         if (dst.last.test(static_cast<unsigned>(AxisLast::Eof)))
         {
            if (src.last.test (static_cast<unsigned>(AxisLast::Eof)))
            {
               // Check that both are now empty
               dst_empty = mAxis.empty ();
               src_empty = sAxis.empty ();
               if (dst_empty && src_empty)
               {
                  // All is well
                  return 0;
               }
               else if (dst_empty)
               {
                  std::cout << "Error: destination empty, but source not @ "
                            << std::dec << std::setw (5) << idx << std::endl;
                  return -1;
               }
               else
               {
                  std::cout << "Error: source empty, but destination not @ "
                            << std::dec << std::setw (5) << idx << std::endl;
                  return -1;
               }
            }
         }
      }
      else if (dst_empty || src_empty)
      {
         // This should not happen, dst.user or dst.last should signal the end
         std::cout << "Error:Premature empty: dst:src[" << idx << ']' << dst_empty << ':' << src_empty
                   << std::endl;
         break;
      }

      idx += 1;

   }

   return 0;
}
/* ---------------------------------------------------------------------- */


#endif
