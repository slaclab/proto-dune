/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     AxiBufChecker.h
 *  @brief    Checks the AXI buffers from the AXI stream DMA driver for 
 *            readability.  This was written when it was discoverd that
 *            a buffer would have a 4K byte range of locations that were
 *            not readable from user code.
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
 *  protoDune
 *
 *  @author
 *  russell@slac.stanford.edu
 *
 *  @par Date created:
 *  2017.02.22
 * *
 * @par Credits:
 * SLAC
 *
\* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\
   
   HISTORY
   -------
  
   DATE       WHO WHAT
   ---------- --- ---------------------------------------------------------
   2017.02.22 jjr Created
  
\* ---------------------------------------------------------------------- */

#include <stddef.h>
#include <unistd.h>
#include "AxiBufChecker.h"
#include <AxisDriver.h>

#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static sigjmp_buf point;

static void handler(int sig, siginfo_t *dont_care, void *dont_care_either)
{
   siglongjmp (point, 1);
}




/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Begins a new range of offsets marking a bad set of memory
 *         locations
 * \return Whether the offset was added.  There are only a finite number
 *         of these ranges, so there could be more bad ranges than can
 *         be accomodated.
 *
 * \param[in] beg  The beginning byte offset
 *
\* ---------------------------------------------------------------------- */
bool AxiBufChecker::begin_range (uint32_t beg)
{
   if (m_irx < sizeof (m_ranges) / sizeof (m_ranges[0]))
   {
      m_ranges[m_irx].beg = beg;
      return true;
   }
   else
   {
      return false;
   }
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Adds the ending offset to the range of offsets marking a bad
 *         set of memory locations
 * \return Whether the offset was added.  There are only a finite number
 *         of these ranges, so there could be more bad ranges than can
 *         be accomodated.
 *
 * \param[in] end  The ending byte offset
 *
\* ---------------------------------------------------------------------- */
bool AxiBufChecker::end_range (uint32_t end)
{
   if (m_irx < sizeof (m_ranges) / sizeof (m_ranges[0]))
   {
      m_ranges[m_irx].beg = end;
      return true;
   }
   else
   {
      return false;
   }
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Check one dma buffer for readability
 * \return The number of bad locations
 *
 * \param[in]   buf  The buffer to check
 * \param[in]   idx  Which buffer
 * \param[in] nsize  The size, in bytes, of the buffer
 *
\* ---------------------------------------------------------------------- */ 
int AxiBufChecker::check_buffer  (uint8_t       *buf, 
                                  int            idx,
                                  unsigned int nsize)
{
   uint32_t  sum __attribute__ ((unused));
   int       n64 = nsize / sizeof (uint64_t);
   uint64_t *ptr = (uint64_t *)buf;

   // -----------------------------------------------------------------
   // These variables capture the context when an exception occurs
   // The compiler has no idea that the flow can return through
   // this section, so when the optimizer is enabled, it assumes
   // compiles the code as if these variables have their initialization
   // values.
   // BOTTOM LINE: These variables must be declared volatile so that
   //              they are forced to memory.
   // -----------------------------------------------------------------
   volatile int       i64 =    -1;
   volatile bool    inerr = false;
   volatile int       end =    -1;
   volatile int      nbad =     0;


   // ------------------------
   // Establish signal handler
   // ------------------------
   struct sigaction sa;

   memset (&sa, 0, sizeof (sa));
   sigemptyset(&sa.sa_mask);

   sa.sa_flags     = SA_NODEFER;
   sa.sa_sigaction = handler;

   sigaction (SIGSEGV, &sa, NULL);

   if (sigsetjmp (point, 1) == 0)
   {
      i64 = 0;
      sum = 0;
   }
   else
   {
      // ---------------------------
      // Exceptions are fielded here
      // ---------------------------
      if (!inerr)
      {
         uint32_t beg_offset = i64 * sizeof (*ptr);

         // ---------------------------------------
         // Previously was not in error, but now is
         // Report and not that in error condition
         // ---------------------------------------
         if (m_irx <= 3)
         {
         fprintf(stderr, "       Error at buffer %d:%p offset %8.8x:%p\n",
                 idx, ptr, beg_offset, (char *)ptr + beg_offset);
         }

         inerr    = true;
         m_status = AxiBufChecker::Bad;
         begin_range (beg_offset);
      }

      // ----------------------------------------
      // Keep track of the 
      //    1) last location that was in error
      //    2) count of 64-bit locations in error
      //
      // Push on to check the next location
      // ----------------------------------------
      end   = i64;
      nbad += 1;
      i64  += 1;
   }


   // --------------------------------
   // Scan this buffer for readability
   // --------------------------------
   for (; i64 < n64;  i64++)
   {
      sum += ptr[i64];
      if (inerr) 
      {
         uint32_t end_offset = end * sizeof (*ptr);
         uint32_t cur_offset = i64 * sizeof (*ptr);

         // -----------------------------------------
         // If previously was in error, but no longer
         // note the last location that was in error
         // and issue report
         // -----------------------------------------
         inerr = false;
         end_range (end_offset);

         if (m_irx >= 3)
         {
         fprintf (stderr, ":%8.8x, continuing @ %8.8x\n", 
                  end_offset, cur_offset);
         }
      }
   }


   // --------------------------------------------------
   // Check if any unreadable locations were encountered
   // --------------------------------------------------
   if (nbad) 
   {
      // ----------------------------------------------
      // Catch the corner case where the last location
      // that was check for readability failed. In this
      // case the 'end' location has not been filled.
      // ----------------------------------------------
      if (inerr) 
      {
         uint32_t end_offset = end * sizeof (*ptr);
         end_range (end_offset);
         fprintf   (stderr, ":%8.8x\n", end_offset);
      }

      uint32_t nbad_bytes = nbad * sizeof (*ptr);
      fprintf (stderr, "       Finished with %8.8x bad bytes sum = %8.8x\n", 
               nbad_bytes, sum);
   }
   else
   {
      m_status = AxiBufChecker::Good;
   }


   // ------------------------------------------
   // Return the number of bad bytes encountered
   // ------------------------------------------
   return nbad * sizeof (*ptr);
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *                                                                             
 * \brief  Checks the dma buffers for readability
 * \retrun Number of bad buffers
 *
 * \param[in]   bad  List of bad buffers
 * \param[in]  bufs  Array of pointers to the buffers
 * \param[in] nbufs  The number of buffers
 * \param[in] nsize  The size, in bytes, of each buffer
 *
\* ---------------------------------------------------------------------- */
int AxiBufChecker::check_buffers (uint16_t      *bad,
                                  uint8_t     **bufs, 
                                  unsigned int nbufs, 
                                  unsigned int nsize)
{
   AxiBufChecker abc[nbufs];

   int nbad = 0;
   for (unsigned int ibuf = 0; ibuf < nbufs; ibuf++)
   {
      int iss = abc[ibuf].check_buffer (bufs[ibuf], ibuf, nsize);
      if (iss)
      {
         bad[nbad] = ibuf;
         nbad     +=    1;
      }
   }

   return nbad;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
int AxiBufChecker::extreme_vetting (AxiBufChecker *abc,
                                    int             fd,
                                    int          trial,
                                    uint8_t     **bufs,
                                    int          nbufs,
                                    int           size)
{
   int nbad = 0;
   fprintf (stderr, 
            "BEGIN: Extreme vetting of bad hombre buffers, trial = %d nbufs = %d\n", 
            trial, nbufs);


   // ------------------------------
   // Make a list of the bad buffers
   // ------------------------------
   for (int idx = 0; idx < nbufs; idx++)
   {
      uint32_t       index;
      int32_t        rxSize;

      // ---------------------------------------
      // Grab a buffer and check for readability
      // ---------------------------------------
      int maxerrs __attribute__ ((unused));
      int   nerrs __attribute__ ((unused));
      maxerrs =  2;
      nerrs   =  0;
      while (1)
     {
         rxSize = dmaReadIndex(fd, &index, NULL, NULL, NULL);

         if (rxSize <= 0)
         {
            if (nerrs <= maxerrs)
            {
                fprintf (stderr,
                   "     Read failure[%3d]: rxSize = %8.8x errno = %d\n", 
                         nerrs, rxSize, errno);
                fflush  (stderr);
            }
            nerrs += 1;
         }
         else
         {
            if (nerrs)
            {
               fprintf (stderr, "Read sucesss @ %d failures = %d trial = %d\n",
                        index, nerrs, trial);
            }

            break;
         }

      }


      // ---------------------------------------------
      // Was this buffer previously catalogued as bad?
      // If, so it should have been removed
      // ---------------------------------------------
      if (abc[index].m_status == AxiBufChecker::Bad)
      {
         fprintf (stderr, "Encountered a previous bad buffer %d\n", index);
      }

      //fprintf (stderr, "Servicing %d\n", index);
      //fflush  (stderr);


      if (1 || ((index < 107 || index > 111)))
      {
         // ------------------------
         // Check it for readability
         // ------------------------
         int iss = abc[index].check_buffer (bufs[index], index, size);
         if (iss)
         {
            // -------------------------------
            // Count the number of bad buffers
            // -------------------------------
            nbad += 1;
         }
      }
      else 
      {
         abc[index].m_status = AxiBufChecker::Skipped;
      }
   }


   // -----------------------------------------
   // All outstanding buffers have been checked
   // Return the good buffers
   // ----------------------------------------
   for (int idx = 0; idx < nbufs; idx++)
   {
      AxiBufChecker::Status status = abc[idx].m_status;

      if (status == AxiBufChecker::Bad)
      {
         // ---------------------------------------
         // Bad buffers are not returned
         // They are permanently out of circulation
         // ---------------------------------------
         fprintf (stderr, "       Buffer %d not returned\n",  idx);
      }
      else if (status == AxiBufChecker::Good ||
               status == AxiBufChecker::Skipped)
      {
         // -----------------------------------
         // Return the good and skipped buffers
         // -----------------------------------
         int status = dmaRetIndex (fd, idx);
         if (status)
         {
            fprintf (stderr, "\naxisPostUser error %d %d\n", status, errno);
         }
         else
         {
            fprintf (stderr, "AxisPostUser success %d\r", idx);
         }
      }
      else
      {
         // --------------------------------------------
         // These are buffers of unknown status.
         // The most likely reason is that these buffers 
         // were previously removed from circulation
         // --------------------------------------------
         fprintf (stderr, "       \nBuffer %d was not catalogued\n", idx);
      }
   }


   fprintf (stderr, 
            "END  : Extreme vetting of bad hombre buffers, trial = %d\n",
            trial);

   return nbad;
}
/* ---------------------------------------------------------------------- */
