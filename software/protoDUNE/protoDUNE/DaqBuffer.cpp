///////////////////////////////////////////////////////////////////////////
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
// 2018.03.26 jjr Release 1.2.0-0
//                Modify TimingMsg to match the V4 firmware
// 2018.02.05 jjr Release 1.1.0-0
//                1) Fixed to run at higher rates.
//                2) Added error bit when a packet drops frames to the
//                3) TpcStream record
//                4) If any error occurs, the TPC stream record type
//                   and the Data subrecord type is marked as TpcDamaged
// 2017.10.12 jjr Release 1.0.4-0,
//                This corrects 2 errors in the calculation of lengths
//                in the output TpcDataRecords.
//                   - The length of the table of contents (TOC) record
//                     failed to include the terminating 64-bit word
//                   - The length of the Packet record failed to include
//                     the header size.
// 2017.09.15 jjr Readied for release 1.0.3-0
// 2017.09.15 jjr Removed a lot of debugging printout.
//
//                Corrected an error whereby the event list was not
//                getting properly initialized for use. This manifested
//                itself in the trgNode not being initialized, so that
//                getIndex could not find the triggering sample because
//                the trigger node was incorret.  This was fixed, but
//                I still saw 1 error around 800 events, so there is
//                still some low level problem.  I am ignoring that for
//                now to get this out in the field.
//
// 2017.09.07 jjr Cleaned up the txRun so that the formatting and
//                transmisssion of the data are more clearly separated.
//
//                Added the Range Record to point mark the event window
//                and the start pointing points in the timesamples. This
//                allows the reader interface routines to locate what
//                has been termed the 'trimmed' data.
//
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
#include "TxMessage.h"
#include "Rssi.h"

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
      CLOCK_PERIOD    = 20, /*!< Number of nanoseconds per clock tick     */
      PER_SAMPLE      = 25, /* Number of clock ticks between ADC samples  */
      SAMPLE_PERIOD   = CLOCK_PERIOD * PER_SAMPLE,
                            /*!< Number of nanoseconds between ADC samples*/
      PER_FRAME       = PER_SAMPLE * 1024,
                            /*!< Elapsed time, in ticks, in a 1024 packet */
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

      This is a 4 bit field which more properly may be thought of as two
      3-bit fields depending on the state of bit 3

          - Bit 3 == 0,  the low 3 bits are the command type
          - Bit 3 == 1,  the low 3 bits are the trigger type
                                                                          */
   /* ------------------------------------------------------------------- */
   enum Type
   {
      TimeSync   = 0,  /*!< Timing resynchonization request               */
      Echo       = 1,  /*!< Echo message to measure the time delay        */
      SpillStart = 2,  /*!< Start of spill                                */
      SpillStop  = 3,  /*!< End   of spill                                */
      RunStart   = 4,  /*!< Start of run                                  */
      RunStop    = 5,  /*!< End   of run                                  */
      Rsvd6      = 6,  /*!< Reserved                                      */
      Rsvd7      = 7,  /*!< Reserved                                      */
      Trigger    = 8,  /*!< All values >= 8 are triggers                  */
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

      // -----------------------------------------------------
      // Demand timing system to be running and the type >= 8
      // The easiest test for a trigger (type >= 8), is to
      // check bit 3 of the type field.
      // -----------------------------------------------------
      return (m_tsw & 0xf8) == ((Running << 4) | Trigger);
   };


   static TimingMsg *from (void *p)
   {
      return reinterpret_cast<TimingMsg *>(p);
   }

   static TimingMsg const *from (void const *p)
   {
      return reinterpret_cast<TimingMsg const *>(p);
   }


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
         fprintf (stderr, "LatencyList initialization\n");
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
      if (Debug)
      {
         fprintf (stderr, "Adding[%3d] %p", node->m_body.getIndex (), node);
      }

      check (node, 12, "replacing");
      insert_tail (node);

      // Check if filled the latency depth
      if (m_remaining <= 0)
      {
         // Depth exceeded, remove the oldest node and free it
         node = remove_head ();

         int index = node->m_body.getIndex ();

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
                     " Status of return = %zd index = %d\n", iss, index);
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
/* Forware Reference                                                      */
/* ---------------------------------------------------------------------- */
class Event;
/* ====================================================================== */


/* ====================================================================== *//*!

  \class EventPool
  \brief Pool of events.  Each accepts trigger gets associated with an
         Event.
                                                                          */
/* ---------------------------------------------------------------------- */
class EventPool
{
public:
   EventPool (int nevents);

public:
   Event      *allocate ();
   Event      *deallocate (Event *event);

private:
   CommQueue       *m_pool;  /*!< The pool of events                      */
};
/* ---------------------------------------------------------------------- */



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

      m_trgNode[0] = 0;
      m_trgNode[1] = 0;

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
¯                                                                         */
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
   void init (int index, EventPool *pool)
   {
      m_list[0].init ();
      m_list[1].init ();

      m_trgNode[0] = 0;
      m_trgNode[1] = 0;

      m_index = index;
      m_ctbs  = 0;
      m_nctbs = 0;

      m_pool  = pool;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief  Allocate and initialize an array of Events
     \return Pointer to the list of initialized events

     \param[in] nevents The number of events to construct
                                                                          */
   /* ------------------------------------------------------------------- */
   static Event *construct (int nevents)
   {
      int    nbytes = nevents * sizeof (Event);
      Event *events = reinterpret_cast<decltype(events)>(malloc (nbytes));

      for (int idx = 0; idx < nevents; idx++)
      {
         events[idx].init (idx, NULL);
      }

      return events;
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief Prepares a destination stream for an event

     \param[in] dest  Which list
                                                                          */
   /* ------------------------------------------------------------------- */
   void prepare (int dest)
   {
      m_list   [dest].init ();
      m_trgNode[dest] = 0;
      m_npkts  [dest] = 0;
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



   /* ------------------------------------------------------------------- *//*!

     \enum   ACTION_M
     \brief  Enumerates the various actions to take
                                                                          */
   /* ------------------------------------------------------------------- */
   enum ACTION_M
   {
      ACTION_M_NONE         = 0, /*!< Packet was just added, no action    */
      ACTION_M_POST         = 1, /*!< Post the completed the event        */
      ACTION_M_FREE         = 2, /*!< Free packet back to the DMA buffer. */
      ACTION_M_ADDTOLATENCY = 4, /*!< Place packet on the latency list    */
   };
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief  Scans the list for the trigger node
     \return A flag indicating whether the node was found

     \param[out] trgNode Returned as the trigger node
     \param[out] trgNpkt Returned as the trigger node packet number
     \param[in ] trgTime The trigger time to scan for
     \param[in ] list     The list to scan
                                                                          */
   /* ------------------------------------------------------------------- */
   static bool scanForTriggerNode (List<FrameBuffer>::Node const **trgNode,
                                   uint16_t                       *trgNpkt,
                                   List<FrameBuffer> const           *list,
                                   uint64_t                        trgTime)
   {
      bool                              found = false;
      int                            scanNpkt = 0;
      List<FrameBuffer>::Node const *scanNode = list->m_flnk;
      List<FrameBuffer>::Node const *scanLast = list->m_blnk;

      while (1)
      {
         uint64_t  scanBegTime = scanNode->m_body._ts_range[0];
         if (trgTime >= scanBegTime)
         {
            fprintf (stderr, "--> Adding trig node from prior node\n");
           *trgNode = scanNode;
           *trgNpkt = scanNpkt;
            found   = true;
            break;
         }

         // Check if this was the last node
         if (scanNode == scanLast)
         {
            // This can only happen if the times are screwed up
            found = false;
            break;
         }
         scanNpkt += 1;
         scanNode  = scanNode->m_flnk;
      }

      return found;
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief  Attempt to add this node to the event
     \return A bit mask indicating what to do with both this node and
             the event.

     \param[in]  node The node to add
     \param[in]  dest The contributor index

     \par
      The return code is a bit mask of 3-bits
           - bit  0,  Post the event
           - bit  1,  Free the packet back to the DMA buffer.
           - bit  2,  Place the packet on the latency list

      Code
          - 000  Packet was added
                 Packet was added but event still has pending contributors.
                 ACTION: None

          - 001  Packet was added and completed the event
                 ACTION: Post the event

          - 010  Packet should be freed
                 This packet likely arrived before the event window opened
                 Since triggers/events are time ordered, this packet cannot
                 be a member of an event.  (The cavaet here is that all
                 event are assumed to be of the same duration.  If they
                 weren't, a more recent trigger could possibly reach further
                 back in time than a previous trigger.
                 ACTION: Free

          - 011  Packet should be freed and put on the latency list
                 This is an error state.
                 ACTION: Error

          - 100  Place the packet on the latency list
                 This packet was from a contributor that already completed,
                 but the event still has other pending contributors.
                 ACTION: Place on the latency list

          - 101  This packet arrived after the event window closed, so it
                 is not part of this event. Since packets are assumed to
                 be in time order, this contribution to the event is
                 considered to be complete.
                 ACTION: Post the event and add this packet to the latency list.

          - 110  Not possible, cannot free and place on the latency list
                 ACTION: Error

          - 111  Not possible, cannot post, free and place on the latency list
                 ACTION: Error
                                                                          */
   /* ------------------------------------------------------------------- */
   int32_t evaluate (List<FrameBuffer>::Node *node, int dest)
   {

      uint32_t ctb_mask = (1 << dest);

      // --------------------------------------
      // Check if this contribution is complete
      // --------------------------------------
      if ((m_ctbs & ctb_mask )== 0)
      {
      // -----------------------------------------
         // Since this contribution is complete,
         // it should be added to the latency list
         // --------------------------------------
         return ACTION_M_ADDTOLATENCY;
      }



      // ------------------------------------------
      // Extract the beginning and end timestamps
      // ------------------------------------------
      uint64_t  begTime = node->m_body._ts_range[0];
      uint64_t  endTime = node->m_body._ts_range[1];


      /**
      fprintf (stderr,
               "Adding: %2d: ctbs:%1.1x:%d %16.16" PRIx64 ":%16.16" PRIx64 ""
               ">= %16.16" PRIx64 ":%16.16" PRIx64 " node = %p\n",
               dest, m_ctbs, m_nctbs,
               begTime, m_limits.m_beg, endTime, m_limits.m_end, (void *)node);
      **/


      // ------------------------------------------------------
      // Does this packet end before the trigger window begins?
      // This should be very rare.  Since triggers arrive
      // in increasing time order, this data can never be part
      // of a trigger, i.e. its time has past. Therefore the
      // action is to free this packet.
      // ------------------------------------------------
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
         return ACTION_M_FREE;
      }


      // --------------------------------------------------
      // After the test above, now that the beginning time
      // of this packet/node is >= beginning of the trigger
      // window.  If the beginning time of this packet/node
      // is at or before the end time of the trigger window
      // then at least some portion of this packet/node is
      // within the trigger window.
      // --------------------------------------------------
      if (begTime <= m_limits.m_end)
      {
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


         // ------------------------------------------
         // Some portion of this node/packet is within
         // the event window, add it to the event.
         // ------------------------------------------
         uint16_t npkt = m_npkts[dest];
         m_list [dest].insert_tail (node);
         m_npkts[dest] = npkt + 1;


         // ----------------------------------------------------
         // If the node/packet containing the trigger time frame
         // has not been found yet, check if is the trigger node.
         // ----------------------------------------------------
         if (m_trgNode[dest] == 0)
         {
            uint64_t trgTime = m_trigger.m_timestamp;

            /*
              fprintf (stderr,
              "Checking for trigger node %16.16" PRIx64 " : %16.16" PRIx64
              " : %16.16" PRIx64 "\n",
              begTime, trgTime, endTime);
            */


            // ------------------------------------------------------
            // It rarely happens, but the node containing the trigger
            // sample could occur before the current node.  It is a
            // buffered system, so most cover this possibility.
            // ------------------------------------------------------
            if (trgTime < begTime)
            {
               // ---------------------------------------------------
               // Trigger node has already occurred, must scan for it
               // ---------------------------------------------------
               scanForTriggerNode (m_trgNode + dest,
                                   m_trgNpkt + dest,
                                   m_list    + dest,
                                   trgTime);
            }
            else if (trgTime <= endTime)
            {
               // ------------------------------------------------
               // Now know that the trigger time is at or after
               // the beginning of this node/packet and, with this
               // check, now know that the trigger time is at or
               // before the ending time of this node/packet.
               // i.e. the trigger occurred some time within this
               // packet/node. Record the node and packet number
               // of this node/packet as the trigger node.
               // ------------------------------------------------
               ///fprintf (stderr, "Adding trigger node\n");
               m_trgNode[dest] = node;
               m_trgNpkt[dest] = npkt;
            }
         }


         // Is this end time after the trigger window
         if (endTime >= m_limits.m_end)
         {
            // ---------------------------------
            // Yes, this contributor is complete
            // Remove it from the pending list
            // ---------------------------------
            m_nctbs +=  1;
            m_ctbs  &= ~ctb_mask;

            if (m_nctbs > 2)
            {
               fprintf (stderr,
                        "Error: ctbs > 2 (%d)\n", m_nctbs);
               exit (-1);
            }

            // -----------------------------------------
            // If no more contributors are pending,
            // then this completes the event, so post it
            // -----------------------------------------
            if (m_ctbs == 0)
            {
               return ACTION_M_POST;
            }
         }
         else
         {
            // ------------------------------------
            // No, this contributor is not complete
            // It was just added
            // -----------------------------------
            return ACTION_M_NONE;
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


         // ----------------------------------------------------
         // Since the beginning time of this node/packet is
         // past the end time of the trigger window, and,
         // believing node/packet occur in increasing time order
         // then no more nodes/packets can ever be added to this
         // event for this contributor. Therefore declare this
         // contribution complete.
         //
         // Note: Since this node/packet is not part of this
         // event, this packet is placed on the latency list.
         // ---------------------------------------------------
         if (m_ctbs & ctb_mask)
         {
            m_nctbs += 1;
            m_ctbs  &= ~ctb_mask;

            static int DumpCount = 1000;
            if (DumpCount >= 0)
            {
               DumpCount -= 1;
               fprintf (stderr,
                        "Rejecting packet[%1d:%4d] begTime > window endTime "
                        "%16.16" PRIx64 " > %16.16" PRIx64 " Ctbs:%2.2x:%d\n",
                        dest, DumpCount,
                        begTime, m_limits.m_end, m_ctbs, m_nctbs);
            }

            // ---------------------------------
            // Check if all contributions are in
            // ---------------------------------
            if (m_ctbs == 0)
            {
               // ---------------------------------------------
               // This completed the event
               // Action is to add to the latency list and post
               // ---------------------------------------------
               return ACTION_M_ADDTOLATENCY | ACTION_M_POST;
            }
         }


         {
            // ---------------------------------------------
            // Did not complete the event
            // Action is to only add to the latency list
            // ---------------------------------------------
            return ACTION_M_ADDTOLATENCY;
         }
      }

      // ---------------------------------------------
      // The code cannot reach this point,
      // but the compiler does not seem to know this.
      // The code will return before it gets here.
      // ---------------------------------------------
      return ACTION_M_NONE;
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------  *//*!

     \brief  Scans the latency list for the first data frame that is
             included in this event.  All packets before this first frame
             are freed and the list of all packets, including this first
             frame, is transferred to the event.

     \retval == 0, the caller should take no action. All relevant
             packets in latency list from all contributors have been
             transferred to the event being assembled. However the event
             is not complete
     \retval == 1, the caller should post the event. The transfer of
             all relevant events from the latency list to the event
             being assembled resulted in the completion of the event.
             This is rare but can happen when packets are dropped

     \param[in]  lists  The array of latency lists for all the contributors
     \param[in]     fd  The fd used to return the unused packets
     \param[in]   ctbs  Bit mask of the enabled contributors
                                                                          */
   /* ------------------------------------------------------------------- */
   int seedAndDrain (LatencyList *lists, int fd, uint32_t ctbs)
   {
      // Initialize the pending list to waiting for all enable contributors
      m_ctbs  = ctbs;
      m_nctbs = 0;

      /**
      fprintf (stderr,
               "Pending contributors %2.2" PRIx16 " (%d)\n",
               m_ctbs,
               m_nctbs);
      **/


      // Loop over the latency lists of all the destinations
      for (int ictb = 0; ictb < MAX_DEST; ictb++)
      {
         // ---------------------------------------------------------
         // Get the contributor list to scan and
         // prepare the context for this contributor for a new event.
         // ---------------------------------------------------------
         LatencyList *list =  &lists[ictb];
         prepare (ictb);


         /**
         fprintf (stderr,
                    "SeedAndDrain[%d] list = %p : %p:%p (check empty)\n",
                  ictb,
                  list,
                  list->m_flnk,
                  list->m_blnk);
         **/


         {
            List<FrameBuffer>::Node const *last = list->terminal ();
            List<FrameBuffer>::Node const *cur  = list->m_flnk;
            int                            cnt  = 0;
            while (last != cur)
            {
               /**
               fprintf (stderr,
                        "Node[%d.%2d] = %p\n", ictb, cnt, (void *)cur);
               **/

               cnt += 1;
               cur  = cur->m_flnk;
            }
         }



         // ---------------------------------------------------------
         // Scan the nodes/packets and determine what to do with them
         //
         // In general there are 3 possible fates
         //
         //    1. This node occurred fully before the event window
         //       opened.
         //       ACTION: The node will be free back to the DMA pool
         //
         //    2. This node is part of the event
         //       ACTION: The node will be transferred from the
         //       latency list to its contributor list in the event.
         //       It is possible that this packet extends past the
         //       close of the event window.  In this case this
         //       contribution is complete.
         //
         //    3. The node occured fully after the event window closed.
         //       ACTION: Replace this node at the head of the latency
         //       list and move on to the next contributor.  By
         //       implicationm, this means that this contribution to
         //       the event is complete.
         //
         // It is possible, but unusual, that the event could complete
         // under scenerios 2 & 3.
         // ----------------------------------------------------------
         while (1)
         {
            List<FrameBuffer>::Node *flnk;

            // -------------------------------------------
            // Remove a node
            // But reduce the count until we are sure that
            // this node has not been returned to the list.
            // -------------------------------------------

            /**
            fprintf (stderr,
                     "SeedAndDrain[%d] Scanning list = %p : %p:%p (before)\n",
                     ictb,
                     list,
                     list->m_flnk,
                     list->m_blnk);
            **/
            flnk = list->remove_head ();

            /**
            fprintf (stderr,
                     "SeedAndDrain[%d] Scanning list = %p : %p:%p (after)\n",
                     ictb,
                     list,
                     list->m_flnk,
                     list->m_blnk);
            **/



            // -------------------------------------------
            // Check if the latency list is exhausted.
            // -------------------------------------------
            if (flnk ==  NULL)
            {
               // --------------------------------------------------
               // This contributor is exhausted
               // --------------------------------------------------

               /**
               fprintf (stderr, "seedAndDrain:Reset[%d] to empty count = %d\n",
                        ictb, list->m_remaining);
               **/

               list->resetToEmpty ();
               break;
            }

            list->check (flnk, 12, "seedAndDrain checky");

            // ---------------------------------------------------------
            // Get and check the disposition action for this node/packet
            // ---------------------------------------------------------
            int32_t action = evaluate (flnk, ictb);

            /**
            fprintf (stderr,
                     "seedAndDrain:Action[%d] %p = %2.2" PRIx32 " remaining = %d\n",
                     ictb, (void *)flnk, action, list->m_remaining);
            **/

            if (action & ACTION_M_FREE)
            {
               // --------------------------------------------------------
               // Free this node, it occurred before the event window
               // opened so is not part of this or any future event.
               // So just return it to the DMA pool and move on to the
               // next node. This addes to the number of nodes needed to
               // completely populate the latency list.
               // --------------------------------------------------------
               int index = flnk->m_body.getIndex ();

               dmaRetIndex (fd, index);
               list->m_remaining += 1;

               /**
               fprintf (stderr,
                        "seedAndDrain:Freeing[%d] %p index = %d remaining = %d\n",
                        ictb, (void *)flnk, index, list->m_remaining);
               **/

               continue;
            }


            if (action & ACTION_M_ADDTOLATENCY)
            {
               // --------------------------------------------
               // Node is beyond the event window, put it back
               // at the head of the latency list. Since all
               // remaining nodes will be later, and therefore
               // also beyond the event window, done with this
               // contributor.
               // --------------------------------------------
               list->check (flnk, 12, "seedAndDrain adddToLatency");
               list->insert_head (flnk);


               /**
               int index = flnk->m_body.getIndex ();
               fprintf (stderr,
                     "seedAndDrain:Reinserting[%d] %p index = %d remaining = %d\n",
                        ictb, (void *)flnk, index, list->m_remaining);
               **/


               if (action & ACTION_M_POST)
               {
                  // ---------------------------------------------
                  // Not only coould this node have completed this
                  // contribution to the event, it may have
                  // completed the event.  This is unusual, but
                  // possible. Instruct the caller to post the
                  // event.
                  // --------------------------------------------

                  /**
                  fprintf (stderr,
                     "seedAndDrain:Posting[%d] %p index = %d remaining = %d\n",
                           ictb, (void *)flnk, index, list->m_remaining);
                  **/

                  return ACTION_M_POST;
               }
               else
               {
                  // --------------------------------------------
                  // Since done with this contributor, move on to
                  // the next contributor (if any).
                  // --------------------------------------------

                  /**
                  fprintf (stderr,
                     "seedAndDrain:Done[%d] index = %d remaining = %d\n",
                        ictb, index, list->m_remaining);
                  **/

                  break;
               }
            }

            // --------------------------------------------------
            // Packet was placed on this contributor's event list
            // Reduce the number placed on the latency list.
            // --------------------------------------------------
            list->m_remaining += 1;

            /**
            fprintf (stderr,
                     "seedAndDrain:Add[%d] remaining = %d\n",
                     ictb, list->m_remaining);
            **/



            // ------------------------------------------------------
            // It is highly unusual, but possible that this completes
            // the event, so instruct the caller to post it.
            // ------------------------------------------------------
            if (action & ACTION_M_POST)
            {
               return ACTION_M_POST;
            }

            // ------------------------------------------------
            // If this contribution is complete, move on to the
            // next contributor
            // ------------------------------------------------
            if ( (m_ctbs & (1 << ictb)) == 0)
            {
               break;
            }
         }
      }


      // ----------------------------------------------------------
      // This is the usual, the nodes/packets were just transferred
      // from the latency list to the event list. No action on the
      // callers part is required.
      // ----------------------------------------------------------
      return ACTION_M_NONE;
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
            int index = node->m_body.getIndex ();

            if ((unsigned int)index > 900)
            {

               fprintf (stderr,
                        "Bad [%d.%d] index on free %p 0x%8.8x %d ******\n",
                        idx, count++, node, index, index);

               Count++;
               exit (-1);
               if (max++ > 10) break;
               continue;
            }



            // The frame buffer associated with this event
            // must be the last to be freed
            if (1) ///m_pool == NULL || index != m_index)
            {
               if (index < 8)
               {
                  fprintf (stderr, "Node[%2d.%2d] freeing index %d\n",
                           idx, count++, index);
               }

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

         // Prepare the list for the next event
         prepare (idx);
      }

      /**
      fprintf (stderr, "Node[%2d.%2d] freeing index %d bad = %d\n",
               dest, cnt, m_index, Count);
      **/

      if (m_pool)
      {
         /**
         fprintf (stderr, "Free: to pool @ %p\n", (void *)m_pool);
         **/

         m_pool->deallocate (this);
      }
      else
      {
         /**
         fprintf (stderr, "Free: back to dma pool index = %d\n", m_index);
         **/
         dmaRetIndex (fd, m_index);
      }

   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!

     \brief Only frees the event control structure

     \par
      When doing RSSI transfers, the dma returns the buffer to the
      hardware pool.
                                                                          */
   /* ------------------------------------------------------------------- */
   void free ()
   {
      m_pool->deallocate (this);
      return;
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
   List<FrameBuffer>::Node
           const *m_trgNode[2];  /*!< The triggering node                 */
   uint16_t       m_trgNpkt[2];  /*!< The triggering packet index         */
   int                 m_nctbs;  /*!< Count of contributors               */
   uint32_t             m_ctbs;  /*!< When building, bit mask of
                                      incomplete contributors
                                      When completed, bit mask of
                                      contributions present               */
   uint16_t         m_ctdid[2];  /*!< The WIB crate.slot.fiber id         */
   uint16_t         m_npkts[2];  /*!< The number of packets in this event */
   uint16_t            m_index;  /*!< Associated dma frame buffer index   */
   EventPool           *m_pool;  /*!< The event pool to free this to      */
};
/* ---------------------------------------------------------------------- */
/* END: Event                                                             */
/* ====================================================================== */





/* ====================================================================== */
/* EventPool implementation                                               */
/* ---------------------------------------------------------------------- *//*!

  \brief EventPool constructor

  \par
   The requested number of event descriptors are allocated and committed
   to the pool.
                                                                          */
/* ---------------------------------------------------------------------- */
EventPool::EventPool (int nevents)
{
   // -----------------
   // Allocate the pool
   // -----------------
   m_pool = new CommQueue (nevents, true);


   // ----------------------------------------
   // Allocate the events to populate the pool
   // ----------------------------------------
   int    nbytes = nevents * sizeof (Event);
   Event *events = reinterpret_cast<decltype(events)>(malloc (nbytes));


   // --------------------------------
   // Initialize and populate the pool
   // --------------------------------
   for (int idx = 0; idx < nevents; idx++)
   {
      events[idx].init (idx, this);
      m_pool->push (&events[idx], 0);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Allocates a event descriptor from the pool. Method blocks if
          none are available.

  \return The allocated event descriptor
                                                                          */
/* ---------------------------------------------------------------------- */
Event *EventPool::allocate ()
{
   Event *event = reinterpret_cast<Event *>(m_pool->pop (0));
   return event;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Dellocates a event descriptor back to the pool.

                                                                          */
/* ---------------------------------------------------------------------- */
Event *EventPool::deallocate (Event *event)
{
   m_pool->push (event, 0);
   return 0;
}
/* ====================================================================== */



/* ====================================================================== */
/* BEGIN: construct_fbs                                                   */
/* ---------------------------------------------------------------------- *//*!

  \brief  Allocates and initializes an array of FrameBuffer nodes
  \return A pointer to the list of allocated and initialized FrameBuffer
          nodes.

  \param[in] nfbs  The number of frame buffer nodes to constuction
  \param[in] bufs  An error of pointers to the data buffers
                                                                          */
/* ---------------------------------------------------------------------- */
static List<FrameBuffer>::Node *construct_fbs (int nfbs, uint8_t **bufs)
{
   // Allocate enough frame buffer nodes to capture an all possible events
   List<FrameBuffer>::Node *fbs = reinterpret_cast<decltype (fbs)>
                                    (malloc (nfbs * sizeof (*fbs)));

   for (int idx = 0; idx < nfbs; idx++)
   {
      // Initialize the one time only fields
      fbs[idx].m_body.setData  (bufs[idx]);
      fbs[idx].m_body.setIndex (idx);
   }


   return fbs;
}
/* ---------------------------------------------------------------------- */
/* END: construct_fbs                                                     */
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
         /**
         uint64_t delta = timestamp[1] - timestamp[0];
         fprintf (stderr,
                  "Check %16.16" PRIx64 ":%8.8" PRIx32 ":"
                  "%16.16" PRIx64 ":%8.8" PRIx32 " trigger -> yes  %16.16" PRIx64 "\n",
                  timestamp[0], beg, timestamp[1], end, delta);
         **/

         return timestamp[1] - end;
      }
      else
      {
         /**
            fprintf (stderr, " -> no\n");
         **/
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

   void dump_headerFrame (void const        *data,
                          int              nbytes,
                          int                dest);

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
 *  \brief  Hex dump of all but the ADC data
 *
 *  \param[in]   data The frame data
 *  \param[in] nbytes The number bytes in the data
 *  \param[in]   dest Which stream
 *
\* ---------------------------------------------------------------------- */
inline void FrameDiagnostics::dump_headerFrame (void const *data,
                                                int       nbytes,
                                                int         dest)
{
   ////if (!m_dataFrameDump[dest].declare ()) return;

   uint64_t const *s = (uint64_t const *)data;
   int            ns = nbytes / sizeof (*s);

   s  += 1;
   ns -= 2;

   uint64_t expected = s[1];

   for (int idx = 0; idx < ns; idx += 30)
   {
      uint64_t ts = s[idx+1];

      // --------------------------------------------
      // If not as expected dump +-3 around the error
      // --------------------------------------------
      if (ts < expected)
      {
         uint64_t expa = s[idx - 3 * 30 + 1];
         for (int  idy = idx - 3*30; idy <= idx + 3*30; idy += 30)
         {
            uint64_t tsa = s[idy+1];

            printf ("%5x"
                    " %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 ""
                    " %16.16" PRIx64 " %16.16" PRIx64 "%c\n",
                    idy,
                    s[idy], s[idy+1], s[idy+2], s[idy+3], s[idy+16],
                    tsa != expa ? '*' : ' ');
            expa = tsa + TimingClockTicks::PER_SAMPLE;
         }
         putchar ('\n');
      }

      expected = ts + TimingClockTicks::PER_SAMPLE;
   }

   return;
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
#  define NBYTES (unsigned int)(((N64_PER_FRAME * 1024) + 1  + 1) \
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



   // -----------------------------------------------------------------------
   // -- The header word has been eliminated, so no need to increment past it
   // data += 1;
   // -----------------------------------------------------------------------


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



/* ====================================================================== */
#define MAX_CONTRIBUTORS  MAX_DEST
#define MAX_PACKETS      (MAX_DEST) * 32

// ---------------------------------------------------------
// Max size of the non-data dependent portion of a TpcRecord
// ---------------------------------------------------------
#define MAX_TPCSTREAMSIZE ( sizeof (pdd::fragment::tpc::Stream)           \
                          + sizeof (pdd::fragment::tpc::Ranges)           \
                          + sizeof (pdd::fragment::tpc::Toc<MAX_PACKETS>) \
                          + sizeof (pdd::fragment::tpc::Packet) )         \
                          / sizeof (uint32_t)
/* ====================================================================== */




/* ====================================================================== *//*!

  \class HeaderAndOrigin
  \brief Contents the semi-static information of the output data record

 \par
  This information, expect for the data length  is more or less static.
  The source identifiers are really one-time initialization.  The one
  twist is for events with no data. Since the trailer is always tacked
  onto the last iov, in this case this is the only iov. Therefore storage
  must be provided for this eventuality.  The nominal situation is that
  this iov is only the length of the Header and Originator records. Only
  in the special case of no data is space for the trailer used.
                                                                          */
/* ---------------------------------------------------------------------- */
class HeaderAndOrigin
{
public:
   //HeaderAndOrigin () { return; }

public:
   pdd::fragment::Header<pdd::fragment::Type::Data> m_header;
   pdd::fragment::Originator                        m_origin;
   uint64_t                     m_xwrds[2*MAX_TPCSTREAMSIZE];

public:
   HeaderAndOrigin &operator = (HeaderAndOrigin const &src)
   {
      int nbytes = n64 () * sizeof (uint64_t);
      printf ("Copying nbytes = %d\n", nbytes);
      memcpy (this, &src, nbytes);
      return *this;
   }

   void *getNextAddress ()
   {
      void *next = reinterpret_cast<void *>
                  (reinterpret_cast<uint64_t *>(this) + n64 ());
      return next;
   }

public:
   pdd::Trailer &getTrailer () 
   { 
      return *reinterpret_cast<pdd::Trailer *>(getNextAddress ());
   }

   pdd::fragment::tpc::Stream *getStreamRecord () 
   { 
      void *next = getNextAddress ();
      printf ("HO this = %p   stream = %p  N64 = %4.4" PRIx32 " origin = %4.4" PRIx32 "\n",
              (void *)this, next, n64 (), sizeof (m_origin));

      return reinterpret_cast<pdd::fragment::tpc::Stream *>(next);
   }


public:
   // ----------------------------------------------------
   // Return the number bytes rounded to a 64-bit boundary
   // This only include the Header and Originator records.
   // If the trailer is used, the length in the iov is
   // increased by trailer size.
   // ----------------------------------------------------
   uint32_t n64 ()
   {
      uint32_t n64l = sizeof(m_header) / sizeof (uint64_t)
                    + m_origin.n64 ();
      return n64l;
   }

};
/* ---------------------------------------------------------------------- */


HeaderAndOrigin HeaderOrigin;
uint16_t      Srcs[MAX_DEST] = { 0 };



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
   _config._enableRssi      =     0;
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
   int status = dma.open (devname, dests, ndests);
   if (status < 0)
   {
      fprintf(stderr,
              "DaqBuffer::open -> Error opening %-8s dma device\n",
              name);
      return false;
   }

   #if 0
   // Enable the destinations
   status = dma.enable (dests, ndests);
   if (status < 0)
   {
      fprintf(stderr,
              "DaqBuffer::open -> Unable to set %-8s dma mask\n",
              name);
      return false;
   }
   #endif


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

   \brief  Helper method to enable a AXI dma channel
   \retval true,  successful enable
   \retval false, successful enableKconstruction

   \param[in]     dma  The dma channel to enable

    This just wraps the DMA class enable method with error reporting
                                                                          */
/* ---------------------------------------------------------------------- */
static bool enable (DaqDmaDevice &dma)
{
   int status = dma.enable ();
   if (status < 0)
   {
      fprintf(stderr,
              "DaqBuffer::enable -> Unable to set %-8s dma mask\n",
              dma._name);
      return false;
   }

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
      fputs ("ESTABLSH DMA INTERFACES\n", stderr);

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


   // !!! KLUDGE !!!
   // Enable driver debugging
   dmaSetDebug(_dataDma._fd, 1);

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
   ///
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

  \param[in]  range The timestamp range of this packet
  \param[in]    d64 64-bit pointer to the data packet
  \param[in] nbytes The read length

                                                                          */
/* ---------------------------------------------------------------------- */
static inline void getTimestampRange (uint64_t     range[2],
                                      uint64_t const   *d64,
                                      uint32_t      nbytes)
{
   // --------------------------------------------------------------------
   // 2018.05.07 -- jjr
   // Initial header word eliminated for RSSI transport.
   // The original + 1 new trailer word now contains this information
   //
   // Locate the timestamp in the first WIB frame.  Since the data starts
   // with the WIB frame, the timestamp is in word #1 (starting from 0).
   // -----------------------------------------------------=-------------
   uint64_t  begin = d64[1];


   // -------------------------------------------------------------------
   // Locate the start of the last WIBframe and get its timestamp.
   // Add the number of clock ticks per time sample to get the
   // ending time.
   // -------------------------------------------------------------------
   d64 += nbytes/sizeof (uint64_t) - 30 - 1 - 1;
   uint64_t end = d64[1] + TimingClockTicks::PER_SAMPLE;


   #if 0
   fprintf (stderr,
           "Beg %16.16" PRIx64 " End %16.16" PRIx64 " %16.16" PRIx64
           " %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 "\n",
            begin, end, d64[-1], d64[0], d64[1], d64[2]);
   #endif


   // -----------------------------------------------------------
   // Store timestamp of the first sample in the first WIB frame
   // and the last sample in the last WIB frame as the range
   // -----------------------------------------------------------
   range[0] = begin;
   range[1] =   end;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static TimingMsg const *readTimingMsg (int              *timingIndex,
                                       DaqDmaDevice       &timingDma,
                                       int                   blowOff,
                                       enum RunMode          runMode,
                                       bool                 isActive,
                                       volatile uint32_t     &trgCnt,
                                       volatile uint32_t  &trgMsgCnt,
                                       volatile uint32_t  &disTrgCnt,
                                       MonitorRate             &rate,
                                       FrameDiagnostics        &diag)
{
   uint32_t rxSize;
   uint32_t  index;
   uint32_t  flags;
   uint32_t   dest;
   uint32_t  error;
   TimingMsg *tmsg =  NULL;


   rxSize = dmaReadIndex (timingDma._fd, &index, &flags, &error, &dest);

   rate.monitor       (2);
   diag.dump_received (index, 2, rxSize);

   // ------------------------------------
   // Check if should process this message
   // ------------------------------------
   if ( (!blowOff                    ) &&
        (runMode == RunMode::EXTERNAL) &&
        (dest    == 0xff)            )
   {
      // -----------------------------------
      // Get a pointer to the timing message
      // -----------------------------------
      tmsg = TimingMsg::from (timingDma._map[index]);
      diag.dump_timingFrame ((uint64_t const *)tmsg, rxSize);



      // -----------------------------------------------------------
      // Count timing messages, process if this is a trigger message
      // -----------------------------------------------------------
      trgCnt += 1;
      if (tmsg->is_trigger ())
      {
         trgMsgCnt += 1;


         // --------------------------------------------------------
         // DEBUGGING
         // Extract the time stamp and check the delta time from
         // the previous trigger message
         // --------------------------------------------------------

         /**
         {
            static uint64_t PrvTrgTime = 0x0;
            uint64_t           trgTime = tmsg->timestamp ();
            float                   dt = float(trgTime - PrvTrgTime)
                                       * TimingClockTicks::CLOCK_PERIOD
                                       / 1.e6;

            fprintf(stderr, "Last accepted trigger ts = %16.16" PRIx64
                    ", dt = %.1f ms \n",
                    trgTime, dt);

            PrvTrgTime = trgTime;
         }
         **/
         // -------------------------------------------------------


         // -------------------------------------------------------
         // Check if an event is in progress
         // Since triggers are disabled while an event is active
         // isActive should always be false
         // -------------------------------------------------------
         if (!isActive)
         {
            *timingIndex = index;
            return tmsg;
         }
         else
         {
            // -------------------------------------------------------
            // This should never happen, while an event is in progress
            // -------------------------------------------------------
            fputs ("Discarding trigger\n", stderr);
            disTrgCnt += 1;
         }
      }
   }


   // --------------------------------------------------
   // If was not a timing message or an event was active
   // Then dispose of this message
   // --------------------------------------------------
   timingDma.free (index);


   // ------------------------------------
   // Indicate no new event is in progress
   // ------------------------------------
   return NULL;
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




/* ---------------------------------------------------------------------- */
#if  CHECK_ARRIVALTIMES
/* ---------------------------------------------------------------------- *//*!

  \brief Checks the rate of data packet arrivals

  \par
   This is purely a diagnostic/monitoring tool
                                                                          */
/* ---------------------------------------------------------------------- */
static void checkArrivalTimes ()
{
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
   return;
}
/* ---------------------------------------------------------------------- */
#else

#define checkArrivalTimes()

/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */

#if CHECK_TIMESTAMPS

/* ---------------------------------------------------------------------- *//*!

   \brief Checks that the current timestamp matches what is expected
          based on the previous timestamp

   \param[in] data Pointer to the incoming data
   \param[in] dest The data channel
                                                                          */
/* ---------------------------------------------------------------------- */
static void checkTimestamps (uint64_t const *data, int dest)
{
   static uint64_t NextTimestamp[2] = { 0, 0 };

   // -----------------------------------------------------------------
   // 2018.05.02 -- jjr
   // ----------
   // Correct location of timestamp after initial head word elimination
   // -----------------------------------------------------------------
   uint64_t  exp  = NextTimestamp[dest];
   uint64_t  got  = data[1];
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

   NextTimestamp[dest] = got + TimingTicks::PER_FRAME;
   return;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */

#define checkTimestamps(_data, _dest)

/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



#define CHECK_LATENCY 1
/* ---------------------------------------------------------------------- */
#if CHECK_LATENCY
#include <limits.h>

static void checkLatency (uint64_t trgTime,
                          uint64_t datTime,
                          bool       found) __attribute ((unused));

/* ---------------------------------------------------------------------- *//*!

  \brief Checks the latency between the trigger arrival time and the
         most recent data packet

  \param[in]  trgTime The trigger time
  \param[in]  datTime The data    time
  \param[in]    found Flag indicating whether the trigger time was found
                      in the latency buffer list.
                                                                          */
/* ---------------------------------------------------------------------- */
static void checkLatency (uint64_t trgTime,
                          uint64_t datTime,
                          bool       found)
{
   static int64_t MaxLatency = LLONG_MIN;
   static int64_t MinLatency = LLONG_MAX;

   int32_t diff = trgTime - datTime;

   if (diff > MaxLatency) MaxLatency = diff;
   if (diff < MinLatency) MinLatency = diff;

   fprintf (stderr,
            "Latency [%16" PRId64 ":%16" PRId64 "] diff = %8.8" PRIx32 " time = "
            " %16.16" PRIx64 "found = %d\n",
            MinLatency, MaxLatency, diff, datTime, found);

   return;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define checkLatency(_trgTime, _datTime, _found)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



/* +===================================================================== */
/* BEGIN: MonitorTimes                                                    */
/* ---------------------------------------------------------------------- *//*!

  \brief Monitor the time period between each reoccurring event

  \par
   It does this maintaining a list of struct timevals that are recorded
`  just before (or after) the event.  The list is dumped or either an
   explicit request (\a report) or if the maximum number of samples
   has been reached.  Guffering the times and only periodically
   reporting them minimizes the impact on the measurement.  This is
   in contrast if each period was printed as it occurred.
                                                                          */
/* ---------------------------------------------------------------------- */
class MonitorTimes
{
public:
   MonitorTimes (int count) :
      m_ntimes   (0),
      m_overflow (0),
      m_tottimes (count),
      m_times  ((struct timeval *) malloc (count * sizeof (*m_times)))
   {
      return;
   }

public:
   int record ()
   {
      gettimeofday (&m_times[m_ntimes++], NULL);

      if (m_ntimes == m_tottimes)
      {
         report (m_ntimes);
         reset  ();
      }

      return m_ntimes;
   }

   void report (int counts)
   {
      fprintf (stderr, "Interpacket times");

      struct timeval *prvTime = m_times;
      uint32_t        elapsed;

      for (int idx = 0; idx < counts; idx++)
      {
         struct timeval difTime;
         timersub (&m_times[idx], prvTime, &difTime);
         if (difTime.tv_sec)
         {
            elapsed = 999999;
         }

         elapsed = difTime.tv_usec;
         prvTime = &m_times[idx];

         if ((idx & 0x7) == 0) fprintf (stderr, "\n%3x", idx);
         fprintf (stderr, " %6" PRIu32 "", elapsed);
      }
   }

   void reset ()
   {
      m_overflow = 0;
      m_ntimes   = 0;
   }


public:
   int              m_ntimes;
   int            m_overflow;
   int            m_tottimes;
   struct timeval   *m_times;
};
/* ---------------------------------------------------------------------- */
/* END: MonitorTimes                                                      */
/* ====================================================================== */




/* ====================================================================== */
/* BEGIN: MonitorCorrelations                                             */
/* ---------------------------------------------------------------------- *//*!

  \brief Monitors the time difference between the current trigger
         timestamp and the most recently received data packet

  \par
   This is meant to monitor how far out of synch these to packets are
                                                                          */
/* ---------------------------------------------------------------------- */
class MonitorCorrelation
{
public:
   MonitorCorrelation () :
      m_prvTriggerTimestamp (0)
   {
      return;
   }


public:
   void report (uint64_t triggerTimestamp, uint64_t dataTimestamp)
   {
      struct timeval curTime;
      gettimeofday (&curTime, NULL);

      if (triggerTimestamp != m_prvTriggerTimestamp)
      {
         int64_t      dts = triggerTimestamp - m_prvTriggerTimestamp;
         uint64_t elapsed = subtimeofday (&curTime, &m_prvTriggerTime);
         int        delta = elapsed - (dts * TimingClockTicks::CLOCK_PERIOD)/1000;

         if (delta)
         {
            fprintf (stderr,
                     "Trg:Trg   msg ts[%d] = %16.16" PRIx64 ": %16.16" PRIx64 ""
                     " dts     = %8" PRId64 " usecs = %8" PRIu64 ":%d\n",
                     0,
                     triggerTimestamp,
                     m_prvTriggerTimestamp,
                     dts,
                     elapsed,
                     delta);
         }

         m_prvTriggerTime = curTime;
      }



      // Check difference of incoming data packet with the trigger timestamp
      int diff = triggerTimestamp - dataTimestamp;
      if (diff > TimingClockTicks::PER_SAMPLE)
      //if (triggerTimestamp != m_prvTriggerTimestamp)
      {
         //m_prvTriggerTimestamp = triggerTimestamp;

         fprintf (stderr,
                  "Data:Trg   msg ts[%d] = %16.16" PRIx64 ": %16.16" PRIx64 ""
                  " diff = %8d  npkts = %d\n",
                  0,
                  dataTimestamp,
                  triggerTimestamp,
                  diff,
                  diff/TimingClockTicks::PER_SAMPLE);

      }

      m_prvTriggerTimestamp = triggerTimestamp;

      // -----------------------------------------
      // Check wallclock time between data packets
      // -----------------------------------------
      uint64_t elapsed = subtimeofday (&curTime, &m_prvDataTime);
      // Check the difference of the last to data packets
      if (elapsed > 600) /// || elapsed < 400)
      {
         fprintf (stderr,
                  "Data:Data      [%d] = %16.16" PRIx64 ": %16.16" PRIx64 ""
                  " elapsed = %8d  npkts = %d",
                  0,
                  dataTimestamp,
                  m_prvDataTimestamp,
                  (unsigned)elapsed,
                  ((unsigned)elapsed)/512);

         // What is the elapsed time since the last large mismatch
         if (elapsed > 12*512)
         {
            unsigned deltaBig = subtimeofday (&curTime, &m_prvDataBigTime);
            fprintf (stderr,
                     "deltaBig = %u\n",
                     deltaBig);
            m_prvDataBigTime = curTime;
         }
         else
         {
            fputc ('\n', stderr);
         }


      }

      m_prvDataTimestamp = dataTimestamp;
      m_prvDataTime      = curTime;

      return;
   }


public:
   uint64_t       m_prvTriggerTimestamp;
   uint64_t          m_prvDataTimestamp;
   struct timeval      m_prvTriggerTime;
   struct timeval         m_prvDataTime;
   struct timeval      m_prvDataBigTime;
};
/* ---------------------------------------------------------------------- */
/* END: MonitorCorrelations                                               */
/* ====================================================================== */



/* ---------------------------------------------------------------------- */
static void postEventAndReset (CommQueue     *queue,
                               Event         *event,
                               LatencyList *latency)
{
   // Get the list of contributors
   uint32_t ctbs = ((1 << MAX_DEST) - 1) & ~event->m_ctbs;


   static int Cnt[4] = { 0, 1, 1, 2 };
   if (Cnt[ctbs] != event->m_nctbs)
   {
      fprintf (stderr,
               "ctbs = %4.4" PRIx32 ":%4.4" PRIx32
               "inconsistent with event->m_nctbs = %d\n",
               ctbs, event->m_ctbs, event->m_nctbs);
      exit (-1);
   }


   event->m_ctbs = ctbs;

   /**
   fprintf (stderr,
           "Posting %p ctbs = %4.4" PRIx32 ":%d\n",
            (void *)(event), event->m_ctbs, event->m_nctbs);
   **/

   queue->push (event);


   /**
   while (ctbs)
   {
      uint32_t ctb = ffsl (ctbs) - 1;
      ////latency[ctb].resetToEmpty ();

      fprintf (stderr,
               "postEventAndReset: remaining = %d\n",
               latency[ctb].m_remaining);


      {
         List<FrameBuffer>::Node const *last = latency[ctb].terminal ();
         List<FrameBuffer>::Node const *cur  = latency[ctb].m_flnk;
         int                            cnt  = 0;
         while (last != cur)
         {
            fprintf (stderr,
                     "postEventAndReset: Node[%d.%2d] = %p\n",
                     ctb, cnt, (void *)cur);
            cnt += 1;
            cur  = cur->m_flnk;
         }
      }

      ctbs  &= ~(1 << ctb);
   }
   **/


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Get the WIB Cate.Slot.Fiber for the contributors

  \param[in] dataDma  The WIB dma channel

   This is a pretty hokey way to find the identity of the WIB streams,
   i.e. the Crate.Slot.Fiber.  The data stream is read and the identity
   is extracted from the data.  The data is then returned.
                                                                          */
/* ---------------------------------------------------------------------- */
static void getWibIdentifiers (uint16_t srcs[MAX_DEST], DaqDmaDevice *dataDma)
{
   uint32_t mask = (1 << MAX_DEST) - 1;


   // -------------------------------------------------------
   // This just protects one from going into an infinite wait
   // -------------------------------------------------------
   int maxTries = MAX_DEST * 10;


   while (mask)
   {
      uint32_t index;
      uint32_t flags;
      uint32_t  dest;
      int32_t rxSize = dmaReadIndex (dataDma->_fd, &index, &flags, NULL, &dest);

      maxTries -= 1;
      if (maxTries <= 0) break;


      // Was the read successful
      if (rxSize > 0)
      {
         uint32_t lastUser = axisGetLuser (flags);
         if ((lastUser & DaqDmaDevice::TUserEOFE) == 0)
         {
            // Is this a data source
            if (dest < MAX_DEST)
            {
               uint32_t mdest = (1 << dest);
               if (mask & mdest)
               {
                  uint64_t *data = reinterpret_cast<decltype(data)>
                                   (dataDma->_map[index]);
                  uint64_t   csf = (data[0] >> 13) & 0xfff;
                  srcs[dest]     =    csf;
                  mask          &= ~mdest;
               }
            }
         }

         dataDma->free (index);
      }
   }

   unsigned int src0 = srcs[0];
   unsigned int src1 = srcs[1];
   fprintf (stderr,
            "RCE is servicing Wib Sources = "
            " %2x.%1x.%1x (0x%3.3" PRIx32 ")"
            " %2x.%1x.%1x (0x%3.3" PRIx32 ")\n",
            ((src0 >> 3) & 0x1f),
            ((src0 >> 8) & 07),
            ((src0 >> 0) & 0x3),
              src0,
            ((src1 >> 3) & 0x1f),
            ((src1 >> 8) & 07),
            ((src1 >> 0) & 0x3),
              src1);

   return;
}
/* ---------------------------------------------------------------------- */




// Class method for rx thread running
void DaqBuffer::rxRun ()
{
   // -------------------------------------------------------------------
   // Initialize everything that can be before enabling the trigger/timing
   // and data streams.  Enabling them too early will put more time
   // between when packets start being queued to these streams and when
   // they are read.  This can lead to the streams backing up many packets
   // in the queue.  This is not necessarily a bad thing, but it extreme
   // cases can lead to packets being dropped.  Might as well avoid this
   // since it easy to do
   // -------------------------------------------------------------------

   static int  InCnt[2] = { 0, 0 };
   static int OutCnt[2] = { 0, 0 };


   fputs ("STARTING\n", stderr);

   // -------------------------------------------------------------------
   // Request array of events, one for every possible trigger
   // Given that
   //   1. there can't be more triggers than front-end frame buffers and
   //   2. event buffers are cheap
   //
   // one event buffer will be allocated for every front-end frame buffer
   //
   // Each event is associate with a number of framebuffers. Again, since
   //  1. there can't by more triggers than front-end frame buffers and
   //  2. FrameBuffer nodes are cheap
   //
   // one Frame Buffer node will be allocated for every front-end
   // frame buffer.
   // -------------------------------------------------------------------
   Event                 *event = NULL;
   fprintf (stderr,
            "Allocating %d buffers\n", _dataDma._bCount);
   EventPool           eventPool (_dataDma._bCount);
   ////Event                *events = Event::construct (_dataDma._bCount);
   List<FrameBuffer>::Node *fbs = construct_fbs    (_dataDma._bCount,
                                                    _dataDma._map);
   Event::Trigger       trigger;
   bool               trgActive = false;
   uint32_t      softTriggerCnt =     0;


   fprintf (stderr, "EventPool @ %p\n", (void *)&eventPool);

   // -------------------------------------------------------------------
   // The latency list allows the trigger to reach back for buffers
   // that belong to a trigger, but may have been read before the
   // trigger arrived.
   // -------------------------------------------------------------------
   LatencyList latency[MAX_DEST];
   _rxPend = 0;

   fprintf (stderr,
            "Latency Lists @: %p, %p\n",
            (void *)(latency + 0),
            (void *)(latency + 1));

   // ------------------------------------------------------------------
   // Debugging aid for monitoring the rates of various dest DMA buffers
   // ------------------------------------------------------------------
   static uint32_t FreqReceived   = -1;
   static uint32_t FreqTimingDump = -1;
   static uint32_t FreqDataDump   =  1;
   static uint32_t FreqDataCheck  = -1;


   FrameDiagnostics diag (FreqReceived,
                          FreqTimingDump,
                          FreqDataDump,
                          FreqDataCheck);

   MonitorRate               rate;
   MonitorTimes dataTimes   (4096);
   MonitorCorrelation  correlation;


   // -------------------------------------------------------------
   // Setup the list of file descriptors to service with the select
   // -------------------------------------------------------------
   fd_set    fds;
   FD_ZERO (&fds);
   int32_t fdMax = ((_dataDma._fd > _timingDma._fd)
                 ?   _dataDma._fd
                 :   _timingDma._fd)
                 + 1;


   // ------------------
   // Enable the streams
   // ------------------
   bool dataSuccess = enable (_dataDma);
   if (!dataSuccess) return;


   // --------------------------------------------------------
   // Check for bad DMA buffers
   // This cannot be done in the initialization thread because
   // it blocks if the dma channel has not been setup to have
   // data flowing. Of course, the thread that fields command
   // to enable data flowing is also blocked, so deadlock.
   //
   // 2016.10.26 -- jjr
   // -----------------
   // This has been disabled. The error causing the bad buffer
   // has been found and fixed in the DMA driver.  There is
   // still a small issue that one cannot allocate all the
   // data buffers iff the timing/trigger buffer is also
   // enabled. This seems weird since they should be almost
   // completer=ly orthogonal to each other.
   // --------------------------------------------------------
   _dataDma.vet ();
   // --------------------------------------------------------


   // --------------------------------------------------------
   // This is pretty hokey, but need to find the identity of
   // the WIB streams, i.e. the Crate.Slot.Fiber.  Normally
   // this is carried in the data, but for triggers that
   // arrive too late, there is no data, so this one time
   // initialization of the HeaderAndOrigin structure is done.
   // --------------------------------------------------------
   getWibIdentifiers (Srcs, &_dataDma);


   bool timingSuccess = enable (_timingDma);
   if (!timingSuccess < 0) return;

   int timingFd = _timingDma._fd;
   int   dataFd =   _dataDma._fd;

   // Preenable the trigger reception
   FD_SET (timingFd, &fds);

   RunMode         runMode = _config._runMode;
   bool     blowOffDmaData = _config._blowOffDmaData;

   struct timeval prvTime[2];
   gettimeofday (prvTime + 0, NULL);
   gettimeofday (prvTime + 1, NULL);

   // Initialize the contributor mask
   uint32_t ctbs = ((1 << MAX_DEST) - 1);
   fprintf (stderr,
            "Ctbs = %2.2" PRIx32 "\n", ctbs);

   // Run while enabled
   while (_rxThreadEn)
   {
      List<FrameBuffer>::Node *fb = NULL;

      // --------------------------------------------------------
      // Enable the fds of the channels to hang a read on.
      //
      // The timing/trigger channel is selected iff no trigger
      // is active. Since triggers cannot overlap (by decree)
      // then, by definition, only one can be active at any
      // given time.
      //
      // This method will use the DMA buffers to pend triggers that
      // may arrive while an event is being built. This can happen
      // because the trigger and data streams are not synchronous.
      // If the data stream is way behind the trigger stream, while
      // that trigger is waiting for its data to arrive, another
      // trigger could occur. By not enabling the trigger stream
      // for a reading, this next trigger will be pended.
      // --------------------------------------------------------
      FD_SET (   dataFd, &fds);
      ////FD_SET ( timingFd, &fds);  /// !!! KLUDGE to test crash

      // ---------------------------------------------------
      // With the event being freed in the destination queue
      // there is no reason anymore for a timeout.
      // ---------------------------------------------------

      int nfds = select (fdMax, &fds, NULL, NULL, NULL);
      if (nfds > 0)
      {
         // --------------------------------------------------------
         // Can only change the run mode while not event in progress
         // --------------------------------------------------------
         if (event == NULL)
         {
            runMode        = _config._runMode;
            blowOffDmaData = _config._blowOffDmaData;
            ctbs           = ((1 << MAX_DEST) - 1);
         }


         // --------------------------------------------
         // Give priority to the timing/trigger messages
         // --------------------------------------------
         if (FD_ISSET (timingFd, &fds))
         {
            int index;
            TimingMsg const *tmsg = readTimingMsg (&index,
                                                   _timingDma,
                                                   blowOffDmaData,
                                                   runMode,
                                                   trgActive,
                                                   _counters._triggers,
                                                   _counters._trgMsgCnt,
                                                   _counters._disTrgCnt,
                                                   rate,
                                                   diag);

            // --------------------------------
            // Check if have an trigger message
            // --------------------------------
            if (tmsg)
            {
               InCnt [0] = InCnt [1] = 0;
               OutCnt[0] = OutCnt[1] = 0;
               uint64_t trgTimestamp = tmsg->timestamp ();


               // -----------------------------------------------
               // Allocate a new event.
               //  -- This is blocking allocator, so no need to
               //     check that something was allocated
               // Set the timing window
               // Populate it from entries from the latency queue
               // -----------------------------------------------
               event = eventPool.allocate ();

               if (event == NULL)
               {
                  fprintf (stderr, "rxRun:Error: Hardware trigger: No event buffers\n");
                  exit (-1);
                  _timingDma.free (index);
                  trgActive = false;
                  FD_SET (timingFd, &fds);
               }
               else
               {
                  event->m_trigger.init  (tmsg);
                  event->setWindow (trgTimestamp - _config._pretrigger,
                                    trgTimestamp + _config._posttrigger);


                  // ---------------------------------------
                  // Done with the timing message, return it
                  // ---------------------------------------
                  _timingDma.free (index);


                  // -------------------------------------------------
                  // Transfer any data packets on the latency queue
                  // that are within the event window to this event.
                  // While unlikely, it is possible that the latency
                  // queue contains all the packets needed to complete
                  // this event.
                  // -------------------------------------------------
                  int32_t post = event->seedAndDrain (latency, dataFd, ctbs);
                  if (post)
                  {
                     postEventAndReset (_txReqQueue, event, latency);
                     event     = NULL;
                     trgActive = false;

                     // Reenable the trigger stream
                     FD_SET  (timingFd, &fds);
                  }
                  else
                  {
                     // -----------------------------------------
                     // Event is not completed, disable servicing
                     // of the timing/trigger messages until the
                     // event is completed.
                     // -----------------------------------------
                     FD_CLR  (timingFd, &fds);
                     trgActive = true;
                  }
               }
            }
         }


         // ----------------------
         // Check on incoming data
         // ----------------------
         if (FD_ISSET (dataFd, &fds))
         {
            uint32_t   index;
            uint32_t   flags;
            uint32_t    dest;
            int32_t   rxSize;

            //dataTimes.record ();

            rxSize = dmaReadIndex (dataFd, &index, &flags, NULL, &dest);

            if (index >= _dataDma._bCount)
            {
               fprintf (stderr, "rxRun:Error received out of range index= %d > %d\n",
                        index, (int)_dataDma._bCount);
            }


            // -------------------------------------
            // If trigger is not active, reenable it
            // The select cleared it
            // -------------------------------------
            if (!trgActive)
            {
               FD_SET  (timingFd, &fds);
            }


            checkArrivalTimes ();
            rate.monitor       (dest);
            diag.dump_received (index, dest, rxSize);


            // -----------------------------------------
            // If data is from an unknown destination or
            // it is not being serviced, dispose of it.
            // -----------------------------------------
            if ( (dest > (MAX_DEST - 1)) || blowOffDmaData)
            {
               if (dest > (MAX_DEST - 1))
               {
                  fprintf (stderr, "rxRun:Error: Bad destination = %d\n", dest);
               }

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
            uint64_t *data = reinterpret_cast<decltype(data)>
                                       (_dataDma._map[index]);

            #if 0
            {
               uint64_t range[2];
               uint64_t delta;
               struct timeval curTime;


               gettimeofday (&curTime, NULL);
               getTimestampRange (range, data, rxSize);
               delta = range[1] - range[0];
               if (delta != 0x6400)
               {
                  struct timeval elpTime;
                  timersub (&curTime, prvTime + dest, &elpTime);
                  printf ("Dumping[%d.%4d] %16.16"
                          PRIx64 ":%16.16" PRIx64 " %16.16" PRIx64 ""
                          " dt = %6lu.%06lu\n",
                          dest, index,
                          range[1], range[0], delta, elpTime.tv_sec, elpTime.tv_usec);
                  diag.dump_headerFrame (_dataDma._map[index], rxSize, dest);
               }

               prvTime[dest] = curTime;
            }
            #endif

            diag.check_dataFrame (data,
                                  rxSize,
                                  dest,
                                  256);



            checkTimestamps    (data, dest);
            //correlation.report (trigger.m_timestamp, data[2]);


            // Check if there is error
            uint32_t lastUser = axisGetLuser (flags);

            /// !!! KLUDGE !!!
            if (lastUser & DaqDmaDevice::TUserEOFE)
            {
               _counters._rxErrors++;
               _dataDma.free (index);
               continue;
            }

            // --------------------------------------------
            // So far Good data frame :Try to Move the data
            // --------------------------------------------
            uint64_t   *dbeg = data;
            uint64_t   *dend = data + (rxSize - sizeof (*dend)) / sizeof (*data);
            uint32_t tlrSize = *reinterpret_cast<uint32_t const *>(dend - 1)
                             & 0xffffff;

            // ---------------------------------------------------
            // Check if the read size matches the anticipated size
            // ---------------------------------------------------
            if (tlrSize != _rxSize)
            {
               fprintf (stderr,
                        "rxRun:Error: Frame[%d.%d] %16.16" PRIx64 " -> "
                        "%16.16" PRIx64 " %8.8" PRIx32 "\n",
                        dest, index, *dbeg, *dend, _rxSize);

               _counters._rxErrors++;
               _dataDma.free (index);
               continue;
            }


            uint64_t timestampRange[2];
            fb = &fbs[index];

            /**
            if ( ((void *)fb == (void *)&latency[0]) ||
                 ((void *)fb == (void *)&latency[1]) ||
                 (        fb == NULL               )  )
            {
               fprintf (stderr,
                        "rxRun:Error: fb[%d] = %p matches one of the latency lists %p:%p\n",
                        index, (void *)fb, (void *)&latency[0], (void *)&latency[1]);
            }
            **/

            getTimestampRange (timestampRange, data, rxSize);
            fb->m_body.setTimeRange  (timestampRange[0],
                                      timestampRange[1]);

            int64_t dt = timestampRange[1] - timestampRange[0];
            if (dt != TimingClockTicks::PER_FRAME)
            {
               fb->m_body.addStatus (FrameBuffer::Missing);
            }


            // Set the size, the two trailer words are included in this size
            fb->m_body.setSize       (rxSize);
            fb->m_body.setRxSequence (_rxSequence++);

#if 0
            {
               static int Count = 2000;
               if (--Count <= 0)
               {
                  fprintf (stderr,
                           "RunMode = %d  trgActive = %d\n",
                           (int)runMode, trgActive);
                  Count = 2000;
               }
            }
#endif


            if (runMode   == RunMode::SOFTWARE &&
                trgActive == false)
            {
               // Check if this is a soft trigger
               int64_t triggerTime = SoftTrigger::check (timestampRange,
                                                         _config._period);

               if (triggerTime >= 0)
               {
                  /**
                  fprintf (stderr,
                           "rxRun: Software trigger active[%d] = %16.16" PRIx64 "\n",
                           dest,
                           triggerTime);

                  fprintf (stderr,
                           "rxRun: Time Range: %16.16" PRIx64 " :  %16.16" PRIx64 "\n",
                           timestampRange[0],
                           timestampRange[1]);
                  **/

                  event = eventPool.allocate ();
                  if (event == NULL)
                  {
                     fprintf (stderr,"rxRun: Software trigger: No event buffers\n");
                     exit (-1);
                  }

                  event->m_trigger.init (triggerTime,
                                         softTriggerCnt++, 0);
                  event->setWindow (triggerTime - _config._pretrigger,
                                    triggerTime + _config._posttrigger);

                  latency[dest].check (fb, 12, "rxRun:seedAndDrain (before)");
                  int32_t post = event->seedAndDrain (latency, dataFd, ctbs);
                  latency[dest].check (fb, 12, "rxRun:seedAndDrain (after)");

                  /**
                  fprintf (stderr,
                           "rxRun: Software trigger action = %2.2" PRIx32 "\n", post);
                  **/

                  if (post)
                  {
                     /**
                     fprintf (stderr, "rxRun::postEventAndReset[%d]\n", dest);
                     **/

                     latency[dest].check (fb, 12, "rxRun:postEventAndReset (before)");
                     postEventAndReset (_txReqQueue, event, latency);
                     latency[dest].check (fb, 12, "rxRun:postEventAndReset (after)");
                     event     = NULL;
                     trgActive = false;

                     // Reenable the trigger stream
                     FD_SET  (timingFd, &fds);
                     continue;
                  }
                  else
                  {
                     trgActive = true;
                  }
               }
            }


            if (trgActive)
            {
               latency[dest].check (fb, 12, "rxRun:evaluate (before)\n");
               int32_t action = event->evaluate (fb, dest);
               latency[dest].check (fb, 12, "rxRun:evaluate (after )\n");

               /**
               {
                  fprintf (stderr, "rxRun: Adding[%d] %d fb = %p event = %p"
                           "action = %2.2" PRIx32 "\n",
                           dest, InCnt[dest]++, (void *)fb, (void *)event,
                           action);
               }
               **/


               if (action & Event::ACTION_M_POST)
               {
                  /**
                  fprintf (stderr, "rxRun: Posting event = %p\n", (void *)event);
                  **/
                  postEventAndReset (_txReqQueue, event, latency);
                  trgActive   = false;
                  event       = NULL;

                  // Reenable the trigger
                  ///fprintf (stderr, "Rearming the trigger 2\n");
                  FD_SET  (timingFd, &fds);
               }

               if (action & Event::ACTION_M_FREE)
               {
                  _dataDma.free (index);
                  continue;
               }

               if (action & Event::ACTION_M_ADDTOLATENCY)
               {
                  /**
                  fprintf (stderr,
                           "rxRun: [%d] AddToLatency(before) %p\n",
                           dest, (void *)fb);
                  **/
                  latency[dest].check (fb, 12, "AddToLatency(before)");
                  latency[dest].replace (fb, dataFd);
                  continue;
               }

            }
            else
            {
               /**
               if ( ((void *)fb == (void *)&latency[0]) ||
                    ((void *)fb == (void *)&latency[1]) ||
                    (        fb == NULL               )  )
               {
                  fprintf (stderr,
                           "rxRun:Error: Latency[%d] @ %p fbs[%d] @ %p remaining = %d\n",
                           dest, &latency[dest], index, fb, latency[dest].m_remaining);
               }
               **/
               latency[dest].replace (fb, dataFd);
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
      [(int)fragment::Type::Reserved_0   ] = "Reserved_0",
      [(int)fragment::Type::Control      ] = "Control",
      [(int)fragment::Type::Data         ] = "Data",
      [(int)fragment::Type::MonitorSync  ] = "MonitorSync",
      [(int)fragment::Type::MonitorUnSync] = "MonitorUnSync"
   };

   static const char *RecType[] =
   {
    [(int)fragment::Header<fragment::Type::Data>::RecType::Reserved_0] = "Reserved_0",
    [(int)fragment::Header<fragment::Type::Data>::RecType::Originator] = "Originator",
    [(int)fragment::Header<fragment::Type::Data>::RecType::TpcNormal ] = "TpcNorma",

    [(int)fragment::Header<fragment::Type::Data>::RecType::TpcDamaged] = "TocDamaged"
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
   if (type == (int)fragment::Type::Data)
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
static void originator_dump (pdd::fragment::Originator const &originator)
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
      fragment::OriginatorBody const &body   = HeaderOrigin.m_origin.m_body;

      fragment::Version const &version = body.version   ();
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
      List<FrameBuffer>      const      *list = &event->m_list[idx];
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
         uint32_t      nbytes = node->m_body.getReadSize ();
         uint8_t         *ptr = node->m_body.getBaseAddr ();
         unsigned int   index = node->m_body.getIndex    ();

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
static void ranges_dump (pdd::fragment::tpc::Ranges const *ranges,
                         int rangeSize)
{
   int           n64 = rangeSize / sizeof (uint64_t);
   uint64_t const *p = reinterpret_cast <decltype(p)>(ranges);

   fprintf (stderr, "Range dump:  n64 = %d\n", n64);
   for (int idx = 0; idx < n64; ++idx)
   {
      fprintf (stderr,
               "p[%2d] = %16.16" PRIx64 "\n", idx, p[idx]);
   }

   return;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define    header0_dump(_header0)
#define originator_dump(__originator)
#define   fragment_dump(_header)
#define     ranges_dump(_ranges, _rangeSize)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if 0
/* ---------------------------------------------------------------------- */
static void node_dump (List<FrameBuffer> const       *list,
                       List<FrameBuffer>::Node const *node,
                       int                            ictb,
                       int                         toc_idx,
                       uint32_t                     nbytes)
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
            node->m_body.getIndex (),
            nbytes);

   return;
}
/* ---------------------------------------------------------------------- */
#else
#define       node_dump(_list, _node, _ictb, _toc_idx, _nbyte)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
#     if 0
/* ---------------------------------------------------------------------- */
static void msgvec_dump (struct msghdr const *msg)
{

   unsigned tot = 0;
   for (unsigned int idx = 0; idx < msg->msg_iovlen; idx++)
   {

      int             len = msg->msg_iov[idx].iov_len;
      uint64_t const *p64 = reinterpret_cast<decltype (p64)>
         (msg->msg_iov[idx].iov_base);

      tot += len;

      fprintf (stderr,
               "Iov[%2d.%4u:%6.6x] = %16.16" PRIx64 " %16.16" PRIx64 " "
               "%16.16" PRIx64 " %16.16" PRIx64 "\n",
               idx, len, tot, p64[0], p64[1], p64[2], p64[3]);


      void *ptr = msg->msg_iov[idx].iov_base;
      fprintf (stderr,
               "msg[%2d] %p:%8.8x %8.8x\n", idx, ptr, len, tot);
   }

   return;
}
/* ---------------------------------------------------------------------- */
# else
#define msgvec_dump(_msg);
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
#define EVENT_ANNOUNCE 1
/* ---------------------------------------------------------------------- */
#if     EVENT_ANNOUNCE
/* ---------------------------------------------------------------------- */
static inline void event_announce (HeaderAndOrigin const &headerOrigin,
                                   int                           nctbs,
                                   int                           npkts)
{
   fprintf (stderr, "Header.m_64 %16.16" PRIx64 " "
            "Identifier.m_64 = %16.16" PRIx64 " ts = %16.16" PRIx64 " "
            "Nctbs:Npackets = %u:%3u\n",
            headerOrigin.m_header.m_w64,
            headerOrigin.m_header.m_identifier.m_w64,
            headerOrigin.m_header.m_identifier.m_timestamp,
            nctbs, npkts);
   return;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define event_announce(_headerOrigin, _nctbs, _npkts)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if STREAM_DUMP
/* ---------------------------------------------------------------------- */
static void stream_dump (uint32_t *streamRecord, Event const *event)
{
   uint32_t (*streamStart)[MAX_TPCSTREAMSIZE]
      = reinterpret_cast<decltype(streamStart)>(streamRecord);

   for (int ictb = 0; ictb < event->m_nctbs; ictb++)
   {
      uint32_t const *stream = streamStart[ictb];
      for (unsigned int idx = 0; idx < 32; idx += 4)
      {
         fprintf (stderr,
                "Stream[%1d][%2d] = "
                " %8.8" PRIx32 " %8.8" PRIx32 " %8.8" PRIx32 " %8.8" PRIx32
                " %8.8" PRIx32 " %8.8" PRIx32 " %8.8" PRIx32 " %8.8" PRIx32 "\n",
                 ictb, idx,
                 stream[idx+0],  stream[idx+1], stream[idx+2], stream[idx+3],
                 stream[idx+4],  stream[idx+5], stream[idx+6], stream[idx+7]);
      }
   }

   return;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define stream_dump(_streamRecord, _event)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */






typedef TxMessage<32> TxMsg;

static  inline uint32_t addHeaderOrigin (TxMsg               *msg,
                                         HeaderAndOrigin  *hdrOrg,
                                         uint32_t           hoIdx);


static inline void completeHeader (pdd::fragment::Header<pdd::fragment::Type::Data>
                                                         *header,
                                   Event const            *event,
                                   uint32_t               status,
                                   uint32_t               txSize);

static uint32_t   addContributors (TxMsg                          *msg,
                                   Event  const                 *event,
                                   pdd::fragment::tpc::Stream  *stream,
                                   void                  **nextAddress,
                                   uint32_t                 *retStatus,
                                   uint32_t                     *mctbs);

static int       addTpcDataRecord (TxMsg                          *msg,
                                   pdd::fragment::tpc::Stream  *stream,
                                   int                            ictb,
                                   uint16_t                        csf,
                                   unsigned int              ctbs_left,
                                   Event const                  *event,
                                   List<FrameBuffer> const       *list,
                                   void                 **nextAddress);


/* ---------------------------------------------------------------------- *//*!

   \brief  Waits on incoming data to be transmitted to a client machine
                                                                          */
/* ---------------------------------------------------------------------- */
void DaqBuffer::txRun ()
{
   // ------------------------------------------------------------------------
   // !!! KLUDGE !!!
   // The event queue removal demands a timeout, but there is no need for one.
   // As a kludge, just set it to some very long time
   // The underlying queue routines need to have a no-wait version implemented.
   // ------------------------------------------------------------------------
   static const uint32_t EventWaitTime = 100*1000000;


   // -----------------------------------
   // Construct the transmitter class
   // This contains enough information to
   // send the data via TCP/IP or RSSI
   // -----------------------------------
   TxMsg txMsg (&_txServerAddr);


   // ---------------------------
   // Wait on the incoming frames
   // ---------------------------
   while ( _txThreadEn )
   {
      // --------------------------------------------------------
      // Wait for data in the pend buffer
      // The timeout has been all but eliminated, no need for it
      // --------------------------------------------------------
      Event *event = reinterpret_cast<decltype (event)>
                     (_txReqQueue->pop(EventWaitTime));
      if (event == NULL) continue;


      // ------------------------------------------------------------------
      // Allocate and fill in the static information of the HeaderAndOrigin
      // ------------------------------------------------------------------
      uint32_t    hoIndex = _dataDma.allocateWait ();
      HeaderAndOrigin *ho = reinterpret_cast<decltype (ho)>
                           (_dataDma._map[hoIndex]);
      *ho = HeaderOrigin;

      //memcpy (ho, &HeaderOrigin, HeaderOrigin.n64 () * sizeof (uint64_t));


      // ----------------------------------------------------------------------
      // Fragment header + Origination Information
      //
      // This would be static except in the case of no contributors.
      // In this case, the size of the trailer will be tacked onto this msg_iov
      // ----------------------------------------------------------------------
      size_t txSize = addHeaderOrigin (&txMsg, ho, hoIndex);


      // -----------------------------------------------------------------------
      // The trailer address is initialized to be at the end of the HeaderOrigin
      // structure.The trailer is normally tacked on to the end of the last
      // contributor's packet. This covers the empty event case when there are
      // no contributor data.
      //
      // Set the Tpc Contributions to start at iov 1
      // The Tpc Record Header, TOC Record and Range Record are placed in the
      // same memory as the HeaderOrigin.  This means that the write blocks
      // allocated in the driver must be large enough to hold this information.
      // Given that it is very modest in size (200-500 bytes) this is not 
      // onerous to arrange.
      // -----------------------------------------------------------------------
      uint32_t contributorSize;
      uint32_t           mctbs;
      uint32_t          status;
      pdd::Trailer    *trailer = &HeaderOrigin.getTrailer ();
      pdd::fragment::tpc::Stream *streamRecord = ho->getStreamRecord ();
      txMsg.setIovlen (1);
      txSize += contributorSize = addContributors (&txMsg,
                                                   event,
                                                   streamRecord,
                                                   (void **)&trailer,
                                                   &status,
                                                   &mctbs);

      txSize += sizeof (pdd::Trailer);


      // ------------------------------------
      // Complete the header/orginator
      // with the values that were not known
      // until the event was fully formatted.
      // ------------------------------------
      completeHeader  (&ho->m_header, event, status, txSize);
      header0_dump    (&ho->m_header);
      originator_dump (&ho->->m_origin);


      // --------------------------------------------
      // Construct the event fragment trailer
      // The last 64-bit word, containing the
      // firmware trailer is usurped for the trailer.
      // --------------------------------------------
      trailer->construct (ho->m_header.retrieve ());
      txMsg.increaseLast (sizeof (*trailer));


      // -------------------------------------------------
      // Each completed packet consists of 
      //    1) A header record 
      //    2) The one iov for each contribuor packet
      // So the total number of data packets is just the
      // number of iov's - 1.
      // -------------------------------------------------
      event_announce (*ho, event->m_nctbs, txMsg.getIovlen () - 1);


      // -----------------------------------
      // If inhibited, just free the event
      // -----------------------------------
      if (_config._blowOffTxEth)
      {
        _dataDma.free (     hoIndex);
         event->free  (_dataDma._fd);
      }
      else
      {
         // -------------------------------------------------------------------
         // Send the data and check that it was all sent
         // Note::
         // 1. The RSSI transfer frees the buffer so only free on Tcp transfer.
         // 2. The type of transfer must be captured because _enableRssi can be
         //    asynchrounously updated.
         // -------------------------------------------------------------------
         size_t     ret;
         bool enableRssi = _config._enableRssi;
         if (enableRssi)
         {
            ret = txMsg.sendRssi (_dataDma._fd, txSize);
            event->free ();
         }
         else
         {
            ret = txMsg.sendTcp  (_txFd,        txSize);
            _dataDma.free (     hoIndex);
            event->free   (_dataDma._fd);
         }

         bool sent = (ret == txSize);


         // ---------------------
         // Record the statistics
         // ---------------------
         if (sent)
         {
            // Successfully sent
            size_t  currSeqId = event->m_trigger.m_sequence;
            size_t deltaSeqId = currSeqId - _txSeqId;
            if (_counters._txCount > 0 && deltaSeqId > 1)
            {
               _counters._dropSeqCnt += deltaSeqId - 1;
            }

            _txSeqId            = currSeqId;
            _counters._txCount += 1;
            _txSize             = txSize;
            _counters._txTotal += txSize;
         }
         else
         {
            // Unsuccessful,
            _counters._txErrors++;
            if (!enableRssi)
            {
               fprintf (stderr,
                        "Send error (%zx, disabling transmission\n", ret);
               stream_dump (streamRecord, event);
               _config._blowOffTxEth = 1;
            }
         }
      }
   }

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
static uint32_t getIndex (List<FrameBuffer>::Node const *node,
                          uint64_t                        win,
                          uint16_t                     pktIdx);


static unsigned int addRanges (pdd::fragment::tpc::Ranges *ranges,
                               int                           ictb,
                               Event                const  *event,
                               List<FrameBuffer>    const   *list)
{
   uint64_t winTrg = event->m_trigger.m_timestamp;
   uint64_t winBeg = event->m_limits.m_beg;
   uint64_t winEnd = event->m_limits.m_end;

   /**
   fprintf (stderr, "Event @ %p trigger = %16.16" PRIx64 "\n",
            (void *)event, winTrg);
   **/

   // ------------------------------------------------------------------
   // Seed the times for the trigger and beginning/ending event window
   // Since this is defined by the trigger and there is only one trigger
   // this is a not potential per contributor set of values.
   // ------------------------------------------------------------------
   ranges->m_body.m_window.construct (winBeg, winEnd, winTrg);

   auto    *dsc = &ranges->m_body.m_dsc;


   // -------------------------------
   // Locate the first and last nodes
   // -------------------------------
   List<FrameBuffer>::Node const *first = list->m_flnk;
   List<FrameBuffer>::Node const  *last = list->m_blnk;
   List<FrameBuffer>::Node const   *trg = event->m_trgNode[ictb];


   // -------------------------------------------------------------
   // Calculate the index to the first, last and trigger timesamples
   // in the window
   // WARNING: This assumes that this occur in the specified nodes.
   // -------------------------------------------------------------
   uint32_t idxBeg = getIndex (first, winBeg, 0);
   //fprintf (stderr, "\nBegin   index[%d] %8.8" PRIx32 "\n", ictb, idxBeg);

   uint32_t idxEnd = getIndex (last,  winEnd, event->m_npkts[ictb] - 1);
   //fprintf (stderr, "\nEnd     index[%d] %8.8" PRIx32 "\n", ictb, idxEnd);

   uint32_t idxTrg;
   if (trg)
   {
      idxTrg = getIndex (trg,   winTrg, event->m_trgNpkt[ictb]);
   }
   else
   {
      // --------------------------------
      // No trigger node was found, error
      // --------------------------------
      idxTrg = -1;
   }
   //fprintf (stderr, "\nTrigger index[%d] %8.8" PRIx32 "\n", ictb, idxTrg);


   uint64_t evtBeg = first->m_body._ts_range[0];
   uint64_t evtEnd =  last->m_body._ts_range[1];


   dsc->construct (idxBeg,
                   idxEnd,
                   idxTrg,
                   evtBeg,
                   evtEnd);

   int n64        = ranges->n64 ();
   ranges->construct (n64);

   return n64 * sizeof (uint64_t);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief  Find the index of the WIB frame associated with the trigger.
   \return An integer containing two bit fields, one for the packet index
           and one for the index of WIB frame.  This combination uniquely
           identifies the WIB frame sample of the trigger.

   \param[in]   node The node containing the trigger
   \param[in]    win The trigger time
   \param[in] pktIdx The index of the packet associated with the trigger

   \warning
    This routine assumes that all WIB frames are present and will not work
    if frames have been dropped.
                                                                          */
/* ---------------------------------------------------------------------- */
static uint32_t getIndex (List<FrameBuffer>::Node const *node,
                          uint64_t                        win,
                          uint16_t                     pktIdx)
{
   uint64_t        pktBeg = node->m_body._ts_range[0];
   uint64_t        pktEnd = node->m_body._ts_range[1];

   if (node == NULL)
   {
      fprintf (stderr, "Error node = NULL\n");
      return -1;
   }

   /*
    * fprintf (stderr,
    *        "PktBeg:End[%d] "
    *        "%16.16" PRIx64 ":%16.16" PRIx64 " %16.16" PRIx64 " node: %p\n",
    *        pktIdx, pktBeg, pktEnd, win, node);
    */


   if (win < pktBeg || win > pktEnd)
   {
      fprintf (stderr,
               "getIndex: Target not within packet %16.16" PRIx64 " : %16.16"
               "" PRIx64  ": %16.16" PRIx64 "\n",
               pktBeg, win, pktEnd);
      return -1;
   }
   else
   {
      uint16_t idx = (win - pktBeg) / TimingClockTicks::PER_SAMPLE;
      {
         /*
          * uint64_t const *p64  = node->m_body.getBaseAddr64 ();
          * uint64_t const *pBeg = p64 + 30 * idx + 2;
          * uint64_t       tsPkt = pBeg[0];
          * fprintf (stderr,
          *        "Idx: %6d Beg: %16.16" PRIx64 " vs %16.16" PRIx64 "\n",
          *        idx, win, tsPkt);
          */
      }

      return pdd::fragment::tpc::RangesBody::Descriptor::
             Indices::index (pktIdx, idx);

   }

}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
static inline uint32_t addHeaderOrigin (TxMsg              *msg,
                                        HeaderAndOrigin *hdrOrg,
                                        uint32_t          hoIdx)
{
   uint32_t nbytes = hdrOrg->n64 () * sizeof (uint64_t);

   msg->add (0, hdrOrg, hoIdx, nbytes, RssiIovec::First);

   return nbytes;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static uint32_t  addContributors (TxMsg                         *msg,
                                  Event  const                *event,
                                  pdd::fragment::tpc::Stream *stream,
                                  void                 **nextAddress,
                                  uint32_t                *retStatus,
                                  uint32_t                 *retMctbs)
{
   unsigned int  ctbs = event->m_ctbs;
   unsigned int nctbs = event->m_nctbs;
   unsigned int mctbs = nctbs;
   uint32_t   ctbSize = 0;
   uint32_t    status = 0;


   // ------------------------------------------------
   // Capture the frames from each incoming HLS source
   // ------------------------------------------------
   while (ctbs)
   {
      int                                ictb = ffs (ctbs) - 1;
      List<FrameBuffer>       const     *list = &event->m_list[ictb];

      // ----------------------------------------
      // Eliminate this contributor from the list
      // ----------------------------------------
      ctbs &= ~(1 << ictb);

      if (list->is_empty ())
      {
         // -------------------------------------------
         // Reduce the number of non-empty contributors
         // -------------------------------------------
         mctbs -= 1;
         fprintf (stderr, "Empty contributor %d\n", ictb);
      }
      else
      {
         // ---------------------------------------------
         // Get the crate.slot.fiber for this contributor
         // ---------------------------------------------
         uint16_t csf = list->m_flnk->m_body.getCsf ();
         nctbs       -= 1;
         ///uint16_t csf = ictb ? 0x123 : 0x456;


         ctbSize += addTpcDataRecord (msg,   stream,   ictb,          csf,
                                      nctbs,  event,   list, nextAddress);


         // -----------------------------------------------------
         // Accumulate any status bits from this TpcStream record
         // -----------------------------------------------------
         status |= stream->getStatus ();
      }

      //streamStart += 1;
      stream = reinterpret_cast<decltype (stream)>(*nextAddress);
   }

   *retMctbs  =  mctbs;
   *retStatus = status;
   return      ctbSize;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Adds the data record for one contributor
  \return The number of bytes in the constructed data record

  \param[in]       msg The message vector with the various pieces of data
                       that will the transported by sendmsg
  \param[in]    stream Buffer to hold the non-TPC data portion of the TPC
                       data stream record. This includes the
                         - TPC data stream record header
                         - Ranges record
                         - Table of Contents record
                         - TPC packet record
  \param[in]      ictb Which contributor
  \param[in]       csf The packed Crate.Slot.Fiber identifier
  \param[in] ctbs_left The number of contributors left to go
  \param[in]      list The list of data frame buffers for this contributor
  \param[in]     event Global information of the event
                                                                          */
/* ---------------------------------------------------------------------- */
static int addTpcDataRecord (TxMsg                          *msg,
                             pdd::fragment::tpc::Stream  *stream,
                             int                            ictb,
                             uint16_t                        csf,
                             unsigned int              ctbs_left,
                             Event const                  *event,
                             List<FrameBuffer> const       *list,
                             void                  **nextAddress)
{
   using namespace   pdd;
   uint32_t    ndata = 0;
   int       pkt_idx = 0;


   // ----------------------------------
   // Locate where the range record goes
   // ----------------------------------
   pdd::fragment::tpc::Ranges
                      *ranges = reinterpret_cast<decltype (ranges)>(&stream[1]);


   // -------------------------------------------------
   // This define the trimmed and untrimmed data ranges
   // -------------------------------------------------
   int rangeSize = addRanges (ranges, ictb, event, list);
   ranges_dump (ranges, rangeSize);


   // -------------------------------------------------------
   // Reserve enough memory to hold the TOC and range records
   // -------------------------------------------------------
   fragment::tpc::Toc<MAX_PACKETS>
                 *toc = reinterpret_cast<decltype (toc)>
                       (reinterpret_cast<uint8_t *>(ranges) + rangeSize);

   auto   toc_pkt    = toc->packets   ();
   auto   msg_iovlen = msg->getIovlen ();
   auto   prv_iovlen = msg_iovlen - 1;

   int                               npkts = 0;
   uint8_t                          status = 0;
   List<FrameBuffer>::Node const     *node = list->m_flnk;
   List<FrameBuffer>::Node const *terminal = list->terminal ();


   uint32_t nbytes;
   uint64_t   *p64;

   // -----------------------------------------------------
   // Grab all the packets associated with this contributor
   // -----------------------------------------------------
   while (node != terminal)
   {
      // --------------------------------------------------
      // Get the size of the data to be written,
      // This excludes any transport headers and trailers.
      // --------------------------------------------------
                   nbytes = node->m_body.getWriteSize  ();
                      p64 = node->m_body.getBaseAddr64 ();
      uint32_t      index = node->m_body.getIndex      ();
      int      dataFormat = node->m_body.getDataFormat ();
      status             |= node->m_body.getStatus     ();


      // ------------------------------------------------------
      // Setup buffer pointers and size
      // Any control information from the HLS stream is not
      // transported as is, but captured in the TOC.
      // ------------------------------------------------------
      msg->add (msg_iovlen, p64, index, nbytes, RssiIovec::Middle);
      msg_iovlen += 1;

      /*
      fprintf (stderr,
               "Pkt[%2d] = %16.16" PRIx64 " %16.16" PRIx64 " "
               "%16.16" PRIx64 " %16.16" PRIx64 " %4.4" PRIx16 "\n",
               pkt_idx, p64[0], p64[1], p64[2], p64[3], csf);
      */


      // -----------------------------------------------------
      // Construct the table of contents entry for this packet
      // -----------------------------------------------------
      toc_pkt[pkt_idx].construct (dataFormat, 0, ndata/sizeof (uint64_t));

      /**
      fprintf (stderr, "Toc entry: %8.8" PRIx32 "\n", toc_pkt[pkt_idx].m_w32);
      **/

      node_dump (list, node, ictb, pkt_idx, nbytes);


      pkt_idx += 1;             // Bump packet index
      npkts   += 1;             // Bump number packets, this contributor
      ndata   += nbytes;        // Running count of data size
      node     = node->m_flnk;  // Next node
   }



   // --------------------------------------------------
   // Set the next available
   // This is used to tack on either 
   //   1) The next stream record's header information
   //   2) The fragment trailer
   // --------------------------------------------------
   *nextAddress = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(p64) 
                                        + nbytes);

   /*
     fprintf (stderr, "Event:packet count[%d] = %d vs %d\n",
     ictb, event->m_npkts[ictb], npkts);
   */


   // ---------------------------------------------------------
   // 2017.10.12 -- jjr
   // ----------------
   // Changed [pkt_idx] -> [pkt_idx++] so that pkt_idx includes
   // the terminating index as part of its count.
   //
   // Add the terminating index so the length
   // of the final frame can be determined.
   // ---------------------------------------------------------
   toc_pkt[pkt_idx++].construct  (0, 0, ndata/sizeof (uint64_t));


   ///fprintf (stderr, "Srcs %4.4" PRIx16 " %4.4" PRIx16 " %8.8" PRIx32 "\n",
   ///         srcs[0], srcs[1], ndata);


   // --------------------------------------------------
   // Complete the construction of the table of contents
   // First compute its total size, in bytes
   // --------------------------------------------------
   uint32_t toc_nbytes   = toc->nbytes   (pkt_idx);
   uint32_t toc_n64bytes = toc->n64bytes (pkt_idx);

   /**
   fprintf (stderr,
            "Toc 0:nctbs=%2d npkts=%2d nbytes=%4d n64=%4d\n",
            ictb, pkt_idx, toc_nbytes, toc_n64bytes);
   **/

   /*
    | 2017.10.19 - jjr
    | ----------------
    | The number of packets does not include the terminating packet
   */
   toc->construct (pkt_idx - 1, toc_n64bytes/sizeof (uint64_t));

   // -------------------------------------------
   // Pad to a 64-bit boundary
   // -------------------------------------------
   uint32_t   *p32 = reinterpret_cast<uint32_t *>((char *)toc + toc_nbytes);

   if ( (toc_nbytes & 0x7) != 0)
   {
      // ----------------------------------------
      // Not on an even 64-bit boundary,
      // Add a padding word and round up size up
      // ----------------------------------------
      p32[0]      = 0;
      p32        += 1;
      toc_nbytes += sizeof (p32[0]);
   }


   // ----------------------------------------------------------------
   // 2017.10.12 -- jjr
   // -----------------
   // Data packet length did not include the size of the packet header
   // Added this on.
   //
   // Add the data packet header
   // This immediately follows the TOC
   // ----------------------------------------------------------------
   void              *pkt_ptr = reinterpret_cast<char *>(toc) + toc_nbytes;
   fragment::tpc::Packet *pkt = reinterpret_cast<decltype (pkt)>(pkt_ptr);
   uint              pktLen64 = (sizeof (Header1) +
                     ndata + sizeof (uint64_t) - 1)/sizeof (uint64_t);
   pkt->construct (pktLen64);


   /**
   fprintf (stderr,
            "Toc 3:nctbs=%2d npkts=%2d nbytes=%4d n64=%4d\n",
            ictb, pkt_idx, toc_nbytes, toc_n64bytes);
   **/


   // ------------------------------------------------------------------------
   // Calculate length of the locally generate portion of the TPC data record.
   // This includes the lengths of
   //   1. The TPC data record header
   //   2. The Range Record
   //   3. The Table of Contents
   //   4. The TPC packet header
   //
   // The calculate the total length to the TPC data reord as the sum of
   //   1. The local tpc data size +
   //   2. The size from the data vectors
   // -----------------------------------------------------------------------
   uint32_t streamLclSize = sizeof (fragment::tpc::Stream)
                          + rangeSize
                          + toc_n64bytes
                          + sizeof (fragment::tpc::Packet);
   uint32_t streamSize    = streamLclSize + ndata;


   /**
   fprintf (stderr,
            "Tpc Record size: %8.8" PRIx32 " : %8.8" PRIx32 "\n"
           "         header: %8.8" PRIx32 " : %8.8" PRIx32 "\n"
           "          range: %8.8" PRIx32 " : %8.8" PRIx32 "\n"
           "            toc: %8.8" PRIx32 " : %8.8" PRIx32 "\n"
           "    data header: %8.8" PRIx32 " : %8.8" PRIx32 "\n",
           streamSize,            streamSize/sizeof (uint64_t),
           sizeof (fragment::tpc::Stream),
           sizeof (fragment::tpc::Stream)/sizeof(uint64_t),
           rangeSize,
           rangeSize/sizeof(uint64_t),
           toc_n64bytes,
           toc_n64bytes/sizeof(uint64_t),
           sizeof (fragment::tpc::Packet),
           sizeof (fragment::tpc::Packet)/sizeof(uint64_t));
   **/


   // --------------------------------
   // Add the Tpc Stream Record Header
   // --------------------------------
   uint32_t bridge = pdd::fragment::tpc::Stream::Bridge::
                     compose (csf, ctbs_left, status);


   // --------------------------------
   // Construct the Data Packet Header
   // --------------------------------
   int recType = (status == 0)
               ? static_cast<decltype (recType)>
                (fragment::Header<fragment::Type::Data>::RecType::TpcNormal)
               : static_cast<decltype (recType)>
                (fragment::Header<fragment::Type::Data>::RecType::TpcDamaged);

   stream->construct (recType, streamSize/sizeof (uint64_t), bridge);


   // ----------------------------------------------------
   // Fill in the message vector for the locally generated
   // portion of the data record. This gets fill in on the
   // first iov.
   // ----------------------------------------------------
   //msg->add (org_iovlen, stream, streamLclSize, RssiIovec::Middle);
   msg->increase  (prv_iovlen, streamLclSize);
   msg->setIovlen (msg_iovlen);

   // ------------------------------------
   // Return the length of the data record
   // ------------------------------------
   return streamSize;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static inline void completeHeader (pdd::fragment::
                                   Header<pdd::fragment::Type::Data> *header,
                                   Event const                        *event,
                                   uint32_t                           status,
                                   uint32_t                           txSize)

{
   // -----------------------------------------------------------------
   // If any error status bits have been accumulated, declare this data
   // packet as Damaged
   // -----------------------------------------------------------------
   pdd::fragment::Header<pdd::fragment::Type::Data>::RecType
   recType = (status == 0)
           ?  pdd::fragment::Header<pdd::fragment::Type::Data>::RecType::TpcNormal
           :  pdd::fragment::Header<pdd::fragment::Type::Data>::RecType::TpcDamaged;


   // -----------------------------------------------------------------------
   //
   // Construct the event fragment header
   //
   // 2017.12.07 - jjr
   // ----------------
   // The WIB crate.slot.fiber used in the header is now seeded at
   // initialization time. This must be done this way in case the first
   // triggers have no data associated with them. This is a real possiblity
   // because the trigger data streams are in an unknown state at startup.
   //
   // The downside of this is that, while unlikely, the identifier from
   // this event's data may differ from that at initialization. The odds
   // of this are low, since this would be either
   //
   //    a) A data corruption in the WIB frame
   //    b) Someone moved the fiber while data
   //       was being taken and somehow the system
   //       did not curl up in a ball and die
   // -----------------------------------------------------------------------
   header->construct (recType,
                      event->m_trigger.m_source,
                      Srcs[0],
                      Srcs[1],
                      event->m_trigger.m_sequence,
                      event->m_trigger.m_timestamp,
                      txSize / sizeof (uint64_t));

   return;
}
/* ---------------------------------------------------------------------- */




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
   \param[in]  enableRssi     A flag for selecting FW RSSI or SW TCP
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
                           uint32_t     enableRssi,
                           uint32_t     pretrigger,
                           uint32_t       duration,
                           uint32_t         period)
{
   _config._blowOffDmaData  = blowOffDmaData;
   _config._blowOffTxEth    = blowOffTxEth;
   _config._enableRssi      = enableRssi;
   _config._pretrigger      = TimingClockTicks::from_usecs (pretrigger);
   _config._posttrigger     = TimingClockTicks::from_usecs (duration - pretrigger);
   _config._period          = TimingClockTicks::from_usecs (period);
}
/* ---------------------------------------------------------------------- */

// Set run mode
void DaqBuffer::setRunMode ( RunMode mode ) {
   _config._runMode        = mode;
}
