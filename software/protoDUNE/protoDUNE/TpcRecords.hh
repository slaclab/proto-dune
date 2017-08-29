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
   2017.08.29 jjr Stripped debugging print statements.
   2017.08.07 jjr Created
  
\* ---------------------------------------------------------------------- */

#include "Headers.hh"

namespace pdd      {
namespace Fragment {
namespace Tpc      {


/* ---------------------------------------------------------------------- *//*!

  \class Data
  \brief The TPC data record.  

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
class Data : public Header1
{
private:
   static const int Version = 0;

public:
    /* TPC Data Record Types */
    enum class RecType
    {
       Reserved   = 0,   /*!< Reserved                                   */
       Toc        = 1,   /*!< Table of Contents record                   */
       Error      = 2,   /*!< Error record                               */
       Packets    = 3    /*!< Data packets                               */
    };


public:
   void construct (int type, uint32_t n64, uint32_t bridge)
   {
      Header1::construct (type, n64, bridge);
      /*
      fprintf (stderr, "TpcData::Header  %p = %16.16" PRIx64 "\n", this, this->m_w64);
      */
      return;
   }
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \class TocBody
  \brief Table of contents for the data pieces
                                                                          */
/* ---------------------------------------------------------------------- */
template<int MAX_CONTRIBUTORS, int MAX_PACKETS>
class TocBody
{
public:

public:
   TocBody () { return; }

   /* ------------------------------------------------------------------- *//*!

      \class Contributor
      \brief Allows one to locate the toc entries an individual contributor
                                                                          */
   /* ------------------------------------------------------------------- */
   class Contributor
   {
   public:
      static const int VersionNumber = 0;

      /* ---------------------------------------------------------------- *//*!

        \enum  class Size
        \brief Enumerates the sizes of the Contributor bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Size: int
      {
         Csf       = 12, /*!< Size of the crate.slot.fiber field          */
         Count     =  8, /*!< Size of the count of packets field          */
         Offset32  = 12, /*!< Size of the offset field                    */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified offsets of the Contributor
               bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Offset: int
      {
         Csf       =  0, /*!< Offset of the crate.slot.fiber field        */
         Count     = 12, /*!< Offset of the count of packets field        */
         Offset32  = 20, /*!< Offset of the offset to toc entries         */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified masks of the Contributor
               bit fields.
                                                                         */
      /* --------------------------------------------------------------- */
      enum class Mask: uint32_t
      {
         Csf       =  0x00000fff,
         Count     =  0x000000ff,
         Offset32  =  0x000fffff
      };
      /* --------------------------------------------------------------- */



      /* --------------------------------------------------------------- *//*!

        \brief Method to layout the Contributor fields

        \param[in]    csf  The packed crate.slot.identifier
        \param[in]  count  The number of packets
        \param[in] offset  The 32-bit offset to the toc entries 
                                                                         */
      /* --------------------------------------------------------------- */
      static uint64_t compose (uint16_t csf, int count, int offset)
      {
         uint32_t w32 = 
             PDD_INSERT32 (Mask::Csf,      Offset::Csf,         csf)
           | PDD_INSERT32 (Mask::Count,    Offset::Count,     count)
           | PDD_INSERT32 (Mask::Offset32, Offset::Offset32, offset);

         /*
         fprintf (stderr, 
                  "Toc.master csf = %4.4" PRIx16 " count = %3d offset = %4d\n -> %8.8" PRIx32 "\n",
                  csf, count, offset, w32);
         */       


         ///fprintf (stderr, "Toc.master = %8.8" PRIx32 "\n", w32);
         return w32;
      }
      /* ---------------------------------------------------------------- */



      /* ---------------------------------------------------------------- *//*!

        \brief   Constructs a master toc 

        \param[in]    csf  The packed crate.slot.identifier
        \param[in]  count  The number of packets
        \param[in] offset  The 32-bit offset to the toc entries 
                                                                          */
      /* ---------------------------------------------------------------- */
      void construct (uint16_t csf, int count, int offset)
      {
         this->m_w32 = compose (csf, count, offset);

         ///fprintf (stderr, "Toc.master.counts = %8.8" PRIx32 ""\n", 
         ///         m_w32);

         return;
      }
      /* ---------------------------------------------------------------- */

   public:
      uint32_t m_w32;
   };
   /* ------------------------------------------------------------------- */


 
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
        \param[in]  offset  The 64 bit offset to the entry
                                                                         */
      /* --------------------------------------------------------------- */
      static uint64_t compose (int            type,
                               int         version,
                               uint32_t     offset)
      {
         uint32_t w32 = PDD_INSERT32 (Mask::Format,   Offset::Format,        1)
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
   uint32_t m_w32[MAX_CONTRIBUTORS + MAX_PACKETS];
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



template<int MAX_CONTRIBUTORS, int MAX_PACKETS>
class Toc : public Header2
{
public:
   static const int Version = 0;
public:
   Toc () { return; }

   void construct (uint32_t cnt, uint32_t size)
   {
      unsigned int bridge = (cnt << 4) | (Version << 0);

      Header2::construct (static_cast<int>(Tpc::Data::RecType::Toc), 
                          size,
                          bridge);
      /*
      fprintf (stderr, 
               "Toc:Header cnt = %4" PRId32 " size = %8.8" PRIx32 "hdr = %8.8" PRIx32 "\n",
               cnt, size, *(uint32_t *)(this));
      */

   }

   uint32_t nbytes (int idx) const
   {
      uint32_t size = sizeof (*this) - (MAX_CONTRIBUTORS + MAX_PACKETS - idx) 
                    * sizeof (typename TocBody<MAX_CONTRIBUTORS, MAX_PACKETS>::Packet);
      return size;
   }

   uint32_t n64bytes (int idx) const
   {
      uint32_t size = (nbytes (idx) 
                    + sizeof (uint64_t) - 1) & ~(sizeof (uint64_t) - 1);
      return size;
   }

   typename TocBody<MAX_CONTRIBUTORS, MAX_PACKETS>::Packet *packets (int nsrcs)
   {
      return reinterpret_cast<typename TocBody<MAX_CONTRIBUTORS, MAX_PACKETS>::Packet *>
                              (m_body.m_w32 + nsrcs);
   }

   typename TocBody<MAX_CONTRIBUTORS, MAX_PACKETS>::Contributor *contributors ()
   {
      return reinterpret_cast<typename TocBody<MAX_CONTRIBUTORS, MAX_PACKETS>::Contributor *>
                             (m_body.m_w32);
   }


public:
   TocBody<MAX_CONTRIBUTORS, MAX_PACKETS> m_body;
} __attribute__ ((packed));


class Packet : public Header1
{
private:
   static const uint32_t Bridge = 0;
public:
   void construct (uint32_t n64)
   {
      Header1::construct (static_cast<int>(Tpc::Data::RecType::Packets), 
                          n64,
                          Bridge);
      return;
   }
};
/* ---------------------------------------------------------------------- */
}  /* Namespace:: Tpc                                                     */ 
}  /* Namespace:: Fragment                                                */
}  /* Namespace:: pdd                                                     */
/* ---------------------------------------------------------------------- */




#endif
