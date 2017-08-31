/////////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////


// ----------------------------------------------------------------------
// 
// HISTORY
// 
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2017.08.28 jjr Fix position of trigger type in auxilliary block to 
//                match the documentation
// 2017.08.29 jjr Added code to support the more 'official' output record
//                format.  Stripped the debugging output back to one line
//                per transmitted event.  There still are messages
//                associated with error conditions.  
//                --> This code needs a good scrubbing, but should be
//                sufficient to test the data reading interface methods.
// 2017.05.11 jjr Added code to monitor the WIB packets for errors in the
//                timestamp and convert counts.
// 2016.10.28 jjr Added the triggering configuration parameters naccept
//                and nframe
// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
// -- Must ensure the the print macros are defined
// ----------------------------------------------------------------------
#define   __STDC_FORMAT_MACROS


#include "DaqBuffer.h"
#include "FrameBuffer.h"
#include <AxisDriver.h>
#include "AxiBufChecker.h"
#include "Headers.hh"
#include "TpcRecords.hh"

#include "List-Single.hh"

typedef uint32_t __s32;
typedef uint32_t __u32;


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#  undef  MONITOR_RATE
#  define MONITOR_RATE 0


using namespace std;


/* ---------------------------------------------------------------------- *//*!

   \class  TimingClockTicks
   \brief  Captures the parameters of and methods for the timing system
                                                                          */
/* ---------------------------------------------------------------------- */
class TimingClockTicks
{
public:
   /* ------------------------------------------------------------------- *//*!
   
     \enum  Constants
     \brief The constants associated with the timing system
                                                                          */
   /* ------------------------------------------------------------------- */
   enum Constants
   {
      CLOCK_PERIOD  = 20,   /*!< Number of nanoseconds per clock tick      */
      PER_SAMPLE    = 25,   /* Number of clock ticks between ADC samples   */
      SAMPLE_PERIOD = CLOCK_PERIOD * PER_SAMPLE, 
                            /*!< Number of nanoseconds between ADC samples */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

      \brief  Convert the period in micro seconds to period in clock ticks.
      \return The period in clock ticks.

      \param[in] period  The period, in usecs, to convert
                                                                          */
   /* ------------------------------------------------------------------- */
   constexpr static inline uint32_t from_usecs (uint32_t period) 
   { 
      return (1000 * period + CLOCK_PERIOD/2) / CLOCK_PERIOD; 
   }
   /* ------------------------------------------------------------------- */
};


#define MAX_DEST 2



/* ====================================================================== */
/* BEGIN: TimingMsg                                                       */
/* ---------------------------------------------------------------------- *//*!

  \class TiminMsg
  \brief Map the contents of a message from the hardware timing/trigger
         system.
                                                                          */
/* ---------------------------------------------------------------------- */
class TimingMsg
{
public:
   TimingMsg () { return; }

public:
   /* ------------------------------------------------------------------- *//*!

     \enum  Type
     \brief Enumerate the message types
                                                                          */
   /* ------------------------------------------------------------------- */
   enum Type
   {
      SpillStart = 0,  /*!< Start of spill                                */
      SpillEnd   = 1,  /*!< End   of spill                                */
      Calib      = 2,  /*!< Calibration trigger                           */
      Trigger    = 3,  /*!< Physics trigger                               */
      TimeSync   = 4   /*!< Timing resynchonization request               */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \enum  State
     \brief Enumerates the state the link with the timing/trigger system
                                                                          */
   /* ------------------------------------------------------------------- */
   enum State
   {
      Reset         = 0x0 , /*!< W_RST, -- Starting state after reset     */
      WaitingSfpLos = 0x1,  /*!< when W_SFP, -- Waiting for SFP LOS 
                                 to go low                                */
      WaitingCdrLock= 0x2,  /*!< when W_CDR, -- Waiting for CDR lock      */
      WaitingAlign  = 0x3,  /*!< when W_ALIGN, -- Waiting for comma 
                                  alignment, stable 50MHz phase           */
      WaitingFreq   = 0x4,  /*!< W_FREQ, -- Waiting for good frequency 
                                 check                                    */
      WaitingLock   = 0x5,  /*!< when W_LOCK, -- Waiting for 8b10 decoder
                                 good packet                              */
      WaitingGpsTs  = 0x6,  /*!< when W_RDY, -- Waiting for time stamp
                                 initialisation                           */
      Running       = 0x8,  /*!< when RUN, -- Good to go                  */
      ErrRx         = 0xc,  /*!< when ERR_R, -- Error in rx               */
      ErrGpsTs      = 0xd,  /*!< when ERR_T; -- Error in time stamp check */
   };
   /* ------------------------------------------------------------------- */

 

public:
   uint64_t timestamp () const { return m_timestamp; }
   uint32_t  sequence () const { return  m_sequence; }
   int           type () const { return (m_tsw >> 0) & 0xf; }
   int          state () const { return (m_tsw >> 4) & 0xf; }
   bool    is_trigger () const 
   { 
      return (m_tsw & 0xff) == ((Running << 4) | Trigger);
   };


   static TimingMsg *from (void *p) { return reinterpret_cast<TimingMsg *>(p); }
   static TimingMsg const *from (void const *p) { return reinterpret_cast<TimingMsg const *>(p); }



public:
   uint64_t m_timestamp;  /*!< 64-bit GPS timestamp                       */
   uint32_t  m_sequence;  /*!< Messaage sequence number                   */
   uint32_t       m_tsw;  /*!< Type and state word                        */
};
/* ---------------------------------------------------------------------- */
/* END: TimingMsg                                                         */
/* ====================================================================== */





/* ====================================================================== */
/* BEGIN: Latency List                                                    */
/* ---------------------------------------------------------------------- *//*!

  \class LatencyList
  \brief Captures a list of previous data frames.

  \par
   This list allows the event builder to reach back to promote data prior
   to the trigger.
                                                                          */
/* ---------------------------------------------------------------------- */
class LatencyList : public List<FrameBuffer>
{
public:
   static const int Depth = 10 - 1;
   static const bool Debug = false;
public:
   LatencyList () :
      List<FrameBuffer> (),
      m_remaining (Depth)
      {
         return;
      }

public:
   /* ------------------------------------------------------------------- *//*!

      \brief Initialize the latency list to an empty list

      \par
      This is generally done after some sort of error. Otherwise this is
      really just for completeness of functionality.
                                                                          */
   /* ------------------------------------------------------------------- */
   void init ()
   {
      List<FrameBuffer>::init ();
      m_remaining = Depth;
      fprintf (stderr, "LatencyList::init %p\n", this);
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief Resets to the latency list to empty.

     \par
      The list is assumed to have been emptied by removing all its 
      elements. All this does is reset the count of elements to capture
      to the maximum.
                                                                          */
   /* ------------------------------------------------------------------- */
   void resetToEmpty ()
   {
       m_remaining = Depth;
       return;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief   Returns the number of nodes currently on the list
     \return  The number of nodes currently on the list
                                                                          */
   /* ------------------------------------------------------------------- */
   int nnodes ()
   {
      return Depth - m_remaining;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief Replaces the oldest node of the latency list with the 
            specified \a node.  The oldest node is freed.

     \param[in] node The new node.
     \param[in]   fd The file descriptor used to free the oldest node
                                                                          */
   /* ------------------------------------------------------------------- */
   void replace (List<FrameBuffer>::Node *node, int fd)
   {
      // Add this node
      if (Debug) fprintf (stderr, "Adding[%3d] %p", node->m_body.index (), node);
      insert_tail (node);

      // Check if filled the latency depth
      if (m_remaining <= 0)
      {
         // Depth exceeded, remove the oldest node and free it
         node = remove_head ();

         int index = node->m_body.index ();

         if (Debug)
         {
            fprintf (stderr,
                     "  Returning[%3d] %p\n", index, node);
         }
         else if (index > 900)
         {
            fprintf (stderr,
                     " Error returning index:%d\n", index);
         }

         ssize_t iss = dmaRetIndex (fd, index);
         if (iss)
         {
            fprintf (stderr,
                     " Status of return = %zd\n", iss);
         }
      }
      else
      {
         // Not filled yet, but have eaten up a spot
         m_remaining -= 1;
         if (Debug) fprintf (stderr, " Remaining %d\n", m_remaining);
      }

      return;
   }
   /* ------------------------------------------------------------------- */

public:
   int  m_remaining;  /*!< The current number of nodes                    */
};
/* ---------------------------------------------------------------------- */
/* END: Latency List                                                      */
/* ====================================================================== */




/* ====================================================================== */
/* BEGIN: Event                                                           */
/* ---------------------------------------------------------------------- *//*!

  \class Event
  \brief Class to build and capture a complete event

  \par
   An event consists of
      -# A number of packets before and after the trigger. This is time
         window is determined at trigger time
      -# The packets specified above from all the contributor (sources)
                                                                          */
/* ---------------------------------------------------------------------- */
class Event
{
public:
   Event () { return; }
   Event (uint64_t beg, uint64_t end) : m_limits (beg, end) 
   {
      m_list[0].init ();
      m_list[1].init ();
      return; 
   }


public:
   typedef List<FrameBuffer>::Node Contribution; 

public:
   /* ------------------------------------------------------------------- *//*!

     \class TimestampLimits
     \brief Captures the lower and upper limits of the data time to 
            include in the event.
                                                                          */
   /* ------------------------------------------------------------------- */
   class TimestampLimits
   {
   public:

      /* ---------------------------------------------------------------- *//*!

        \brief  Constructor to the timestamp limits.  These are 
                arbitrarily set to 0.  

        \par
         The values are irrelevant since these will be set when an event
         is triggered.  A constructor of some sort is necessary since 
         an array of these is initialized at process start-up
                                                                          */
      /* ---------------------------------------------------------------- */
      TimestampLimits () : 
         m_beg (0),
         m_end (0)
      {
         return;
      }
      /* ---------------------------------------------------------------- */



      /* ---------------------------------------------------------------- *//*!

        \brief  Constructor to set the timestamp limits

        \param[in]  beg  The beginning time of the data for this event.
        \param[in]  end  The ending    time of the data for this event.
                                                                          */
      /* ---------------------------------------------------------------- */
      TimestampLimits (uint64_t beg, uint64_t end) :
         m_beg (beg),
         m_end (end)
      {
         return;
      }
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \brief  Sets the timestamp limits

        \param[in]  beg  The beginning time of the data for this event.
        \param[in]  end  The ending    time of the data for this event.
                                                                          */
      /* ---------------------------------------------------------------- */
      void set (uint64_t beg, uint64_t end)
      {
         m_beg = beg;
         m_end = end;
         return;
      }
      /* ---------------------------------------------------------------- */

   public:
      uint64_t  m_beg; /*!< The beginning time of the data for this event */
      uint64_t  m_end; /*!< The ending    time of the data for this event */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief Initializes the static information of an event

     \param[in] index The DMA data frame index to associate with this 
                       event. 

     \par
      In order to avoid any need for interlocked allocation and 
      deallocation, each event is associated with a DMA data frame. This
      buffer index can be any frame that is a member of this event. The
      only requirement is that this buffer must be the last buffer freed.
      This assures that the event associated with this DMA data frame 
      cannot be reused until the DMA data frame is reused, which must be
      after that buffer index has been free. Ergo, it is safe to use
      this event again. 
                                                                          */
   /* ------------------------------------------------------------------- */
   void init (int index)
   {
      m_list[0].init ();
      m_list[1].init ();
      m_index = index;
      m_ctbs  = 0;
      m_nctbs = 0;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief  Set the beginning and ending time of the data to include in
             this event.

     \param[in]  beg  The beginning time of the data for this event.
     \param[in]  end  The ending    time of the data for this event.
                                                                         */
   /* ------------------------------------------------------------------ */
   void setWindow (uint64_t beg, uint64_t end)
   {
      /*
      fprintf (stderr,
               "Trigger Window %16.16" PRIx64 " -> %16.16" PRIx64 "\n",
               beg, end);
      */
      m_limits.set (beg, end);
      return;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------  *//*!

     \brief Scans the latency list for the first data frame that is 
            included in this event.  All packets before this first frame
            are freed and the list of all packets, including this first
            frame, is transferred to the event.

     \param[in]  lists  The array of latency lists for all the contributors
     \param[in]     fd  The fd used to return the unused packets

     \par
      This process leaves the latency list empty.
                                                                          */
   /* ------------------------------------------------------------------- */
   bool seedAndDrain (LatencyList *lists, int fd)
   {
      bool found = false;

      // Initialize the pending list to waiting for all possible destinations
      m_ctbs  = (1 << MAX_DEST) - 1;
      m_nctbs = 0;

      // Loop over the latency lists of all the destinations
      for (int idx = 0; idx < MAX_DEST; idx++)
      {
         LatencyList             *list =  &lists[idx];
         List<FrameBuffer>::Node *flnk = list->m_flnk;


         /*
         fprintf (stderr,
                  "SeedAndDrain[%d] list = %p : %p:%p (check empty)\n", 
                  idx,
                  list,
                  flnk,
                  list->m_blnk);
         */


         // If the latency list is empty, need to initialize the event list
         if (flnk == reinterpret_cast<decltype (flnk)>(list))
         {
            m_list[idx].init ();
            m_npkts[idx] = 0;
            /*
            fprintf (stderr,
                     "SeedAndDrain[%d] list = %p : %p (is empty)\n", 
                     idx,
                     m_list[idx].m_flnk,
                     m_list[idx].m_blnk);
            */

            continue;
         }

         uint64_t winBeg = m_limits.m_beg;
         uint64_t winEnd = m_limits.m_end;

         int npkts = list->nnodes ();

         // ----------------------------------------------------------------
         // Find the first node that has a 
         //       timestamp >= lower limit 
         // of the event window
         // 
         // Because these occur before this trigger and all future triggers
         // occur at a later time and are not allowed to overlap, nodes 
         // before the lower limit of the event window can be freed.  
         // ---------------------------------------------------------------
         while (1)
         {

            // -------------------------------------------------
            // Check if an portion of the event windos is within
            // this packet
            // ------------------------------------------------
            uint64_t pktBeg = flnk->m_body._ts_range[0];
            uint64_t pktEnd = flnk->m_body._ts_range[1];


            if ( (pktEnd  >  winBeg && pktEnd <  winEnd) ||
                 (pktBeg  >= winBeg && pktBeg <= winEnd)  )
            {
               // ----------------------------------------
               // This transfers all the remaining nodes 
               // in 'list' and set it to empty.
               // ----------------------------------------
               //m_list[idx] = *list;
               //list->init ();
               m_list[idx].transfer (list);

               /*
               fprintf (stderr,
                        "SeedAndDrain[%d] list = %p : %p  list %p : %p\n", 
                        idx,
                        m_list[idx].m_flnk,
                        m_list[idx].m_blnk,
                       &m_list[idx],
                        m_list[idx].m_flnk->m_flnk);
               */


               // ------------------------------------------
               // Since triggers are not allowed to overlap, 
               // the latency list must be reset to empty
               // ------------------------------------------
               found  = true;
               break;
            }
            else
            {
               // This packet occurred before the trigger window opened
               int index = flnk->m_body.index ();

               // Reduce the packet count
               npkts    -= 1;

               /*
               fprintf (stderr,
                        "Discarding[%1d] node = %p {%16.16" PRIx64 " -> "
                        "%16.16" PRIx64 "\n",
                        idx, flnk, 
                        flnk->m_body._ts_range[0],
                        flnk->m_body._ts_range[1]);
               */

               

               // Remove and free the node we were working on
               list->remove_head ();
               dmaRetIndex (fd, index);

            
               // Get the next node
               flnk = flnk->m_flnk;


               // -------------------------------------------
               // Check if the latency list is exhausted.
               // 
               // This really should not happen.  Either
               //   1. This contributor is not enabled.
               //      -> This means the latency list 
               //         will be empty and this is checked
               //         for before this loop is entered.
               //   2. Lower trigger window edge is after
               //      the last entry.
               //      entry.
               //      -> This probably means the trigger
               //         arrived too late or possibly that
               //         the lower limit of the trigger
               //         window is nearly 0, so one does not
               //         have to reach back in time
               // -------------------------------------------
               if (flnk == reinterpret_cast<decltype (flnk)>(list))
               {
                  static int Exhausted = 0;
                  fprintf (stderr, "Latency[%d] exhausted %d\n", 
                           idx, Exhausted++);
                  list->resetToEmpty ();
                  break;
               }


            }
         }

         // -------------------------------------------------
         // Stash the number of packets so far for this event
         // -------------------------------------------------
         m_npkts[idx] = npkts;
      }

      return found;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \enum   Fate
     \brief  Enumerates the fate of the node
                                                                          */
   /* ------------------------------------------------------------------- */
   enum Fate 
   {
      TooEarly  = -2,  /*!< Data node was before the lower windom         */
      TooLate   = -1,  /*!< Data node was after  the upper window         */
      Added     =  0,  /*!< Data node was added to the event              */
      Completed =  1,  /*!< Data node completed the event, \e i.e. post it*/
      Overrun   =  2,  /*!< Data node was well past the ending time, 
                            this likely means some contributor is very
                            late. Action is usually to reject this node
                            and complete the event. This is effectively
                            a sort of timeout on building this event      */
      CompletedOverrun = 3,  
                      /*!< Beginning of packet packet outside of window
                           If the timestamp is correct, the packet will
                           be within the window.  What this means is that
                           the timestamp jumped forward beyond the
                           window                                         */

   };
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief  Attempt to add this node to the event
     \return The fate of this node/event. \sa enum Fate.

     \param[in]  node The node to add
     \param[in]  dest The contributor index
                                                                          */
   /* ------------------------------------------------------------------- */
   Fate add (List<FrameBuffer>::Node *node, int dest)
   {
      static int Count[2] = {0, 0};
      uint32_t ctb_mask = (1 << dest);

      //fprintf (stderr, 
      //         "Adding: %2d:%1.1x %16.16" PRIx64 " >= %16.16" PRIx64 "\n",
      //         dest, m_ctbs, endTime, m_limits.m_end);


      // Check if this contribution is pending
      if (m_ctbs & ctb_mask) 
      {
         uint64_t  begTime = node->m_body._ts_range[0];
         uint64_t  endTime = node->m_body._ts_range[1];

         // --------------------------------------------
         // Is this packet end before the trigger window
         // This show be very rare.  This means the data
         // packets are arriving much later in real time
         // than the trigger.
         // --------------------------------------------
         if (endTime < m_limits.m_beg)
         {
            /*
            fprintf (stderr,
                     "Adding node[%1d.%3d] %p evtEnd < winBeg %16.16" PRIx64 "<"
                     " %16.16" PRIx64 " early\n",
                     dest, Count[dest]++,
                     node,
                     endTime, m_limits.m_beg);
            */
            return TooEarly;
         }
            

         // Is this within the trigger window
         if (begTime <= m_limits.m_end)
         {
            // Yes, add to the list 
            /*
            fprintf (stderr, 
                     "Adding node[%1d.%3d] %p flnk:%p blnk:%p end: %16.16" PRIx64 " -> "
                     "%16.16" PRIx64 "\n",
                     dest,
                     Count[dest]++,
                     node, 
                     m_list[dest].m_flnk,
                     m_list[dest].m_blnk,
                     begTime,
                     endTime);
            */
            m_list [dest].insert_tail (node);
            m_npkts[dest] += 1;


            // Is this end time after the trigger window
            if (endTime >= m_limits.m_end)
            {
               // ---------------------------------
               // Yes, this contributor is complete
               // Remove it from the pending list
               // ---------------------------------
               m_nctbs += 1;
               m_ctbs  &= ~ctb_mask;
               if (m_ctbs == 0) { Count[0] = Count[1] = 0; return Completed; }

               // This did not complete the event
               else            {                          return     Added; }
            }
            else
            {
               // ------------------------------------
               // No, this contributor is not complete
               // It was just added
               // -----------------------------------
               return Added;
            }

         }
         else
         {
            // ------------------------------------------------
            // Outside the window
            // This should happen iff there are dropped packets.
            // If all is in sequence, the previous should have
            // completed this contributor.
            // ------------------------------------------------
            static int DumpCount = 10;
            if (DumpCount >= 0)
            {
               DumpCount -= 1;
               fprintf (stderr, "Rejecting packet begTime > window endTime "
                        "%16.16" PRIx64 " > %16.16" PRIx64 "\n",
                        begTime, m_limits.m_end);
            }

            m_ctbs &= ~ctb_mask;
            if (m_ctbs == 0) { Count[0] = Count[1] = 0; return CompletedOverrun; }
            return TooLate;
         }
      }
      else
      {
         // --------------------------------------------------
         // This contributor Is already completed
         // Just waiting for the other contributors to come in
         // --------------------------------------------------
         if (true)
         {
            // If have waited too long, then declare this event complete
            ///fputs ("Overrun\n", stderr);
            Count[0] = 0;
            Count[1] = 0;
            return Overrun;
         }
         else
         {
            // --------------------
            // Willing to wait more
            // --------------------
            return TooLate;
         }
      }
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief Frees all the data nodes associated with this event; 
            effectively the destructor

     \param[in]  fd The fd used to free the data nodes.

     \par
      This is usually called after the event has been posted
                                                                          */
   /* ------------------------------------------------------------------- */
   void free (int fd)
   {
      static int Count = 0;

      int dest __attribute__ ((unused));
      int  cnt __attribute__ ((unused));
      int  max = 0;


      // Return the frames
      for (int idx = 0; idx < MAX_DEST; idx++)
      {
         List<FrameBuffer>::Node *node = m_list[idx].m_flnk;
         List<FrameBuffer>::Node *end  = reinterpret_cast<decltype(end)>
                                                          (&m_list[idx]);

         int count = 0;
         while (node != end)
         {

            // Return the underlying DMA buffer
            int index = node->m_body.index ();

            if ((unsigned int)index > 900)
            {

               fprintf (stderr, "Bad [%d.%d] index on free %p 0x%8.8x %d ******\n", 
                        idx, count++, node, index, index);

               Count++;
               exit (-1);
               if (max++ > 10) break;
               continue;
            }


            // The frame buffer associated with this event 
            // must be the last to be freed
            if (index != m_index)
            {
               /*
               fprintf (stderr, "Node[%2d.%2d] freeing index %d\n", 
                        idx, count++, index);
               */
               if (count > 40) 
               {
                  fprintf (stderr, "Free error: too many nodes\n");
                  break;
               }
               dmaRetIndex (fd, index);
            }
            else
            {
               dest =   idx;
               cnt  = count++;
            }

            node = node->m_flnk;
         }
      }

      /*
      fprintf (stderr, "Node[%2d.%2d] freeing index %d bad = %d\n",
               dest, cnt, m_index, Count);
      */
      dmaRetIndex (fd, m_index);

   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

      \class Trigger
      \brief Captures the triggering information for this event
                                                                          */
   /* ------------------------------------------------------------------- */
   class Trigger
   {
   public:
      Trigger () { return; }

      /* ---------------------------------------------------------------- *//*!

         \enum  Source
         \brief Enumerate the trigger sources
                                                                          */
      /* ---------------------------------------------------------------- */
      enum Source
      {
         Software = 0,  /*!< Software trigger                             */
         Hardware = 1   /*!< Hardware trigger                             */
      };

   public:

      /* ---------------------------------------------------------------- *//*!

         \brief Initializes the trigger information for a software trigger

         \param[in] timestamp The 64-bit trigger timestamp
         \param[in]  sequence The 32-bit trigger sequence number
         \param[in]    opaque A 32-bit opaque value
                                                                          */
      /* ---------------------------------------------------------------- */
      void init (uint64_t timestamp, uint32_t sequence, uint32_t opaque)
      {
         m_timestamp = timestamp;
         m_sequence  = sequence;
         m_opaque    = opaque;
         m_source    = Software;
         return;
      }
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \brief Initializes the trigger information for a hardware trigger

        \param[in] msg  The trigger/timing message
                                                                          */
      /* ---------------------------------------------------------------- */
      void init (TimingMsg const *msg)
      {
         m_timestamp = msg->m_timestamp;
         m_sequence  = msg->m_sequence;
         m_opaque    = msg->m_tsw;
         m_source    = Hardware;
      }
      /* ---------------------------------------------------------------- */

   public:
      uint64_t m_timestamp;  /*!< The trigger timestamp                   */
      uint32_t  m_sequence;  /*!< The trigger sequence number             */
      uint32_t    m_opaque;  /*!< An 32-bit value that is type dependent  */
      enum Source m_source;  /*!< The trigger source                      */
   };
   /* ------------------------------------------------------------------- */
      
      
public:
   TimestampLimits    m_limits;  /*!< Event time window                   */
   Trigger           m_trigger;  /*!< Event triggering information        */
   List<FrameBuffer> m_list[2];  /*!< List of the contributors            */
   int                 m_nctbs;  /*!< Count of contributors               */
   uint32_t             m_ctbs;  /*!< When building, bit mask of 
                                      incomplete contributors 
                                      When completed, bit mask of 
                                      contributions present               */
   uint16_t         m_ctdid[2];  /*!< The WIB crate.slot.fiber id         */
   uint16_t         m_npkts[2];  /*!< The number of packets in this event */
   uint16_t            m_index;  /*!< Associated dma frame buffer index   */
};
/* ---------------------------------------------------------------------- */
/* END: Event                                                             */
/* ====================================================================== */





/* ====================================================================== */
/* BEGIN: SoftTrigger                                                     */
/* ---------------------------------------------------------------------- */
class SoftTrigger
{
public:
   static const uint32_t SoftTriggerPeriod = 1000 * 1000;

   SoftTrigger ()                : 
      m_period (TimingClockTicks::from_usecs (1000 * 1000)) { return; }
   SoftTrigger (uint32_t period) : 
      m_period (TimingClockTicks::from_usecs (period))      { return; }


   /* ------------------------------------------------------------------- *//*!

      \brief  Configure the software trigger to trigger every period usecs.

      \param[in]  period The triggering period in usecs.
                                                                          */
   /* ------------------------------------------------------------------- */
   void configure (uint32_t period)
   {
      m_period = TimingClockTicks::from_usecs (period);
   }
   /* ------------------------------------------------------------------- */


   bool check (uint64_t timestamp[2]) const
   {
      return check (timestamp, m_period);
   }

   /* ------------------------------------------------------------------- *//*!

     \brief  Check if this time range contains a software trigger
     \retval >= 0,  the trigger time
     \retval <  0,  no trigger

     \param[in]  timestamp  The beginning and ending time range to check
                                                                          */
   /* ------------------------------------------------------------------- */
   static int64_t check (uint64_t timestamp[2], uint32_t period)
   {
      /* Hardwire trigger rate to 1 sec */
      uint32_t beg = timestamp[0] % period;
      uint32_t end = timestamp[1] % period;

      // If the end phase < beg phase, then phase went through 0
      if (end < beg) 
      {
         /*
         uint64_t delta = timestamp[1] - timestamp[0];
         fprintf (stderr,
                  "Check %16.16" PRIx64 ":%8.8" PRIx32 ":"
                  "%16.16" PRIx64 ":%8.8" PRIx32 " trigger -> yes  %16.16" PRIx64 "\n",
                  timestamp[0], beg, timestamp[1], end, delta);
         */
         return timestamp[1] - end;
      }
      else 
      {
         //fprintf (stderr, " -> no\n");
         return -1;
      }
   }
   /* ------------------------------------------------------------------- */


private:
   int32_t  m_period;
};
/* ---------------------------------------------------------------------- */
/* END: SoftTrigger                                                       */
/* ====================================================================== */


/* ====================================================================== */
#  if     MONITOR_RATE
/* ---------------------------------------------------------------------- *//*!

  \brief A debugging aid only.  It should be disable in production 
         releases.
                                                                          */
/* ---------------------------------------------------------------------- */
class MonitorRate
{
public:
   MonitorRate ()
   {
      gettimeofday (&m_begTime, NULL);
      memset (m_count, 0, sizeof (m_count));
   }


   void monitor (int dest)
   {
      struct timeval   difTime;
      struct timeval   curTime;
      gettimeofday (&curTime, NULL);
      timersub     (&curTime, &m_begTime, &difTime); 

      uint32_t interval = 1000 * 1000 * difTime.tv_sec + difTime.tv_usec;

      int idx = (dest <= MAX_DEST) ? dest+1 : 1+MAX_DEST+1;
      m_count[idx] += 1;

      if (interval >= 10* 1000 * 1000)
      {
         if ( (m_total & 0x1f) == 0)
         {
            fprintf (stderr,
                     " Empty + Dest=0 + Dest=1 + Dest=2 + Unknown = "
                     " Total/    usecs\n");
         }

         m_total       += 1;
         uint32_t total = m_count[0] + m_count[1] 
                        + m_count[2] + m_count[3] + m_count[4];
         fprintf (stderr, "%6u + %6u + %6u + %6u +  %6u = %6u/%9u\n", 
                  m_count[0], m_count[1], m_count[2], m_count[3], m_count[4],
                  total, interval);

         // Reset for next batch 
         m_begTime = curTime;
         memset (m_count, 0, sizeof (m_count));
      }
   }
   

private:
   struct timeval     m_begTime; /*!< Begin time of this sample           */
   uint32_t             m_total; /*!< Total number of calls               */
   uint32_t m_count[MAX_DEST+3]; /*!< Count of dest types                 */
};

/* ---------------------------------------------------------------------- */
# else
/* ---------------------------------------------------------------------- */

class MonitorRate
{
public:
   MonitorRate  ()         { return; }
   void monitor (int dest) { return; }
};

/* ---------------------------------------------------------------------- */
#  endif
/* ====================================================================== */


/* ====================================================================== */
/* BEGIN: FrameDiagnostics                                                */
/* ---------------------------------------------------------------------- *//*!

   \brief Collect all the frame diagnostics in one place
                                                                          */
/* ---------------------------------------------------------------------- */
class FrameDiagnostics
{
public:
   FrameDiagnostics      (uint32_t   freqReceived, 
                          uint32_t freqTimingDump,
                          uint32_t   freqDataDump,
                          uint32_t  freqDataCheck);

   void dump_received    (int               index,
                          int                dest,
                          int              rxSize);

   void dump_timingFrame (void const        *data,
                          int              nbytes);

   void dump_dataFrame   (void const        *data,
                          int              nbytes,
                          int                dest);

   void check_dataFrame  (uint64_t const    *data,
                          int              nbytes,
                          unsigned int       dest,
                          int              sample);
public:

   /* ------------------------------------------------------------------- *//*!

      \class Activator
      \brief Helper class to determine whether to activate a particular
             diagnostic.
                                                                          */
   /* ------------------------------------------------------------------- */
   class Activator
   {
   public:
      uint32_t  m_freq;
      uint32_t  m_count;

   public:
      void init (uint32_t freq)
      {
         m_freq  = freq;
         m_count = freq == 0xffffffff ? freq : 0;
      }


      bool declare ()
      {
         if (m_count--) return false;
         else           { m_count = m_freq; } return true;
      }
      

   };
   
   Activator         m_received;
   Activator  m_timingFrameDump;
   Activator  m_dataFrameDump[2];
   Activator m_dataFrameCheck[2];
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief  Constructor for the various frame diagnostic routines

   \param[in] freqReceived   The frequency to dump received information,
   \param[in] freqTimingDump The frequency to dump timing frames
   \param[in] freqDataDump   The frequency to dump a data frame, 
   \param[in] freqDataCheck  The frequency to check a data frame, 

   \par
    If a dump frequency is set to 100, this means to activate the
    diagnostic every 100th time.
                                                                          */
/* ---------------------------------------------------------------------- */
FrameDiagnostics::FrameDiagnostics (uint32_t freqReceived, 
                                    uint32_t freqTimingDump,
                                    uint32_t freqDataDump, 
                                    uint32_t freqDataCheck)
{
   // ----------------------------------
   // Received DMA dumper
   // -------------------
   m_received.init (freqReceived);
   // ----------------------------------


   // ----------------------------------
   // Timing frame dumper
   // -------------------
   m_timingFrameDump.init (freqTimingDump);
   // ----------------------------------
      

   // ----------------------------------
   // Data frame dumper
   // -------------------
   m_dataFrameDump[0].init (freqDataDump);
   m_dataFrameDump[1].init (freqDataDump);
   // ----------------------------------


   // ----------------------------------
   // Data frame checker
   // -------------------
   m_dataFrameCheck[0].init (freqDataCheck);
   m_dataFrameCheck[1].init (freqDataCheck);
   // ----------------------------------


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Dump the information about a received DMA transaction

   \param[in]  index  The frame index
   \param[in]   dest  The destination stream
   \param[in] rxSize  The received size
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_received (int index, int dest, int rxSize)
{
   if (!m_received.declare ()) return;

   printf ("Index:%4u  dest: %3u rxSize = %6u\n", index, dest, rxSize);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Primitive hex dump of the received timing frame
 *
 *  \param[in]   data The timing frame data
 *  \param[in] nbytes The number bytes in the data
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_timingFrame (void const *data, 
                                                int       nbytes)
{
   struct TimingMsg
   {
      uint64_t gps;
      uint32_t seq;
      uint32_t ctl;
   };

   static struct TimingMsg LastMsg = { 0, 0, 0};

   if (!m_timingFrameDump.declare ()) return;

   TimingMsg const *s = reinterpret_cast<TimingMsg const *>(data);

   int64_t deltaGps = s->gps - LastMsg.gps;
   int32_t deltaSeq = s->seq - LastMsg.seq;

   printf ("t = %16.16" PRIx64 " %8.8" PRIx32 " %8.8" PRIx32 " "
           "%16" PRId64 " %8" PRId32" \n",
           s->gps, s->seq, s->ctl, deltaGps, deltaSeq);

   LastMsg = *s;

   /*
   for (int idx = 0; idx < ns; idx++)
   {
      if ((idx%16) == 0) printf ("t[%3d]:", idx);
      printf (" %4.4" PRIx16, s[idx]);
      if ((idx%16) == 15) putchar ('\n');
   }

   // If have left off in the middle of a line
   if (ns % 16) putchar ('\n');
   */
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Primitive hex dump of the received data frame
 *
 *  \param[in]   data The frame data
 *  \param[in] nbytes The number bytes in the data
 *  \param[in]   dest Which stream
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_dataFrame (void const *data, 
                                              int       nbytes,
                                              int         dest)
{
   if (!m_dataFrameDump[dest].declare ()) return;

   uint16_t const *s = (uint16_t const *)data;
   int            ns = nbytes / sizeof (*s);

   for (int idx = 0; idx < ns; idx++)
   {
      if ((idx%16) == 0) printf ("d[%1d.%3d]:", dest, idx);
      printf (" %4.4" PRIx16, s[idx]);
      if ((idx%16) == 15) putchar ('\n');
   }

   // If have left off in the middle of a line
   if (ns % 16) putchar ('\n');
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Performs basic integrity checks on the frame.
 *
 *  \param[in]   data The frame data
 *  \param[in] nbytes The number bytes in the data
 *  \param[in]   dest The originating module, 0 or 1
 *  \param[in] sample The sampling interval. Since not all received 
 *                    packets are checked, this is used to predict the
 *                    values of the next packet that will be checked.
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::check_dataFrame (uint64_t const *data,
                                               int           nbytes,
                                               unsigned int    dest,
                                               int           sample)
{
#  define N64_PER_FRAME 30
#  define NBYTES (unsigned int)(((1 + N64_PER_FRAME * 1024) + 1) \
                * sizeof (uint64_t))

   if (!m_dataFrameCheck[dest].declare ()) return;

   struct History_s
   {
      uint64_t  timestamp;
      uint16_t convert[2];
   };

   static struct History_s History[2] = { {0, {0, 0}}, {0, {0,0}} };
   static unsigned int        Counter = 0;
   int                            n64 = nbytes / sizeof (*data) - 2;
  
   //uint64_t hdr = data[0];
   data += 1;


   // ------------------------------------
   // Check for the correct amount of data
   // ------------------------------------
   if (nbytes != NBYTES)
   {
      printf ("Aborting @ %2u.%6u %u != %u incorrect amount of data received\n",
              dest, Counter, nbytes, NBYTES);
      return;
   }

   
   // -------------------------------------------------------------
   // Seed predicted sequence number with either
   //   1) The GPS timestamp of the previous packet
   //   2) The GPS timestamp of the first frame
   // -------------------------------------------------------------
   uint16_t   predicted_cvt_0;
   uint16_t   predicted_cvt_1;
   uint64_t   predicted_ts = History[dest].timestamp;
   if (predicted_ts)
   {
      predicted_ts    = data[ 1];
      predicted_cvt_0 = data[ 2] >> 48;
      predicted_cvt_1 = data[16] >> 48;
   }
   else
   {
      predicted_cvt_0 = History[dest].convert[0];
      predicted_cvt_1 = History[dest].convert[1];
   }


   // --------------------
   // Loop over each frame
   // --------------------
   unsigned int    frame = 0;
   for (int idx = 0; idx < n64; idx += N64_PER_FRAME)
   {
      uint64_t d = data[idx];

      // -------------------------
      // Check the comma character
      // -------------------------
      if ((d & 0xff) != 0xbc)
      {
         printf ("Error frame @ %2u.%6u.%4u: %16.16" PRIx64 "\n",
                 dest, Counter, frame, d);
      }

      // -----------------------------------
      // Form and check the sequence counter
      // ----------------------------------
      uint64_t timestamp = data[idx+1];
      uint16_t     cvt_0 = data[idx +  2] >> 48;
      uint16_t     cvt_1 = data[idx + 16] >> 48;

      int error = ((timestamp != predicted_ts   ) << 0)
                | ((cvt_0     != predicted_cvt_0) << 1)
                | ((cvt_1     != predicted_cvt_1) << 2);

      if (error)
      {
         printf ("Error  seq @ %2u.%6u.%4u: "
                 "ts: %16.16" PRIx64 " %c= %16.16" PRIx64 " "
                 "cvt: %4.4" PRIx16 " %c= %4.4" PRIx16 " "
                 "cvt: %4.4" PRIx16 " %c= %4.4" PRIx16 "\n",
                 dest, Counter, frame, 
                 timestamp, (error&1) ? '!' : '=', predicted_ts,
                 cvt_0    , (error&2) ? '!' : '=', predicted_cvt_0,
                 cvt_1    , (error&4) ? '!' : '=', predicted_cvt_1);

         // -----------------------------------------------------
         // In case of error, resynch the predicted to the actual
         // -----------------------------------------------------
         predicted_ts    = timestamp;
         predicted_cvt_0 = cvt_0;
         predicted_cvt_1 = cvt_1;
      }
      else if (1 && ((Counter % (8*32768/sample)) == 0) && ((frame % 256) == 0))
      {
         // ------------------------------------------
         // Print reassuring message at about 1/2-1 Hz
         // ------------------------------------------
         printf ("Spot check @ %2u.%6u.%4u: "
                 "ts: %16.16" PRIx64 " == %16.16" PRIx64 " "
                 "cvt: %4.4" PRIx16 " == %4.4" PRIx16 " "
                 "cvt: %4.4" PRIx16 " == %4.4" PRIx16 "\n",
                 dest, Counter, frame, 
                 timestamp, predicted_ts,
                 cvt_0    , predicted_cvt_0,
                 cvt_1    , predicted_cvt_1);
      }


      // -------------------------------------
      // Advance the predicted sequence number
      // Advance the frame counter
      // -------------------------------------
      predicted_ts     = timestamp + TimingClockTicks::PER_SAMPLE;
      predicted_cvt_0 += 1;
      predicted_cvt_1 += 1;
      frame           += 1;
   }

   // -----------------------------------------------
   // Keep track of the number of times called and
   // the expected sequence number of the next packet
   // for this destination.
   // -----------------------------------------------
   Counter                 += sample;
   History[dest].timestamp  = predicted_ts    + (sample - 1) 
                                              * TimingClockTicks::PER_SAMPLE;
   History[dest].convert[0] = predicted_cvt_0 +  sample - 1;
   History[dest].convert[1] = predicted_cvt_1 +  sample - 1;

   return;
}
/* ---------------------------------------------------------------------- */
/* END:  FrameDiagnostic                                                  */
/* ====================================================================== */



class HeaderAndOrigin
{
public:
   HeaderAndOrigin () { return; }

public:
   pdd::Fragment::Header<pdd::Fragment::Type::Data> m_header;
   pdd::Fragment::Originator                        m_origin;

   // ----------------------------------------------------
   // Return the number bytes rounded to a 64-bit boundary
   // ----------------------------------------------------
   uint32_t n64 ()
   {
      uint32_t n64l = sizeof(m_header) / sizeof (uint64_t)
                    + m_origin.n64 ();
      return n64l;
   }


};


HeaderAndOrigin HeaderOrigin;





// Class create
DaqBuffer::DaqBuffer () {
   _rxThreadEn     = false;
   _workThreadEn   = false;
   _txThreadEn     = false;
   _rxPend         = 0;
   _rxQueue        = NULL;
   _relQueue       = NULL;
   _txReqQueue     = NULL;
   _txAckQueue     = NULL;
   _txPend         = 0;

   hardReset();

   _txFd       = -1;
   _txSequence = 0;


   RceInfo rceInfo;
   rceInfo.open  ();
   rceInfo.read  ();
   rceInfo.print ();
   rceInfo.close ();

   int n64 __attribute__ ((unused));

   auto &origin = HeaderOrigin.m_origin;
   n64          = origin.construct (rceInfo.m_firmware.m_fpgaVersion,
                                    SoftwareVersion,
                                    rceInfo.m_clusterElement.m_rptSwTag,
                                    strlen (rceInfo.m_clusterElement.m_rptSwTag),
                                    rceInfo.m_clusterIpmc.m_serialNumber,
                                    rceInfo.m_clusterIpmc.m_address,
                                    rceInfo.m_clusterIpmc.m_groupName,
                                    strlen (rceInfo.m_clusterIpmc.m_groupName));

   /*
   fputs ("Origin\n", stderr);
   uint32_t const *p = reinterpret_cast<decltype(p)>(&origin);
   for (int idx = 0; idx < (nbytes + 3) / 4;  idx++)
   {
      fprintf (stderr, "origin[%2d] = %8.8" PRIx32 "\n", idx, p[idx]);
   }
   */

   resetCounters ();
}



// Class destroy
DaqBuffer::~DaqBuffer () {
   this->disableTx();
   this->close();
}




// Method to reset configurations to default values
void DaqBuffer::hardReset () {
   _config._runMode         = RunMode::IDLE;
   _config._blowOffDmaData  =     0;
   _config._blowOffTxEth    =     0;
   _config._pretrigger      =  5000;
   _config._posttrigger     =  5000;
   _config._period          = 1000 * 1000;
}


/* ---------------------------------------------------------------------- *//*!

   \brief  Helper method to construct a AXI dma channel
   \retval true,  successful construction
   \retval false, successful construction

   \param[in]     dma  The dma channel to construct
   \param[in] devname  The device name
   \param[in]    name  A descriptive name used in the error messages,
   \param[in]   dests  The array of destinations to enable
   \param[in]  ndests  The number destinations to enable
                                                                          */
/* ---------------------------------------------------------------------- */
static bool construct (DaqDmaDevice     &dma, 
                       char    const*devname, 
                       char    const   *name,
                       uint8_t const  *dests, 
                       int            ndests)
{
   // Open the device
   int status = dma.open (devname);
   if (status < 0)
   {
      fprintf(stderr,
              "DaqBuffer::open -> Error opening %-8s dma device\n",
              name);
      return false;
   }

   // Enable the destinations
   status = dma.enable (dests, ndests);
   if (status < 0)
   {
      fprintf(stderr,
              "DaqBuffer::open -> Unable to set %-8s dma mask\n", 
              name);
      return false;
   }


   // Create the index -> virtual address map
   status = dma.map ();
   if (status < 0)
   {
      fprintf(stderr,
              "DaqBuffer::open -> Failed to map %-8s dma buffers\n",
              name);
      return false;
   }


   // Report the parameters 
   fprintf(stderr, 
           "DaqBuffer::open -> %-8s dma, running with %4i + %4i = %4i (tx+rx=total)"
           " buffers of %8i bytes\n",
           name,
           dma._txCount, dma._rxCount, dma._bCount, dma._bSize);


   return true;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Constructs the DaqBuffer class

  \par
   The DMA interfaces are established and the various threads started.
                                                                          */
/* ---------------------------------------------------------------------- */
bool DaqBuffer::open (string devPath) 
{
   struct sched_param txParam;

   puts ("-------------------------\n"
         "JJ's version of rceServer\n"
         "-------------------------\n");

   this->close();



   // --------------------------------
   // Establish the Data DMA interface
   // --------------------------------
   {
      fputs ("ESTABLISH DMA INTERFACES\n", stderr);
      
      static const uint8_t DataDests[2] = {0, 1};
      bool success = construct (_dataDma, 
                                "/dev/axi_stream_dma_2",
                                "Data",
                                DataDests, 
                                sizeof (DataDests) / sizeof (*DataDests));
      if (!success)
      {
         this->close ();
         return false;
      }
   }


   // ------------------------------------------
   // Establish the TIming/Trigger DMA interface
   // ------------------------------------------
   {
      static const uint8_t TimingDests[1] = {0xff};
      bool success = construct (_timingDma, 
                                "/dev/axi_stream_dma_0",
                                "Timing",
                                TimingDests,
                                sizeof (TimingDests) / sizeof (*TimingDests));
      if (!success)
      {
         this->close ();
         return false;
      }

      fputc ('\n', stderr);
   }


   // -----------------------------------------------
   // Create queues 
   // -- None of these 4 queues are currently.  
   //    They are vestigial and should be eliminated
   // ----------------------------------------------
   _workQueue    = new CommQueue(RxFrameCount+5,true);
   _relQueue     = new CommQueue(RxFrameCount+5,true);
   _rxQueue      = new CommQueue(RxFrameCount+5,false);
   _txAckQueue   = new CommQueue(TxFrameCount+5,true);

   // ------------------------------------------------------------------
   // Making this queue as large as the total number of DMA data receive
   // buffers guarantees that a push onto it will never fail 
   _txReqQueue   = new CommQueue(_dataDma._rxCount,true);
   // -----------------------------------------------------------------




   // Create and populate the RX Queue Entries
   FrameBuffer *frames = new FrameBuffer[RxFrameCount];
   if ( frames == NULL ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to allocate FrameBuffers\n");
      this->close();
      return(false);
   }

   for ( uint32_t x=0; x < RxFrameCount; x++ ) {
      _rxQueue->push (&frames[x]);
   }


   // Create RX thread
   _rxThreadEn = true;
   if ( pthread_create(&_rxThread,NULL,rxRunRaw,(void *)this) ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to create rxThread\n");
      _rxThreadEn = false;
      this->close();
      return(false);
   }

   
   // Set priority
#if 0
   struct sched_param rxParam;
   int                 policy;
   struct timeval         cur;

   gettimeofday (&cur, NULL);
   pthread_getschedparam (_rxThread, &policy, &rxParam);
   fprintf (stderr, "Policy = %d priority = %d time = %lu.%06lu\n", 
            policy, rxParam.sched_priority, cur.tv_sec, cur.tv_usec);

   //rxParam.sched_priority = 3;
   //policy                 = SCHED_FIFO;
   //pthread_setschedparam(_rxThread, policy, &rxParam);
   //pthread_getschedparam (_rxThread, &policy, &rxParam);
   //fprintf (stderr, "Policy = %d priority = %d\n", 
   //         policy, rxParam.sched_priority);


#endif

   // Create Work Thread
   _workThreadEn = true;
   if ( pthread_create(&_workThread,NULL,workRunRaw,(void *)this) ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to create workThread\n");
      _workThreadEn = false;
      this->close();
      return(false);
   } 



   // Create TX thread
   _txThreadEn = true;
   if ( pthread_create(&_txThread,NULL,txRunRaw,(void *)this) ) {
      fprintf(stderr,"DaqBuffer::open -> Failed to create txThread\n");
      _txThreadEn = false;
      this->close();
      return(false);
   } 


   // Set priority
   txParam.sched_priority = 1;
   pthread_setschedparam(_txThread, SCHED_FIFO, &txParam);

   // Init
   resetCounters ();

   return(true);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Close and stop threads
                                                                          */
/* ---------------------------------------------------------------------- */
void DaqBuffer::close () {
   FrameBuffer *tempBuffer;

   // Disable transmit
   disableTx();

   // Stop tx thread
   if ( _txThreadEn ) {
      _txThreadEn = false;
      pthread_join(_txThread, NULL);
   }

   // Stop work thread
   if ( _workThreadEn ) {
      _workThreadEn = false;
      pthread_join(_workThread, NULL);
   }

   // Stop rx thread
   if ( _rxThreadEn ) {
      _rxThreadEn = false;
      pthread_join(_rxThread, NULL);
   }

   // Unmap user space
   _dataDma  .unmap ();
   _timingDma.unmap ();

   // Close the device
   _dataDma.close ();
   _timingDma.close ();


   // Delete Frame Queue Entries
   if ( _rxQueue   != NULL ) {
      while ( (tempBuffer = (FrameBuffer *)_rxQueue->pop()) != NULL )
         delete (tempBuffer);
      delete _rxQueue;
   }
   _rxQueue   = NULL;

   // Delete queues
   if ( _workQueue    != NULL ) delete _workQueue;
   if ( _relQueue     != NULL ) delete _relQueue;
   if ( _txReqQueue   != NULL ) delete _txReqQueue;
   if ( _txAckQueue   != NULL ) delete _txAckQueue;
   _workQueue    = NULL;
   _relQueue     = NULL;
   _txReqQueue   = NULL;
   _txAckQueue   = NULL;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief  Run the receive thread
                                                                          */
/* ---------------------------------------------------------------------- */
void * DaqBuffer::rxRunRaw (void *p) 
{
   DaqBuffer *buff = reinterpret_cast<decltype (buff)>(p);
   buff->rxRun  ();
   pthread_exit (NULL);
   return NULL;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Check the DMA buffers for readability

  \warning
   This method should be expunged when the root cause of these unreadable
   DMA buffers is found.

   Until then, this method will remove any such buffers from circulation
                                                                          */
/* ---------------------------------------------------------------------- */
void DaqDmaDevice::vet ()
{
   fputs ("BAD HOMBRE BUFFER VETTING\n", stderr);

   // ----------------------------------------------
   // !!! KUDGE !!! CHECK READBILITY OF THE BUFFERS
   // ----------------------------------------------
   uint16_t bad[_bCount];
   AxiBufChecker::check_buffers (bad, _map, _bCount, _bSize);
   // ----------------------------------------------
      
      
   // -------------------------------------------------------
   // !!! KLUDGE !!!
   // Get rid of the bad buffers
   // ------------------------------
   int nbufs     =   _bCount;
   int nrxbufs   =   _rxCount;
   AxiBufChecker abc[_bCount];


   /// !!! KLUDGE !!!
   /// REDUCE BUFFERS BY ONE TO TEST IDEA THAT 
   /// WHEN 2 DESTS ARE ENABLED, THE LAST BUFFER
   /// CANNOT BE ALLOCATED
   ///
   /// This appears to be the case, so will leave this in temporarily.  
   ///
   /// !!! NOTE !!!
   /// If the bad buffer is this last buffer, then have lost the 
   /// protection afforded by this routine.
   //
   nrxbufs -= 1;

   for (int idx = 0; idx < 2; idx++)
   {

      int nbad = AxiBufChecker::extreme_vetting (abc, 
                                                 _fd,
                                                 idx, 
                                                 _map, 
                                                 nrxbufs,
                                                 nbufs, 
                                                 _bSize);
      nrxbufs -= nbad;
   }

   fputc ('\n', stderr);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Hokey routine to get the timestamp range of the packet.
  \return  The  timestamp of the initial WIB frame

  \param[in] range The timestamp range of this packet
  \param[in]  data The data frame

                                                                          */
/* ---------------------------------------------------------------------- */
static inline void getTimestampRange (uint64_t range[2],
                                      uint8_t const *data,
                                      uint32_t nbytes)
{
   uint64_t const *d64 =   reinterpret_cast<uint64_t const *>(data);

   // Skip header
   d64 += 1;
   uint64_t  begin = d64[1];


   d64 += nbytes/sizeof (uint64_t) - 1 - 30 - 1;
   uint64_t end = d64[1] + TimingClockTicks::PER_SAMPLE;

   //fprintf (stderr,
   //        "Beg %16.16" PRIx64 " End %16.16" PRIx64 " %16.16" PRIx64 
   //        " %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 "\n",
   //         begin, end, d64[-1], d64[0], d64[1], d64[2]);

   range[0] = begin;
   range[1] =   end;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static bool process (Event::Trigger    &trigger,
                     DaqDmaDevice    &timingDma,
                     int                blowOff,
                     enum RunMode       runMode,
                     bool              isActive,
                     volatile uint32_t  &trgCnt,
                     volatile uint32_t  &trgMsgCnt,
                     volatile uint32_t  &disTrgCnt,
                     MonitorRate          &rate,
                     FrameDiagnostics     &diag)
{
   uint32_t rxSize;
   uint32_t  index;
   uint32_t  flags;
   uint32_t   dest;
   bool  trgActive = false;

   rxSize = dmaReadIndex (timingDma._fd, &index, &flags, NULL, &dest);

   rate.monitor       (2);
   diag.dump_received (index, 2, rxSize);


   // ------------------------------------
   // Check if should process this message
   // ------------------------------------
   if ( (!blowOff                    ) &&
        (runMode == RunMode::EXTERNAL) &&
        (dest    == 0xff)            )
   {
      TimingMsg *tmsg = TimingMsg::from (timingDma._map[index]);
      diag.dump_timingFrame ((uint64_t const *)timingDma._map[index], 
                             rxSize);

      // Count external triggers
      trgCnt += 1;
               
      // Is this a trigger message?
      if (tmsg->is_trigger ())
      {
         trgMsgCnt += 1;
         fprintf (stderr, "Trigger msg ts = %16.16" PRIx64 " [discard=%d]\n",
                 tmsg->timestamp(), isActive);


         float dt = float(tmsg->timestamp() - trigger.m_timestamp);
         dt *= 1.e3 / 250.e6;
         fprintf(stderr, "Last accepted trigger ts = %16.16" PRIx64
                 ", dt = %.1f ms \n",
                 trigger.m_timestamp, dt);

         if (!isActive)
         {
            trigger.init (tmsg);
         }
         else
         {
            fputs ("Discarding trigger\n", stderr);
	          disTrgCnt += 1;
         }
         trgActive = true;
      }
   }

   // Always free the trigger message
   timingDma.free (index);

   return trgActive;
}
/* ---------------------------------------------------------------------- */ 


/* ---------------------------------------------------------------------- */ 
static inline uint64_t subtimeofday (struct timeval const *cur, 
                                     struct timeval const *prv)
{
   uint64_t  sec = cur->tv_sec  - prv->tv_sec;
   int32_t  usec = cur->tv_usec - prv->tv_usec;

   if (usec < 0)
   {
      usec += 1000 * 1000;
      sec  -= 1;
   }

   return sec * (1000 * 1000) + usec;
}
/* ---------------------------------------------------------------------- */ 


#include <limits.h>

// Class method for rx thread running
void DaqBuffer::rxRun ()
{
   // --------------------------------------------------------
   // Check for bad DMA buffers
   // This cannot be done in the initialization thread because
   // it blocks if the dma channel has not been setup to have 
   // data flowing. Of course, the thread that fields command
   // to enable data flowing is also blocked, so deadlock.
   // --------------------------------------------------------
    _dataDma.vet ();


   // Hardwire trigger window to 5 msecs and convet to equivalent 
   // number of clock ticks
   fd_set                    fds;
   bool                trgActive;

   trgActive = false;
   FD_ZERO (&fds);

   LatencyList latency[MAX_DEST];


   uint32_t      softTriggerCnt;

   FrameDiagnostics diag (-1, -1, -1, -1);
   _rxPend = 0;   


   // -------------------------------------------------------------------
   // Request array of events, one for every possible trigger
   // Given that 
   //   1. there can't be more triggers than front-end frame buffers
   //   2. event buffers are cheap
   //
   // one event buffer will be allocated for every front-end frame buffer
   // -------------------------------------------------------------------
   Event *events = 
      reinterpret_cast<decltype(events)>(malloc (_dataDma._bCount * sizeof (*events)));


   // Allocate enough frame buffer nodes to capture an all possible events
   List<FrameBuffer >::Node *fbs = 
      reinterpret_cast<decltype (fbs)>(malloc (_dataDma._bCount * sizeof (*fbs)));
   for (decltype(_dataDma._bCount)(idx) = 0; idx < _dataDma._bCount; idx++)
   {
      // Initialize the one time only fields
      fbs[idx].m_body.setData  (_dataDma._map[idx]);
      fbs[idx].m_body.setIndex (idx);
      events[idx].init (idx);
   }

   fputs ("STARTING\n", stderr);

   // Debugging aid for monitoring the rates of various dest DMA buffers
   MonitorRate       rate;
   Event::Trigger trigger;
   Event           *event = NULL;

   FD_ZERO (&fds);


   int32_t fdMax = ((_dataDma._fd > _timingDma._fd)
                 ?   _dataDma._fd 
                 :   _timingDma._fd) 
                 + 1;

   // Run while enabled
   while (_rxThreadEn) 
   {

      //struct timeval timeout;   
      //timeout.tv_sec  = 0;
      //timeout.tv_usec = WaitTime;

      int timingFd = _timingDma._fd;
      int   dataFd =   _dataDma._fd;

      FD_SET  (  dataFd, &fds);
      FD_SET  (timingFd, &fds);

      // ---------------------------------------------------
      // With the event being freed in the destination queue
      // there is no reason anymore for a timeout.
      // ---------------------------------------------------
      int nfds = select (fdMax, &fds, NULL, NULL, NULL); 
      if (nfds > 0)
      {
         // --------------------------------------------
         // Give priority to the timing/trigger messages
         // --------------------------------------------
         if (FD_ISSET (timingFd, &fds))
         {
            trgActive = process (trigger, 
                                 _timingDma,
                                 _config._blowOffDmaData,
                                 _config._runMode,
                                 trgActive,
                                 _counters._triggers,
                                 _counters._trgMsgCnt,
                                 _counters._disTrgCnt,
                                 rate,
                                 diag);
         }


         // ----------------------
         // Check on incoming data
         // ----------------------
         if (FD_ISSET (dataFd, &fds))
         {
            uint32_t         index;
            uint32_t         flags;
            uint32_t          dest;
            int32_t         rxSize;

            rxSize = dmaReadIndex (dataFd, &index, &flags, NULL, &dest);

#if         0
            static int      Count = 0;
            static struct timeval prv;
            struct timeval        cur;

            gettimeofday (&cur, NULL);
            int64_t elapsed = subtimeofday (&cur, &prv);
            Count++;
            if (elapsed > 1 * 1000)
            {

               static int64_t Largest = 0;
               static bool      First = true;
               fprintf (stderr,
                        "Large elapsed time[%6u] = %10" PRId64 "  %10" PRId64 "\n", 
                        Count, elapsed, Largest);
               if (elapsed > Largest) 
               {
                  if (!First)
                  {
                     Largest = elapsed;
                  }

                  First = false;
               }


            }
            prv = cur;
#endif

            rate.monitor       (dest);
            diag.dump_received (index, dest, rxSize);


            if (_config._blowOffDmaData || dest > (MAX_DEST - 1))
            {
               _dataDma.free (index);
               continue;
            }


            _counters._rxCount += 1;
            _counters._rxTotal += rxSize;
            _rxSequence        += 1;
            _rxSize             = rxSize; 


            // -----------------------------------------------
            // Check every 256 received packets in each stream
            // -----------------------------------------------
            diag.check_dataFrame ((uint64_t const *)_dataDma._map[index],
                                  rxSize,
                                  dest,
                                  256);

            diag.dump_dataFrame (_dataDma._map[index], rxSize, dest);

            {
               static uint64_t NextTimestamp[2] = { 0, 0 };

               uint64_t  exp  = NextTimestamp[dest];
               uint64_t *data = reinterpret_cast<decltype(data)>
                                           (_dataDma._map[index]);
               uint64_t  got = data[2];
               uint64_t diff = got - exp;

               if (diff != 0)
               {
                  static int ErrorCnt[2] = { 0, 0};
                  if (got != exp && exp != 0)
                  {
                     fprintf (stderr, 
                              "Error[%1d.%4u] = %16.16" PRIx64 " != "
                              "%16.16" PRIx64 " diff = %10" PRId64 ""
                              " ------------ *\n",
                              dest, ErrorCnt[dest]++, got, exp, diff);
                     ///if (ErrorCnt[dest] > 1) exit (-1);

                  }
               }

               NextTimestamp[dest] = got + 1024 * 25;
            }


               


            // Check if there is error
            uint32_t lastUser = axisGetLuser (flags);

            /// !!! KLUDGE !!!
            if (lastUser & TUserEOFE)
            {
               _counters._rxErrors++;
               _dataDma.free (index);
               continue;

            // Try to Move the data
            } else {

               uint8_t  *data = _dataDma._map[index];
               uint64_t *dbeg = reinterpret_cast<decltype(dbeg)>(data);
               uint64_t *dend = reinterpret_cast<decltype(dbeg)>(data +
                                                              + rxSize
                                                              - sizeof (*dend));

               uint32_t tlrSize = *reinterpret_cast<uint32_t const *>(dend) 
                                & 0xffffff;

               if (tlrSize != _rxSize)
               {
                  fprintf (stderr, 
                           "Error Frame %16.16" PRIx64 " -> " 
                           "%16.16" PRIx64 " %8.8x" PRIx32 "\n", 
                           *dbeg, *dend, _rxSize);

                  _counters._rxErrors++;
                  _dataDma.free (index);
                  continue;
               }

               // Patch in the length, sans the trailer
               uint32_t nbytes = rxSize - sizeof (uint64_t);
               dbeg[0] |= nbytes;


               uint64_t timestampRange[2];
               auto fb = &fbs[index];
               getTimestampRange (timestampRange, _dataDma._map[index], rxSize);
               fb->m_body.setTimeRange  (timestampRange[0],
                                         timestampRange[1]);

               // Set the size, sans the trailer
               fb->m_body.setSize       (nbytes);
               fb->m_body.setRxSequence (_rxSequence++);
            
                                  
               if (_config._runMode == RunMode::SOFTWARE && 
                   trgActive        == false)
               {
                  // Check if this is a soft trigger
                  int64_t triggerTime = SoftTrigger::check (timestampRange, 
                                                            _config._period);

                  //fprintf (stderr, "Software trigger active = %d\n", accepted);
                  
                  if (triggerTime >= 0)
                  {
                     event = &events[index];
                     event->m_trigger.init (triggerTime,
                                            softTriggerCnt++, 0); 
                     event->setWindow (triggerTime - _config._pretrigger,
                                       triggerTime + _config._posttrigger);
                     event->seedAndDrain (latency, dataFd);
                     trgActive = true;
                  }
               }
               else if (_config._runMode == RunMode::EXTERNAL && 
                        trgActive                             &&
                        event == NULL)
               {
                  event            = &events[index];
                  event->m_trigger = trigger;
                  event->setWindow (trigger.m_timestamp - _config._pretrigger,
                                    trigger.m_timestamp + _config._posttrigger);
                  bool found = event->seedAndDrain (latency, dataFd);

                  static int64_t MaxLatency = LLONG_MIN;
                  static int64_t MinLatency = LLONG_MAX;
                  int64_t diff = trigger.m_timestamp - fb->m_body._ts_range[0];
                  if (diff > MaxLatency) MaxLatency = diff;
                  if (diff < MinLatency) MinLatency = diff;
                  fprintf (stderr,
                           "Latency [%16" PRId64 ":%16" PRId64 "] time = "
                           " %16.16" PRIx64 "found = %d\n",
                           MinLatency, MaxLatency, fb->m_body._ts_range[0], 
                           found);



                  // ------------------------------------------------------
                  // The latency queue had no members in the trigger window
                  // Place this frame on the latency queue and wait.
                  // ------------------------------------------------------
                  if (found == false)
                  {
                     fprintf (stderr, "Waiting for frame within time window\n");
                     event = NULL;
                     latency[dest].replace (fb, dataFd);
                     fprintf (stderr, "Remaining = %d\n", 
                              latency[dest].m_remaining);
                     continue;
                  }
               }

               bool replace = true;
               if (trgActive)
               {
                  Event::Fate fate = event->add (fb, dest);

                  if (fate == Event::Added)
                  {
                     continue;
                  }
                  else if (fate == Event::TooEarly)
                  {
                     _dataDma.free (index);
                     continue;
                  }
                  else if (fate > 0)
                  {
                     static int CoCnt[3] = { 0, 0, 0};

                     // -------------------------------------------------------
                     // This was an accepted event because either
                     //   Completed -- All contributions present and 
                     //                accounted for
                     //   Overrun   -- The time of at least one contributor was
                     //                already completed.  Basically the other
                     //                contribution failed to show up.
                     //   CompleteOverrun
                     //             -- The event completed because an existing
                     //                contributor did not cleanly complete
                     //                the event.  The event is declared 
                     //                completed, and this packet pitched.
                     //                This is same action as the Overrun,
                     //                but is catagorized differently for
                     //                accounting and reporting purposes.
                     // -------------------------------------------------------

                     // Get the list of contributors
                     uint32_t ctbs = ((1 << MAX_DEST) - 1) & ~event->m_ctbs;
                     event->m_ctbs = ctbs;

                     _txReqQueue->push (event);
                     trgActive   = false;
                     event       = NULL;

                     // --------------------------------
                     // Reset the latency lists to empty
                     // This is okay since triggers are
                     // prohibited from overlappping.
                     // --------------------------------
                     while (ctbs)
                     {
                        uint32_t ctb = ffsl (ctbs) - 1;
                        latency[ctb].resetToEmpty ();
                        ctbs  &= ~(1 << ctb);
                     }


                     if (fate == Event::Completed) 
                     { 
                        CoCnt[0]++; 
                        replace = false;
                     }
                     else if (fate == Event::CompletedOverrun) 
                     { 
                        CoCnt[2]++;
                     }
                     else
                     {
                        CoCnt[1]++;
                     }
                    
                     ///fprintf (stderr, "Completed %d:%5d:%5d\n",
                     ///         CoCnt[0], CoCnt[1], CoCnt[2]);
                  }
               }

               if (replace)
               {
                  ///fprintf (stderr, "Latency[%d] @ %p fbs[%d] @ %p\n", 
                  ///           dest, &latency[dest], index, fb);
                  latency[dest].replace (fb, dataFd);
               }
            }
         }
      }
   }

   return;
}

// Static work thread run
void * DaqBuffer::workRunRaw ( void *p ) {
   DaqBuffer *buff = (DaqBuffer *)p;
   buff->workRun();
   pthread_exit(NULL);
   return(NULL);
}

// Class method for work thread
void DaqBuffer::workRun () {
   FrameBuffer * tempBuffer;
   FrameBuffer * txBuffer;
   _txPend      = 0;



   // Run while enabled
   while (_workThreadEn) {

      // Process TX ack queue, wait on it if we are saturated
      while ( (tempBuffer = (FrameBuffer *)_txAckQueue->pop((_txPend == TxFrameCount)?WaitTime:0)) != NULL ) {
         _relQueue->push(tempBuffer);
         _txPend--;
      }

      // Go back around if we are saturated
      if ( _txPend == TxFrameCount ) continue;

      // Wait for an entry in pend buffer
      if ( (tempBuffer = (FrameBuffer *)_workQueue->pop(WaitTime)) == NULL) continue;

      // Custom code here
      txBuffer = tempBuffer;     
      _txReqQueue->push(txBuffer);
      _txPend++;
   }
}

// Static raw tx thread run
void * DaqBuffer::txRunRaw ( void *p ) {
   DaqBuffer *buff = (DaqBuffer *)p;
   buff->txRun();
   pthread_exit(NULL);
   return(NULL);
}



#define DUMPS 0
#if DUMPS
/* ---------------------------------------------------------------------- *//*!

  \brief Dump the contents of a generic Header0 

  \param[in]  header0  The header0 structure to dump
                                                                          */
/* ---------------------------------------------------------------------- */
static void header0_dump (pdd::Header0 const & header0)
{
   using namespace pdd;
   static const char *TypeNames[] =
   {
      [(int)Fragment::Type::Reserved_0   ] = "Reserved_0",
      [(int)Fragment::Type::Control      ] = "Control",
      [(int)Fragment::Type::Data         ] = "Data",
      [(int)Fragment::Type::MonitorSync  ] = "MonitorSync",
      [(int)Fragment::Type::MonitorUnSync] = "MonitorUnSync"
   };

   static const char *RecType[] =
   {
    [(int)Fragment::Header<Fragment::Type::Data>::RecType::Reserved_0] = "Reserved_0",
    [(int)Fragment::Header<Fragment::Type::Data>::RecType::Originator] = "Originator",
    [(int)Fragment::Header<Fragment::Type::Data>::RecType::TpcNormal ] = "TpcNorma",

    [(int)Fragment::Header<Fragment::Type::Data>::RecType::TpcDamaged] = "TocDamaged"
   };



   uint64_t  w64 = header0.m_w64;
   unsigned int    format = header0.format  (w64);
   unsigned int      type = header0.type    (w64);
   unsigned int    length = header0.length  (w64);
   unsigned int   subtype = header0.subtype (w64);
   unsigned int    naux64 = header0.naux64  (w64);
   unsigned int    bridge = header0.bridge  (w64);

   char const *typeName = type < sizeof (TypeNames) / sizeof (TypeNames[0])
                        ? TypeNames[type]
                        : (char const *)"Unknown Type";

   char const *recName;
   if (type == (int)Fragment::Type::Data)
   {
      recName  = (subtype < sizeof (RecType) / sizeof (RecType[0]))
               ? RecType[subtype] 
               : "UnknownDataType";
   }
   else
   {
      recName = "NotDefinedYet";
   }
               


   fprintf (stderr, "Header0: %16s.%16s\n"
                    "        format =%1.1x  type  =%1.1x length:%6.6x\n"
                    "        subtype=%1.1x  naux64=%1.1x bridge:%6.6x\n",
            typeName,
            recName,
            format,
            type,
            length,
            subtype,
            naux64,
            bridge);

   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief Dump the contents of an Originator structure

  \param[in] originator originator
                                                                          */
/* ---------------------------------------------------------------------- */
static void originator_dump (pdd::Fragment::Originator const &originator)
{
   using namespace pdd;


   {
      Header2  const &header = originator;
      uint32_t     m_w32   = header.m_w32;
      unsigned int type    = header.type    (m_w32);
      unsigned int version = header.bridge  (m_w32);
      unsigned int    n64  = header.n64     (m_w32);

      fprintf (stderr, "Origin: type=%1.1x version=%1.1x n64=%6.6x\n",
               type,
               version,
               n64);
   }


   {
      Fragment::OriginatorBody const &body   = HeaderOrigin.m_origin.m_body;
      
      Fragment::Version const &version = body.version   ();
      char const             *rptSwTag = body.rptSwTag  ();
      uint64_t            serialNumber = body.serialNumber ();
      uint32_t                location = body.location     ();
      char const            *groupName = body.groupName ();
      unsigned                    slot = (location >> 16) & 0xff;
      unsigned                     bay = (location >>  8) & 0xff;
      unsigned                 element = (location >>  0) & 0xff;

      uint32_t software = version.software ();
      uint8_t     major = (software >> 24) & 0xff;
      uint8_t     minor = (software >> 16) & 0xff;
      uint8_t     patch = (software >>  8) & 0xff;
      uint8_t   release = (software >>  0) & 0xff;

      fprintf (stderr,
               "        Software ="
               " %2.2" PRIx8 ".%2.2" PRIx8 ".%2.2" PRIx8 ".%2.2" PRIx8 ""
               " Firmware = %8.8" PRIx32 "\n"
               "        RptTag   = %s\n"
               "        Serial # = %16.16" PRIx64 "\n"
               "        Location = %s/%u/%u/%u\n",
               major, minor, patch, release,
               version.firmware (),
               rptSwTag,
               serialNumber,
               groupName, slot, bay, element);
   }

   

   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
static void fragment_dump (Event const *event)
{
   for (int idx = 0; idx < MAX_DEST; idx++)
   {
       List<FrameBuffer>      const     *list = &event->m_list[idx];
      List<FrameBuffer>::Node const     *node = list->m_flnk;
      List<FrameBuffer>::Node const *terminal = list->terminal();
      
      // ----------------------
      // Test for an empty list
      // ----------------------
      if (node == terminal) 
      {
         break;
      }

      int       count = 0;
      do
      {
         uint32_t      nbytes = node->m_body.size     ();
         uint8_t         *ptr = node->m_body.baseAddr ();
         unsigned int   index = node->m_body.index    ();

         fprintf (stderr,
                  "Fragment[%1d.%2d] = ptr:size: %p:%8.8x" PRIx32 "index: %3u\n",
                  idx, count, ptr, nbytes, index);

         AxiBufChecker x;
         int iss = x.check_buffer (ptr, index, nbytes);
         if (iss)
         {
            fprintf (stderr,
                     "Fragment[%1d.%2d] is bad\n",
                     idx, count);
         }

         count += 1;
         
         if (count > 15) break;
         
         node = node->m_flnk;
      }
      while (node != terminal);
      
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void node_dump (List<FrameBuffer> const *list,
                       List<FrameBuffer>::Node const *node,
                       int ictb,
                       int toc_idx,
                       uint32_t nbytes)
{
                                   
   int  delta = node->m_body._ts_range[1] 
              - node->m_body._ts_range[0];
            
   fprintf (stderr, 
            "Node[%2d.%2d] %p flnk:%p blnk:%p end: %12.12" 
            PRIx64 "->%12.12" PRIx64 " diff: %6d index: %3d size: %8.8x\n",
            ictb,
            toc_idx,
            node, 
            list->m_flnk,
            list->m_blnk,
            node->m_body._ts_range[0],
            node->m_body._ts_range[1],
            delta,
            node->m_body.index (),
            nbytes);

   return;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define header0_dump(_header0)
#define originator_dump(__originator)
#define fragment_dump(_header)
#define node_dump (_list, _node, _ictb, _toc_idx, _nbyte)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

   \brief  Waits on incoming data to be transmitted to a client machine

                                                                          */
/* ---------------------------------------------------------------------- */
void DaqBuffer::txRun () 
{
   struct msghdr       msg;
   struct iovec        msg_iov[32];

   using namespace pdd;

   #define MAX_CONTRIBUTORS  MAX_DEST
   #define MAX_PACKETS      (MAX_DEST) * 32

   // --------------------------------------------------------------
   // Provide the storage for the Data record. This includes the 
   //    1. The Data header
   //    2. The Table of Contents 
   //       -- Sized for MAX_CONTRIBUTORS and a total of MAX_PACKETS
   //    3. The Packet Header
   // --------------------------------------------------------------
   uint8_t dataRecord[sizeof (Fragment::Tpc::Data) 
                     + sizeof (Fragment::Tpc::Toc<MAX_CONTRIBUTORS, MAX_PACKETS>)
                     + sizeof (Fragment::Tpc::Packet)] __attribute__ ((aligned (8)));
   Fragment::Tpc::Data  
             *tpcData = reinterpret_cast<Fragment::Tpc::Data *>(dataRecord);

   Fragment::Tpc::Toc<MAX_CONTRIBUTORS, MAX_PACKETS> 
                 &toc = reinterpret_cast<Fragment::Tpc::Toc<MAX_CONTRIBUTORS, MAX_PACKETS> &>(tpcData[1]);
   
      

   // --------------------
   // Setup message header
   // --------------------
   msg.msg_name       = &_txServerAddr;
   msg.msg_namelen    = sizeof(struct sockaddr_in);
   msg.msg_iov        = msg_iov;
   msg.msg_iovlen     = 2;
   msg.msg_control    = NULL;
   msg.msg_controllen = 0;
   msg.msg_flags      = 0;


   // ------------------------------------
   // Initialization the static records in
   // their respective iov's
   // ------------------------------------

   // --------------------------------------------
   // -- Fragment header + Origination Information
   // --------------------------------------------
   msg_iov[0].iov_base = &HeaderOrigin;
   msg_iov[0].iov_len  =  HeaderOrigin.n64 () * sizeof (uint64_t);


   // ---------------------------
   // Wait on the incoming frames
   // ---------------------------
   while ( _txThreadEn ) 
   {
      // Wait for data in the pend buffer
      Event *event = reinterpret_cast<decltype (event)>
                    (_txReqQueue->pop(WaitTime));

      if (event == NULL) 
      {
         continue;
      }

      // -----------------------------------
      // The Tpc Data Records start at iov 2
      // -----------------------------------
      msg.msg_iovlen  = 2;
      uint32_t ndata  = 0;

      uint16_t    srcs[2];


      int         nctbs = event->m_nctbs;
      unsigned int ctbs = event->m_ctbs;
      int       ctb_idx = 0;
      int       pkt_idx = 0;

      auto   toc_pkt    = toc.packets (nctbs);
      auto   toc_ctb    = toc.contributors ();
      int    toc_offset = 0;


      // -------------------------------
      // !!! KLUDGE -- for emulated data
      // -------------------------------
      static const uint16_t Csfs[2] = 
      {
         (0x1 << 6) | (0x2 << 3) | (0x3 << 0),
         (0xa << 5) | (0x6 << 3) | (0x7 << 0),
      };


      //fprintf (stderr, "Got Event %p ctbs:%8.8x nctbs:%d\n", event, ctbs, nctbs);

      // ------------------------------------------------
      // Capture the frames from each incoming HLS source
      // ------------------------------------------------
      while (ctbs)
      {
         int                ictb = ffs (ctbs) - 1;
         List<FrameBuffer> *list = &event->m_list[ictb];
         List<FrameBuffer>::Node           *node = list->m_flnk;
         List<FrameBuffer>::Node const *terminal = list->terminal ();
         ctbs &= ~(1 << ictb);

         int npkts = 0;

         // ---------------------------------------------
         // Get the crate.slot.fiber for this contributor
         // ---------------------------------------------
         ////// !!!! srcs[ictb] = node->m_body.getCsf ();
         uint16_t csf = srcs[ictb] = Csfs[ictb];


         // ---------------------------------------------------------------
         // Fill in the csf, packet count and offset to the first TOC entry
         // for this contributor.
         // ---------------------------------------------------------------
         toc_ctb[ctb_idx].construct (csf, event->m_npkts[ictb], toc_offset);


         // -----------------------------------------------------
         // Grab all the packets associated with this contributor
         // -----------------------------------------------------
         do
         {
            uint32_t      nbytes = node->m_body.size       () - sizeof (uint64_t);
            uint64_t        *p64 = node->m_body.baseAddr64 ();

            uint64_t  dataHeader = p64[0];
            int       dataFormat = (dataHeader >> 24) & 0xf;


            // ------------------------------------------------------
            // Setup buffer pointers and size
            // The data header from the HLS stream is not transported.
            // All the relevant informatin is captured in the TOC
            // ------------------------------------------------------
            msg_iov[msg.msg_iovlen].iov_base = p64 + 1;
            msg_iov[msg.msg_iovlen].iov_len  = nbytes;

            /*
            fprintf (stderr, 
                     "Pkt[%2d] = %16.16" PRIx64 " %16.16" PRIx64 " "
                     "%16.16" PRIx64 " %16.16" PRIx64 "\n",
                     pkt_idx, p64[0], p64[1], p64[2], p64[3]);
            */

            msg.msg_iovlen++;


            // -----------------------------------------------------
            // Construct the table of contents entry for this packet
            // -----------------------------------------------------
            toc_pkt[pkt_idx].construct (dataFormat, 
                                        0, ndata/sizeof (uint64_t));

            ///node_dump (list, node, ictb, pkt_idx, nbytes);


            pkt_idx += 1;             // Bump packet index
            npkts   += 1;             // Bump number packets, this contributor
            ndata   += nbytes;        // Running count of data size
            node     = node->m_flnk;  // Next node

         }
         while (node != terminal);

         /*
         fprintf (stderr, "Event:packet count[%d] = %d vs %d\n",
                  ictb, event->m_npkts[ictb], npkts);
         */


         // ----------------------------------------------------------
         // Beginning offset to the TOC entry for the next contributor
         // ----------------------------------------------------------
         toc_offset += pkt_idx;
         ctb_idx    += 1;
      }


      // ---------------------------------------
      // Add the terminating index so the length
      // of the final frame can be determined.
      // ---------------------------------------
      toc_pkt[pkt_idx++].construct  (0, 0, ndata/sizeof (uint64_t));


      ///fprintf (stderr, "Srcs %4.4" PRIx16 " %4.4" PRIx16 " %8.8" PRIx32 "\n", 
      ///         srcs[0], srcs[1], ndata);


      // --------------------------------------------------
      // Complete the construction of the table of contents
      // First compute its total size, in bytes
      // --------------------------------------------------
      uint32_t toc_nbytes   = toc.nbytes   (ctb_idx + pkt_idx);
      uint32_t toc_n64bytes = toc.n64bytes (ctb_idx + pkt_idx);

      /*
      fprintf (stderr,
               "Toc:nctbs=%2d npkts=%2d nbytes=%4d n64=%4d\n",
               ctb_idx, pkt_idx, toc_nbytes, toc_n64bytes);
      */
      toc.construct (nctbs, toc_n64bytes/sizeof (uint64_t));




      // -------------------------------------------
      // Pad to a 64-bit boundary
      // -------------------------------------------
      uint32_t   *p32 = reinterpret_cast<uint32_t *>((char *)&toc + toc_nbytes);
      if ( (toc_nbytes & 0x7) != 0)
      {
         // ----------------------------------------
         // Not on an even 64-bit boundary, 
         // Add a padding word and round up size up
         // ----------------------------------------

         p32[0]  = 0;
         p32    += 1;
         toc_nbytes += sizeof (p32[0]);
      }


      // --------------------------------
      // Add the data packet header
      // This immediately follows the TOC
      // --------------------------------
      void                   *pkt_ptr = reinterpret_cast<char *>(&toc) + toc_nbytes;
      pdd::Fragment::Tpc::Packet *pkt = reinterpret_cast<decltype (pkt)>(pkt_ptr);
      pkt->construct ((ndata + sizeof (uint64_t) - 1)/sizeof (uint64_t));


      // -------------------------
      // Construct the data header
      // ------------------------
      static int DataType  = static_cast<decltype (DataType)>
                          (Fragment::Header<Fragment::Type::Data>::RecType::TpcNormal);

      ndata += sizeof (Fragment::Tpc::Packet);
      tpcData->construct (DataType, 
                          (sizeof (Fragment::Tpc::Packet) + ndata)/sizeof (uint64_t),
                          0);

      msg_iov[1].iov_base = dataRecord;
      msg_iov[1].iov_len  = sizeof (Fragment::Tpc::Data) 
                          + toc_n64bytes 
                          + sizeof (Fragment::Tpc::Packet);

#if   0
      {
         uint32_t const *p = (uint32_t const *)&toc;
         for (unsigned i = 0; i < (toc_nbytes + 3) / 4 + 3; i++)
         {
            fprintf (stderr,
                     "toc[%2u] = %8.8" PRIx32 " %p:%p\n", i, p[i], &p[i], pkt);
         }
      }
#endif




 
      // -----------------------------------------------------
      // Add the size of the table of contents and the trailer
      // to the transmitted count.
      // -----------------------------------------------------
      uint32_t txSize = HeaderOrigin.n64 () * sizeof (uint64_t)
                      + toc_nbytes 
                      + 8
                      + ndata
                      + sizeof (Trailer);
      /*
     fprintf (stderr,
               "Header  %8.8x\n"
               "Toc     %8.8x\n"
               "DataH   %8.8x\n"
               "Data    %8.8x\n"
               "Trailer %8.8x\n"
               "Total   %8.8x\n",
               (unsigned)HeaderOrigin.n64 (),
               (unsigned)toc_nbytes,
              (unsigned)8,
               (unsigned)ndata,
               (unsigned)sizeof (Trailer),
               txSize);
      */



      // -----------------------------------
      // Construct the event fragment header
      // -----------------------------------
      HeaderOrigin.m_header.construct
                          (Fragment::Header<Fragment::Type::Data>::RecType::TpcNormal,
                           event->m_trigger.m_source,
                           srcs[0], 
                           srcs[1],
                           event->m_trigger.m_sequence,
                           event->m_trigger.m_timestamp,
                           txSize / sizeof (uint64_t));
      /*
      uint64_t *p = (uint64_t *)&HeaderOrigin;
      fprintf (stderr, 
               "Header = %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 "\n",
               p[0],
               p[1],
               p[2]);
      */
      header0_dump    (HeaderOrigin.m_header);
      originator_dump (HeaderOrigin.m_origin);

      fprintf (stderr, "Header.m_64 %16.16" PRIx64 " "
                  "Identifier.m_64 = %16.16" PRIx64 " ts = %16.16" PRIx64 " "
                  "Npackets = %3u\n",
               HeaderOrigin.m_header.m_w64,
               HeaderOrigin.m_header.m_identifier.m_w64,
               HeaderOrigin.m_header.m_identifier.m_timestamp,
               pkt_idx - 1);


      // ---------------------------------------
      // Construct the event fragment trailer
      // The last 64-bit word, containing the
      // firmware trailer is repurposed for this
      // ---------------------------------------
      Trailer *trailer = reinterpret_cast<decltype(trailer)>
                        (reinterpret_cast<char *>(msg_iov[msg.msg_iovlen-1].iov_base) 
                                                + msg_iov[msg.msg_iovlen-1].iov_len);
      trailer->construct (HeaderOrigin.m_header.retrieve ());
      msg_iov[msg.msg_iovlen - 1].iov_len += sizeof (*trailer);

                                                  
#     if 1
      unsigned tot = 0;
      for (unsigned int idx = 0; idx < msg.msg_iovlen; idx++)
      {

         int             len = msg_iov[idx].iov_len;
         uint64_t const *p64 = reinterpret_cast<decltype (p64)>
                              (msg_iov[idx].iov_base);

         tot += len;
         
         /*
         fprintf (stderr, 
                  "Iov[%2d.%4u:%6.6x] = %16.16" PRIx64 " %16.16" PRIx64 " "
                  "%16.16" PRIx64 " %16.16" PRIx64 "\n",
                  idx, len, tot, p64[0], p64[1], p64[2], p64[3]);
         */

         /*
         void *ptr = msg_iov[idx].iov_base;
         fprintf (stderr,
                  "msg[%2d] %p:%8.8x %8.8x\n", idx, ptr, len, tot);
         */
      }      
#     endif

                           

      int disconnect_wait = 10;
      if (_config._blowOffTxEth == 0)
      {
         ssize_t ret;
         if ( (_txFd < 0)  || 
              (ret = sendmsg(_txFd,&msg,0) != (int32_t)txSize)) 
         {
            if (_txFd >= 0)
            {
               /// Kill the ethernet 
               _config._blowOffTxEth = 1;

               fprintf (stderr,
                        "Have error, waiting %d seconds to disconnect\n",
                        disconnect_wait);

               fprintf (stderr, 
                        "sendmsg: %zu errno: %d\n"
                        "Header; %8.8" PRIx32 "\n"
                        "msg_iovlen = %2zu\n"
                        "0. iov_base: %p  iov_len: %zu\n"
                        "1. iov_base: %p  iov_len: %zu\n",
                        ret, errno,
                        txSize, 
                        msg.msg_iovlen,
                        msg_iov[0].iov_base, msg_iov[0].iov_len, 
                        msg_iov[1].iov_base, msg_iov[1].iov_len);
               _counters._txErrors++;


               for (unsigned int idx = 0; idx < msg.msg_iovlen; idx++)
               {

                  unsigned int len = msg_iov[idx].iov_len;
                  uint8_t     *ptr = (uint8_t *)msg_iov[idx].iov_base;

                
                  AxiBufChecker x;
                  int iss = x.check_buffer (ptr, idx, len);
                  if (iss)
                  {
                     fprintf (stderr,
                              "Iov[%2d] %p for %u bytes -> BAD\n",
                              idx, ptr, len);
                  }
                  else
                  {
                     fprintf (stderr,
                              "Iov[%2d] %p for %u bytes -> okay\n",
                              idx, ptr, len);
                  }
               }      



               fragment_dump (event);

               ret = sendmsg(_txFd,&msg,0);
               if (ret != (int32_t)txSize)
               {
                  fprintf (stderr, "Resend failed %d\n", ret);
               }
               else
               {
                  fprintf (stderr, "Resend succeeded %d\n", ret);
               }

               /// --------------------------------
               ///  --- This was just for debugging
               /// --------------------------------
               ///sleep (disconnect_wait);
               ///disableTx ();
               ///fputs ("Disconnected", stderr);
               
            }
         }
         else
         {
	    size_t currSeqId = event->m_trigger.m_sequence;
            size_t deltaSeqId = currSeqId - _txSeqId;
            if (_counters._txCount > 0 && deltaSeqId > 1) {
		_counters._dropSeqCnt += deltaSeqId - 1;
	    }
	    _txSeqId = currSeqId;

            _counters._txCount++;
            _txSize             = txSize;
            _counters._txTotal += txSize;
         }
      }



      // ------------------------------------------------------------------
      // Free all the nodes of this event
      // Note: 
      // There is no 'free' of the event structure itself.  This structure
      // is directly tied one of the nodes, so until that node is freed
      // this event structure cannot be reused.
      // ------------------------------------------------------------------
      event->free (_dataDma._fd);

      // Return entry
      //_txAckQueue->push (event);
   }
}

// Get status
void DaqBuffer::getStatus(struct BufferStatus *status) {
   struct timeval currTime;
   struct timeval diffTime;
   float  rxRate;
   float  triggerRate;
   float  rxBw;
   float  txBw;
   float  txRate;
   float  interval;

   gettimeofday (&currTime, NULL);
   timersub     (&currTime, &_lastTime, &diffTime); 

   interval = (float)diffTime.tv_sec + ((float)diffTime.tv_usec / 1000000.0);

   // 
   // 2016.09.15 -- jjr
   // -----------------
   // The original rate calculation assumed fixed size receive packets
   // The rxRate is in Hz.
   // The rxBw   is in MBits/sec
   //
   rxRate = (_counters._rxCount - _last_counters._rxCount) / interval;
   rxBw   = ((((float)(_counters._rxTotal - _last_counters._rxTotal)) / interval) * 8.0) / 1e6;


   triggerRate = ((float)(_counters._triggers - _last_counters._triggers)) / interval;

   // txRate is in Hz
   // txBw   in in Mbits/sec
   //
   txRate = (_counters._txCount - _last_counters._txCount) / interval;
   txBw   = ((((float)(_counters._txTotal - _last_counters._txTotal)) / interval) * 8.0) / 1e6;


   // These variables are static or updated every received packets
   status->buffCount    = _dataDma._bCount;
   status->rxSize       = _rxSize;
   status->rxPend       = _rxPend;
   status->txSize       = _txSize;
   status->txPend       = _txPend;


   // These are the accumulated counters
   status->rxCount      = _counters._rxCount;
   status->rxErrors     = _counters._rxErrors;
   status->dropCount    = _counters._dropCount; 
   status->triggers     = _counters._triggers;
   status->txCount      = _counters._txCount;
   status->txErrors     = _counters._txErrors;
   status->disTrgCnt    = _counters._disTrgCnt;
   status->dropSeqCnt   = _counters._dropSeqCnt;
   status->trgMsgCnt    = _counters._trgMsgCnt;

   // These are derived rates
   status->triggerRate  = triggerRate;
   status->rxRate       = rxRate;
   status->rxBw         = rxBw;
   status->txRate       = txRate;
   status->txBw         = txBw;


   // Save the time and counters for the next go-around
   _lastTime      = currTime;
   memcpy ((void *)&_last_counters, (void *)&_counters, sizeof (_last_counters));
}


bool DaqBuffer::enableTx ( const char *addr, uint16_t port ) {
   this->disableTx();

   memset(&_txServerAddr,0,sizeof(struct sockaddr_in));
   _txServerAddr.sin_family = AF_INET;
   _txServerAddr.sin_addr.s_addr=inet_addr(addr);
   _txServerAddr.sin_port=htons(port);

   if ( (_txFd = socket(AF_INET,SOCK_STREAM,0) ) < 0 ) {
      fprintf(stderr,"DaqBuffer::enableTx -> Failed to create socket\n");
      return false;
   }

   if ( connect(_txFd,(struct sockaddr *)&_txServerAddr, sizeof(struct sockaddr_in)) != 0 ) {
     fprintf(stderr,"DaqBuffer::enableTx -> Failed to connect to server at %s port %u\n",addr,port);
      this->disableTx();
      return false;
   }


   int       sndSize;
   socklen_t optSize;

   /* Retrieve the socket's send buffer size */
   optSize = sizeof (sndSize);

   sndSize = 1024*1024;
   setsockopt (_txFd, SOL_SOCKET, SO_SNDBUF, &sndSize, optSize);
   fprintf (stderr, "Send buffer size (request)  = %d\n", sndSize);


   getsockopt (_txFd, SOL_SOCKET, SO_SNDBUF, &sndSize, &optSize);
   fprintf (stderr, "Send buffer size (readback) = %d\n", sndSize);

   _txSequence = 0;
   return(true);
}

void DaqBuffer::resetCounters() {

   _counters.reset      ();
   _last_counters.reset ();

   _rxSize        = 0;
   _txSize        = 0;
   _txSeqId       = 0;

   gettimeofday (&_lastTime, NULL);
}


// Close
void DaqBuffer::disableTx () {
   if ( _txFd >= 0 ) {
      ::close(_txFd);
      _txFd = -1;
   }
}


/* ---------------------------------------------------------------------- *//*!

  \brief Sets the data taking parameters configuration

   \param[in]  blowOffDmaData A flag indicating to return the data dma 
                              buffers immediately after reception.
   \param[in]  blowOffTxEth   A flag indicating to return the event
                              immediately after reception in the transmit
                              task.
   \param[in]  pretrigger     The number of usecs before the trigger to
                              open the event window
   \param[in]  duration       The duration, in usecs, of the event window
   \param[in]  period         The software triggering period, in usecs.

    The two blow off parameters are primarly used in debugging and
    checkout phases.  These allow one to monitor the reception and 
    disposition of data by dumping the data at various places along
    the acquisition pipeline.
                                                                          */
/* ---------------------------------------------------------------------- */
void DaqBuffer::setConfig (uint32_t blowOffDmaData,
                           uint32_t   blowOffTxEth,
                           uint32_t     pretrigger,
                           uint32_t       duration,
                           uint32_t         period)
{
   _config._blowOffDmaData  = blowOffDmaData;
   _config._blowOffTxEth    = blowOffTxEth;
   _config._pretrigger      = TimingClockTicks::from_usecs (pretrigger);
   _config._posttrigger     = TimingClockTicks::from_usecs (duration - pretrigger);
   _config._period          = TimingClockTicks::from_usecs (period);
}
/* ---------------------------------------------------------------------- */

// Set run mode
void DaqBuffer::setRunMode ( RunMode mode ) {
   _config._runMode        = mode;
}
