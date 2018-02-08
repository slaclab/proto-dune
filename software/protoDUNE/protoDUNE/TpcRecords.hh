// -*-Mode: C++;-*-

#ifndef PDD_TPCRECORDS_HH
#define PDD_TPCRECORDS_HH

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     TpcRecords.hh
 *  @brief    Proto-Dune Data Tpc Data Records
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
 *  pdd
 *
 *  @author
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  <2017/08/07>
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
   2018.02.06 jjr Added a status field to the bridge word.  This is used to
                  accumulate status and error bits.
   2017.08.29 jjr Stripped debugging print statements.
   2017.08.07 jjr Created
  
\* ---------------------------------------------------------------------- */

#include "Headers.hh"

namespace pdd      {
namespace fragment {
namespace tpc      {


/* ---------------------------------------------------------------------- *//*!

  \class Stream
  \brief The TPC data record for a single fiber stream

   This may include any of the following records

     -# Table of Contents, 
        this provides a description and the location of each TPC data
        packet. It allows random location of both the first packet in
        the HLS streams and the packets within that stream.  Future
        versions will allow random location of the channels within the
        packets
    -#  Error Record
        This is an optional/as needed record describing any error
        conditions
    -#  The Tpc Data packets.

                                                                          */
/* ---------------------------------------------------------------------- */
class Stream : public Header1
{
private:
   static const int Version = 0;

public:
    /* TPC Data Record Types */
    enum class RecType
    {
       Reserved = 0,  /*!< Reserved                                       */
       Toc      = 1,  /*!< Table of Contents record                       */
       Ranges   = 2,  /*!< Event Window record                            */
       Packets  = 3   /*!< Data packets                                   */
    };


   class Bridge
   {
   public:
      Bridge (uint32_t csf, uint32_t left, uint8_t status) : 
              m_w32 (compose (csf, left, status))
      {
         return;
      }


   public:
      /* ---------------------------------------------------------------- *//*!

         \brief Size of the bit fields of the bridge word
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Size
      {
         Format    =  4,  /*!< Size of the bridge word's format field     */
         Csf       = 12,  /*!< Size of the Crate.Slot.Fiber field         */
         Left      =  8,  /*!< Size of the number of Tpc Records left     */
         Status    =  8,  /*!< Size of the status field                   */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

         \brief Right justified offsets of the bit fields of the bridge
                word
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Offset
      {
         Format   =  0, /*!< Offset to the bridge words's format field    */
         Csf      =  4, /*!< Offset to the Crate.Sloc.Fiber field         */  
         Left     = 16, /*!< Offset to the number of Tpc Records left     */
         Status   = 24  /*!< Offset to the status field                   */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

         \brief Right justified masks of the bit fields of the bridge word
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Mask : uint32_t
      {
         Format   = 0x0000000f,
         Csf      = 0x00000fff,
         Left     = 0x000000ff,
         Status   = 0x000000ff
      };
      /* ---------------------------------------------------------------- */

   public:
      static uint32_t compose (uint32_t csf, uint32_t left, uint8_t status)
      {
         uint32_t w32 = PDD_INSERT32 (Mask::Format, Offset::Format,      0)
                      | PDD_INSERT32 (Mask::Csf,    Offset::Csf,       csf)
                      | PDD_INSERT32 (Mask::Left,   Offset::Left,     left)
                      | PDD_INSERT32 (Mask::Status, Offset::Status, status);
         return w32;
      }

      static uint32_t getStatus (uint32_t bridge)
      {
         uint32_t status = PDD_EXTRACT32 (bridge, Mask::Status, Offset::Status);
         return status;
      }

      uint32_t getStatus () const
      {
         uint32_t bridge = m_w32;
         uint32_t status = getStatus (bridge);
         return status;
      }

   public:
      uint32_t m_w32;  /*!< Storage for the bridge word                   */
   };
   /* ------------------------------------------------------------------- */

public:
   void construct (int type, uint32_t n64, uint32_t bridge)
   {
      Header1::construct (type, n64, bridge);

      //fprintf (stderr, "TpcData::Header  %p = %16.16" PRIx64 "\n", 
      //         this, this->m_w64);

      return;
   }

   uint32_t getStatus () const
   {
      uint64_t      header = retrieve ();
      uint32_t bridge_word = bridge   (header);
      uint32_t      status = Bridge::getStatus (bridge_word);

      return   status;
   }
      
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class RangesBody
  \brief Defines the timestamps and packet indices of both the untrimmed
         and trimmed (the event) windows
                                                                          */
/* ---------------------------------------------------------------------- */
class RangesBody
{
public:
   /* ------------------------------------------------------------------- *//*!

     \class Descriptor
     \brief The range definitions for one or more contributors
   
      Typically all contributors will have the same range definitions.
      but, since contributors are feed by different WIB fibers, there
      is no guarantee of synchonization on these streams due to missing
      WIB frames.  Therefore, there may be multiple range definitions.

      To accommodate this, each range defintion includes a bit mask of
      which contributors are described.
                                                                          */
   /* ------------------------------------------------------------------- */
   class Descriptor
   {
   public:

      /* ---------------------------------------------------------------- *//*!
 
        \struct Indices
        \brief  Defines the beginning and ending of the event in terms
                of an index into the data packets for one contributor.

        An index consists of two 16-bit values. The upper 16-bits
        gives the packet number and the lower 16-bits gives the offset 
        into the packet.
                                                                          */
      /* ---------------------------------------------------------------- */
      struct Indices
      {
         /* ------------------------------------------------------------- *//*!

            \brief Construct a standard index from its components
                                                                          */
         /* ------------------------------------------------------------- */
         static uint32_t index (uint16_t packet, uint16_t sample)
         {
            uint32_t idx = (packet << 16) | (sample << 0);
            return idx;
         }
         /* ------------------------------------------------------------- */

         uint32_t   m_begin; /*!< Index of beginning of event time sample */
         uint32_t     m_end; /*!< Index to ending   of event time sample  */
         uint32_t m_trigger; /*!< Index to event triggering time sample   */         

      } __attribute__ ((packed));
      /* ---------------------------------------------------------------- */




      /* ---------------------------------------------------------------- *//*!
  
         \struct Timestamps
         \brief  Gives the beginning and ending timestamp of the data 
                 this contributor

                                                                          */
      /* ---------------------------------------------------------------- */
      struct Timestamps
      {
         uint64_t m_begin;  /*!< Begining timestamp of the range          */
         uint64_t   m_end;  /*!< Ending   timestamp of the range          */
      } __attribute__ ((packed));
      /* ---------------------------------------------------------------- */




      /* ---------------------------------------------------------------- *//*!

         \brief  Constructs one range descriptor

         \param[in] idxBeg  Index of time sample for the beginning of
                            the  event window.
         \param[in] idxEnd  Index of time sample for the ending of the
                            event window.
         \param[in] idxTrg  Index of time sample for this events trigger.
         \param[in]  tsBeg  The 64-bit timestamp of first packet. This is
                             sometimes referred to as the untrimmed 
                             timestamp.
         \param[in]  tsEnd  The 64-bit timestamp of last packet. This is
                             sometimes referred to as the untrimmed
                             timestamp.
                                                                          */
      /* ---------------------------------------------------------------- */
      void construct (uint32_t       idxBeg,
                      uint32_t       idxEnd,
                      uint32_t       idxTrg,
                      uint64_t        tsBeg,
                      uint64_t        tsEnd)
     {
        m_indices.m_begin    = idxBeg;
        m_indices.m_end      = idxEnd;
        m_indices.m_trigger  = idxTrg;

        m_timestamps.m_begin = tsBeg;
        m_timestamps.m_end   = tsEnd;

        return;
     }
     /* ----------------------------------------------------------------- */

   public:
      Indices       m_indices; /*!< Indices to the event time samples     */
      Timestamps m_timestamps; /*!< Begin/end of data packet timestamps   */
   } __attribute__ ((packed));
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

      \struct Window
      \brief  Gives the timestamps of the beginning, ending and trigger.

      \note
       This window is a property of the trigger and, therefore, is common
       for all contributors.
                                                                         */
   /* ------------------------------------------------------------------- */
   struct Window
   {
      uint64_t    m_begin; /*!< Beginning timestamp of the event window   */
      uint64_t      m_end; /*!< Begin     timestamp of the event window   */
      uint64_t  m_trigger; /*!< Triggering timestamp                      */

      void construct (uint64_t tsBeg, uint64_t tsEnd, uint64_t tsTrg)
      {
         m_begin   = tsBeg;
         m_end     = tsEnd;
         m_trigger = tsTrg;

         return;
      }
      
   } __attribute__ ((packed));
   /* ------------------------------------------------------------------- */


public:
   Descriptor      m_dsc; /*!< Range descriptor                           */
   Window       m_window; /*!< Beginning and ending event timestamps      */
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class Ranges : public Header2
{
public:
   static const int Format = 0;

public:
   Ranges () { return; }

   /* ------------------------------------------------------------------- *//*!

      \brief  Get the size, in units of 64-bit words, of this record based
              on the number of ranges actually used.
      \return The size, in units of 64-bit words.

      \param[in] nranges  The number of range descriptors actually used.
                                                                          */
   /* ------------------------------------------------------------------- */
   static unsigned int n64 ()
   {
      unsigned int nbytes = sizeof (Ranges);
      return     nbytes / sizeof (uint64_t);
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief  Constructs the header of the Ranges record

      \param[in] nranges  The number of range descriptors actually used.
      \param[in]     n64  The size, in units of 64-bit words.  This value
                          should come for the n64 () method.
                                                                          */
   /* ------------------------------------------------------------------- */
   void construct (uint32_t n64)
   {
      unsigned int bridge = (Format << 0);

      Header2::construct (static_cast<int>(tpc::Stream::RecType::Ranges),
                          n64,
                          bridge);
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

     \brief  Constructs one range descriptor
     
     \param[in] idxBeg  Index of time sample for the beginning of the
                        event window.
     \param[in] idxEnd  Index of time sample for the ending of the
                        event window.
     \param[in] idxTrg  Index of time sample for this events trigger.
     \param[in]  tsBeg  The 64-bit timestamp of first packet. This is
                        sometimes referred to as the untrimmed timestamp.
     \param[in]  tsEnd  The 64-bit timestamp of last packet. This is
                        sometimes referred to as the untrimmed timestamp.
                                                                          */
   /* ------------------------------------------------------------------- */
   void descriptor (uint32_t       idxTrg,
                    uint32_t       idxBeg,
                    uint32_t       idxEnd,
                    uint64_t        tsBeg,
                    uint64_t        tsEnd)
  {
     auto &dsc = m_body.m_dsc;

     dsc.construct (idxTrg, idxBeg, idxEnd, tsBeg, tsEnd);
     return;
  }
  /* ------------------------------------------------------------------- */


public:
   RangesBody m_body;
} __attribute__ ((packed));   
/* -------------------------------------------------------------------- */


   

/* ---------------------------------------------------------------------- *//*!

  \class TocBody
  \brief Table of contents for the data pieces
                                                                          */
/* ---------------------------------------------------------------------- */
template<int MAX_PACKETS>
class TocBody
{
public:

public:
   TocBody () { return; }

public:
   class Packet
   {
      /* ---------------------------------------------------------------- *//*!

        \enum  class Size
        \brief Enumerates the sizes of the Packet bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Size: int
      {
         Format    =  4, /*!< Size of the format field                    */
         Type      =  4, /*!< Size fo the type field                      */  
         Offset64  = 24, /*!< Size of the offset field, in 64 bit units   */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified offsets of the Packet bit 
               fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Offset: int
      {
         Format    =  0, /*!< Offset of the format field                  */
         Type      =  4, /*!< Offset of the frame type field              */
         Offset64  =  8, /*!< Offset of the offset index field, 64-bits   */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Mask
        \brief Enumerates the right justified masks of the Packet bit 
               fields.
                                                                         */
      /* --------------------------------------------------------------- */
      enum class Mask: uint32_t
      {
         Format    =  0x0000000f,
         Type      =  0x0000000f,
         Offset64  =  0x00ffffff
      };
      /* --------------------------------------------------------------- */

   public:
      /* --------------------------------------------------------------- *//*!

        \brief Method to compose a Packet TOC entry

        \param[in]    type  The fragment type
        \param[in]  format  The packet's data format
        \param[in]  offset  The 64 bit offset to the entry
                                                                         */
      /* --------------------------------------------------------------- */
      static uint64_t compose (int            type,
                               int          format,
                               uint32_t     offset)
      {
         uint32_t w32 = PDD_INSERT32 (Mask::Format,   Offset::Format,   format)
                      | PDD_INSERT32 (Mask::Type,     Offset::Type,       type)
                      | PDD_INSERT32 (Mask::Offset64, Offset::Offset64, offset);

         return w32;
      }
      /* ---------------------------------------------------------------- */

      
      void construct (int        type,
                      int     version,
                      uint32_t offset)
      {
         m_w32 = compose (type, version, offset);
         return;
      }

   public:
      uint32_t m_w32;

   };
   /* ------------------------------------------------------------------- */


public:
   uint32_t m_w32[MAX_PACKETS];
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
template<int MAX_PACKETS>
class Toc : public Header2
{
public:
   static const int Format = 0;

public:
   Toc () { return; }



      /* ---------------------------------------------------------------- *//*!

        \enum  class Size
        \brief Enumerates the sizes of the Bridge bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Size: int
      {
         TocFormat =  4, /*!< Size of TOC record format field             */
         Count     =  8, /*!< Size of the count of packets field          */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified offsets of the Bridge
               bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Offset: int
      {
         TocFormat =  0, /*!< Offset of the TOC record format field       */
         Count     =  4, /*!< Offset of the count of packets field        */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Mask
        \brief Enumerates the right justified masks of the Bridge
               bit fields.
                                                                         */
      /* --------------------------------------------------------------- */
      enum class Mask: uint32_t
      {
         TocFormat =  0x0000000f,  /*!< Mask of the record format field */
         Count     =  0x000000ff,  /*!< Mask of the packet count  field */
      };
      /* --------------------------------------------------------------- */




   void construct (int pkt_cnt, uint32_t size)
   {
      static const int Format = 0;
      unsigned int bridge =
               PDD_INSERT32 (Mask::TocFormat, Offset::TocFormat,      Format)
             | PDD_INSERT32 (Mask::Count,     Offset::Count,         pkt_cnt);
                           

      Header2::construct (static_cast<int>(tpc::Stream::RecType::Toc), 
                          size,
                          bridge);
      /*
       *fprintf (stderr, 
       *        "Toc:Header " PRId32 " size = %8.8" PRIx32 " pkt_cnt = %d "
       *        "bridge = %8.8" PRIx32 " hdr = %8.8" PRIx32 "\n",
       *        size, pkt_cnt, bridge, *(uint32_t *)(this));
       */

   }

   uint32_t nbytes (int idx) const
   {
      uint32_t size = sizeof (*this) - (MAX_PACKETS - idx) 
                    * sizeof (typename TocBody<MAX_PACKETS>::Packet);
      return size;
   }

   uint32_t n64bytes (int idx) const
   {
      uint32_t size = (nbytes (idx) 
                    + sizeof (uint64_t) - 1) & ~(sizeof (uint64_t) - 1);
      return size;
   }

   typename TocBody<MAX_PACKETS>::Packet *packets ()
   {
      return reinterpret_cast<typename TocBody<MAX_PACKETS>::Packet *>
                              (m_body.m_w32);
   }

//   typename TocBody<MAX_PACKETS>::Contributor *contributors ()
//   {
//      return reinterpret_cast<typename TocBody<MAX_PACKETS>::Contributor *>
//                             (m_body.m_w32);
//   }


public:
   TocBody<MAX_PACKETS> m_body;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class Packet : public Header1
{
private:
   static const uint32_t Bridge = 0;
public:
   void construct (uint32_t n64)
   {
      Header1::construct (static_cast<int>(tpc::Stream::RecType::Packets), 
                          n64,
                          Bridge);
      return;
   }
};
/* ---------------------------------------------------------------------- */
}  /* Namespace:: tpc                                                     */ 
}  /* Namespace:: fragment                                                */
}  /* Namespace:: pdd                                                     */
/* ---------------------------------------------------------------------- */




#endif
