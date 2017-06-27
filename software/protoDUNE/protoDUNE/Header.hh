// -*-Mode: C++;-*-

#ifndef PDD_HEADER_HH
#define PDD_HEADER_HH

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     Header.hh
 *  @brief    Proto-Dune Data Header 
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
 *  <2017/06/19>
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
   2017.06.19 jjr Created
  
\* ---------------------------------------------------------------------- */

namespace pdd {

#define PDD_DEBUG(_mask, _offset, _val)                                   \
   fprintf (stderr,                                                       \
           "Mask: %8.8" PRIx32 " Offset: %8.8x Value: %8.8" PRIx32 "\n",  \
            static_cast<uint32_t>(  _mask),                               \
            static_cast<     int>(_offset),                               \
            static_cast<uint32_t>(  _val)),                               \


#define PDD_INSERT(_type, _mask, _offset, _val)                           \
   (                                                                      \
     ((static_cast<_type>((_val) & static_cast<uint32_t>(  _mask)))       \
                        << static_cast<     int>(_offset))                \
   )


#define PDD_INSERTC(_type, _mask, _offset, _val)                         \
   ((~(static_cast<_type>(_val) & static_cast<uint32_t>(  _mask)))       \
                         << (64 - static_cast<     int>(_offset)))


#define PDD_INSERT64(_mask, _offset, _val)                               \
        PDD_INSERT(uint64_t, _mask, _offset, _val)


#define PDD_INSERT64C(_mask, _offset, _val)                              \
        PDD_INSERTC(uint64_t, _mask, _offset, _val)


#define PDD_INSERT32(_mask, _offset, _val)                               \
        PDD_INSERT(uint32_t, _mask, _offset, _val)


#define PDD_INSERT32C(_mask, _offset, _val)                              \
        PDD_INSERTC(uint32_t, _mask, _offset, _val)


#define PDD_EXTRACT(_type, _val, _mask, _offset)                         \
   (((_val) >> static_cast<int>(_offset)) & static_cast<uint32_t>(_mask))

#define PDD_EXTRACT64(_val, _mask, _offset)                              \
        PDD_EXTRACT(uint64_t, _val, _mask, _offset)
   
#define PDD_EXTRACT32(_val, _mask, _offset)                              \
        PDD_EXTRACT(uint64_t, _val, _mask, _offset)


/* ---------------------------------------------------------------------- *//*!

  \class  Header 0
  \brief  Generic format = 0 header
                                                                          */
/* ---------------------------------------------------------------------- */
class Header0
{
public:
   Header0 (int        type, 
            int      naux64, 
            uint8_t   spec0, 
            uint32_t  spec1, 
            uint32_t nbytes) :
      m_w64 (compose (type, naux64, spec0, spec1, nbytes))
  {
     return;
  }

   /* ------------------------------------------------------------------- *//*!

     \enum  class Size
     \brief Enumerates the sizes of the Header bit fields.
                                                                         */
   /* ------------------------------------------------------------------ */
   enum class Size: int
   {
      Format    =  4, /*!< Size of the format field                      */
      Type      =  4, /*!< Size of the frame type field                  */
      Length    = 24, /*!< Size of the length field                      */
      Specific0 =  4, /*!< Size of the first type specific field         */
      NAux64    =  4, /*!< Size of the auxillary length field            */
      Specific1 = 24  /*!< Size of the first type specific field         */
   };
   /* ------------------------------------------------------------------ */


   /* ------------------------------------------------------------------- *//*!

     \enum  class Offset
     \brief Enumerates the right justified offsets of the header bit 
            fields.
                                                                         */
   /* ------------------------------------------------------------------ */
   enum class Offset: int
   {
      Format    =  0, /*!< Size of the format field                       */
      Type      =  4, /*!< Size of the frame type field                   */
      Length    =  8, /*!< Size of the length field                       */
      Specific0 = 32, /*!< Size of the first type specific field          */
      NAux64    = 36, /*!< Size of the auxillary length field             */
      Specific1 = 40  /*!< Size of the second type specific field         */
   };
   /* ------------------------------------------------------------------ */


   /* ------------------------------------------------------------------- *//*!

     \enum  class Offset
     \brief Enumerates the right justified masks of the header bit fields.
                                                                         */
   /* ------------------------------------------------------------------ */
   enum class Mask: uint32_t
   {
      Format    =  0x0000000f,
      Type      =  0x0000000f,
      Length    =  0x00ffffff,
      Specific0 =  0x0000000f,
      NAux64    =  0x0000000f,
      Specific1 =  0x00ffffff
   };
   /* ------------------------------------------------------------------ */

public:
   static uint64_t compose (int            type,
                            int          naux64,
                            uint8_t       spec0,
                            uint32_t      spec1, 
                            uint32_t     nbytes);

   uint64_t retrieve () const
   {
      return m_w64;
   }

   static uint32_t format (uint64_t w64) 
   { 
      return PDD_EXTRACT64 (w64,  Mask::Format, Offset::Format);
   }


   static uint32_t type   (uint64_t w64) 
   {
      return PDD_EXTRACT64 (w64,  Mask::Type,  Offset::Type);
   }


   static uint32_t length  (uint64_t w64) 
   {
      return PDD_EXTRACT64 (w64, Mask::Length, Offset::Length);
   }


   static uint32_t specific0 (uint64_t w64)
   { 
      return PDD_EXTRACT64 (w64, Mask::Specific0, Offset::Specific0);
   }


   static uint32_t naux64 (uint64_t w64) 
   { 
      return PDD_EXTRACT64 (w64,   Mask::NAux64,   Offset::NAux64);
   }


   static uint32_t specific1 (uint64_t w64)
   { 
      return PDD_EXTRACT64 (w64, Mask::Specific1, Offset::Specific1);
   }


public:
   uint64_t m_w64;
}  __attribute__ ((packed));
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief Method to compose a generic Header0 

  \param[in]   type  The fragment type
  \param[in] naux64  The number of 64 bit auxillary words
  \param[in]  spec0  The  8-bit type specific word
  \param[in]  spec1  The 24-bit type specific word
  \param[in] nbytes  The length of this fragment in bytes
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t Header0::compose (int          type,
                                  int        naux64,
                                  uint8_t     spec0, 
                                  uint32_t    spec1,
                                  uint32_t   nbytes)
{
   uint64_t w64 = PDD_INSERT64 (Mask::Format,    Offset::Format,        0)
                | PDD_INSERT64 (Mask::Type,      Offset::Type,       type)
                | PDD_INSERT64 (Mask::Length,    Offset::Length,   nbytes)
                | PDD_INSERT64 (Mask::Specific0, Offset::Specific0, spec0)
                | PDD_INSERT64 (Mask::NAux64,    Offset::NAux64,   naux64)
                | PDD_INSERT64 (Mask::Specific1, Offset::Specific1, spec1);

   return w64;
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \class  Header 1
  \brief  Generic format = 1 header
                                                                          */
/* ---------------------------------------------------------------------- */
class Header1
{
   static const int VersionNumber = 1;public:
   Header1 () { return; }


   Header1 (int        type, 
            int     version,
            uint32_t nbytes) :
      m_w32 (compose (type, version, nbytes))
  {
     return;
  }

   int nbytes () { return (m_w32 >> static_cast<     int>(Offset::Length))
                                  & static_cast<uint32_t>(  Mask::Length); }

   /* ------------------------------------------------------------------- *//*!

     \enum  class Size
     \brief Enumerates the sizes of the Header bit fields.
                                                                         */
   /* ------------------------------------------------------------------ */
   enum class Size: int
   {
      Format    =  4, /*!< Size of the format field                      */
      Type      =  4, /*!< Size of the record/frame type    field        */
      Version   =  4, /*!< Size of the resOcord/frame version field        */
      Length    = 24, /*!< Size of the length field                      */
   };
   /* ------------------------------------------------------------------ */


   /* ------------------------------------------------------------------- *//*!

     \enum  class Offset
     \brief Enumerates the right justified offsets of the header bit 
            fields.
                                                                         */
   /* ------------------------------------------------------------------ */
   enum class Offset: int
   {
      Format    =  0, /*!< Offset of the format field                    */
      Type      =  4, /*!< Offset of the frame type field                */
      Version   =  8, /*!< Offset of the record/frame version field      */
      Length    = 12, /*!< Offset of the length field                    */
   };
   /* ------------------------------------------------------------------ */


   /* ------------------------------------------------------------------- *//*!

     \enum  class Offset
     \brief Enumerates the right justified masks of the header bit fields.
                                                                         */
   /* ------------------------------------------------------------------ */
   enum class Mask: uint32_t
   {
      Format    =  0x0000000f,
      Type      =  0x0000000f,
      Version   =  0x0000000f,
      Length    =  0x000fffff,
   };
   /* ------------------------------------------------------------------ */

public:
   static uint64_t compose (int            type,
                            int         version,
                            uint32_t     nbytes);

   void construct (int        type,
                   int     version,
                   uint32_t nbytes)
    {
       Header1::m_w32 = Header1::compose (type, version, nbytes);
       return;
    }

   uint64_t retrieve () const
   {
      return m_w32;
   }

   static uint32_t type (uint32_t w32)
   {
      return PDD_EXTRACT32 (w32, Mask::Type, Offset::Type);
   }

   static uint32_t version (uint32_t w32)
   {
      return PDD_EXTRACT32 (w32, Mask::Version, Offset::Version);
   }

   static uint32_t nbytes (uint32_t w32)
   {
         return PDD_EXTRACT32 (w32, Mask::Length, Offset::Length);
   }

public:
   uint32_t m_w32;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Method to compose a generic Header1

  \param[in]    type  The fragment type
  \param[in] version  A 4-bit version number of this frame/record
  \param[in]  nbytes  The length of this fragment in bytes
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t Header1::compose (int        type,
                                  int     version,
                                  uint32_t nbytes)
{
   uint32_t w32 = PDD_INSERT32 (Mask::Format,    Offset::Format,        1)
                | PDD_INSERT32 (Mask::Type,      Offset::Type,       type)
                | PDD_INSERT32 (Mask::Version,   Offset::Version, version)
                | PDD_INSERT32 (Mask::Length,    Offset::Length,   nbytes);


   return w32;
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \class  Trailer
  \brief  Generic trailers.  Trailers are always the complemented value
          of its corresponding Header.  As such, they are not actually
          filled on a field by field basis.
                                                                          */
/* ---------------------------------------------------------------------- */
class Trailer
{
public:
   Trailer (uint64_t  header) : m_w64 (~header) { return; }
   static   uint64_t compose  (uint64_t header) { return  ~header; }
   void            construct  (uint64_t header) { m_w64 = ~header; }

public:
   uint64_t m_w64;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */





namespace Fragment
{

/* ---------------------------------------------------------------------- *//*!

  \enum  class Type
  \brief Enumerates the type of fragment. A fragment is the largest 
         self-contained piece of data that originates from 1 contributor.
                                                                          */
/* ---------------------------------------------------------------------- */
enum class Type
{
   Reserved_0    = 0, /*!< Reserved for future use                        */
   Control       = 1, /*!< Control, \e e.g. Begin/Pause/Resume/Stop/etc   */
   Data          = 2, /*!< Detector data of some variety                  */
   MonitorSync   = 3, /*!< Statistics synchronized across contributors    */
   MonitorUnSync = 4  /*!< Statistics for a single contributor            */
};
/* ---------------------------------------------------------------------- */


static const uint32_t Pattern = 0x8b309e;


/* ---------------------------------------------------------------------- *//*!
                                                                          */
/* ---------------------------------------------------------------------- */
class Identifier
{
   /* ------------------------------------------------------------------- *//*!

     \enum  class FormatType
     \brief This enumerates the identifier formats. It is really more
            for documentation than usage
                                                                          */
   /* ------------------------------------------------------------------- */
   enum class FormatType
   {
      _0 = 0,  /*!< Only 1 source identifier                              */
      _1 = 1   /*!< Two source identifiers                                */
   };
   /* ------------------------------------------------------------------- */


public:
   Identifier ()  { return; }
   Identifier (uint16_t        src,
               uint32_t   sequence,
               uint8_t        type,
               uint64_t  timestamp) : 
      Identifier (FormatType::_0, src,     0, type, sequence, timestamp) { return; }

   Identifier (uint16_t       src0, 
               uint16_t       src1,
               uint16_t       type,
               uint32_t   sequence,
               uint64_t  timestamp) :
      Identifier (FormatType::_1, src0, src1, type, sequence, timestamp) { return; }

   Identifier (enum FormatType format,
               uint16_t          src0,
               uint16_t          src1,
               uint8_t           type,
               uint32_t      sequence,
               uint64_t     timestamp) :
      m_w64       (compose (static_cast<int>(format), src0, src1, type, sequence)),
      m_timestamp (timestamp)
   {
      return;
   }
   
   Identifier (Identifier const &identifier) 
   {
      *this = identifier;
   }


   enum class Size
   {
      Format   =  4, /*!< Size of the format field                       */
      Src0     = 12, /*!< Channel bundle 0 source identifier (from WIB)  */
      Reserved =  4, /*!< Reserved, must be 0                            */
      Src1     = 12, /*!< Channel bundle 1 source identified (from WIB)  */
      Sequence = 32  /*!< Overall sequence number                        */
   };

   enum class Mask : uint32_t
   {
      Format   = 0x0000000f,
      Src0     = 0x00000fff,
      Type     = 0x0000000f,
      Src1     = 0x00000fff,
      Sequence = 0xffffffff
   };

   enum Offset
   {
      Format    = 0,
      Src0      = 4,
      Type      = 16,
      Src1     = 20,
      Sequence = 32
   };

   void construct (uint16_t       src0, 
                   uint16_t       src1,
                   uint8_t        type,
                   uint32_t   sequence,
                   uint64_t  timestamp)
      {
         m_w64 = compose (static_cast<int>(FormatType::_1),
                          src0,
                          src1,
                          type,
                          sequence);
         m_timestamp = timestamp;

         /*
         fprintf (stderr, 
                  "Identifier.m_64 = %16.16" PRIx64 " ts = %16.16" PRIx64 "\n",
                  m_w64,
                  m_timestamp);
         */

      }
   
   static uint64_t compose (int         format, 
                            uint16_t      src0, 
                            uint16_t      src1,
                            uint8_t       type,
                            uint32_t  sequence)
   {
      uint64_t w64 = 
               PDD_INSERT64 (Mask::Format,   Offset::Format,     format)
             | PDD_INSERT64 (Mask::Src0,     Offset::Src0,         src0)
             | PDD_INSERT64 (Mask::Type,     Offset::Type,         type)
             | PDD_INSERT64 (Mask::Src1,     Offset::Src1,         src1)
             | PDD_INSERT64 (Mask::Sequence, Offset::Sequence, sequence);
 
      return w64;
   }


public:
   uint64_t       m_w64; /*!< 64-bit packed word of format,srcs, sequence */
   uint64_t m_timestamp; /*!< The identifying timestamp                   */
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \brief  Template class for Fragment Headers

   This class is specialized for the various types of fragment headers
                                                                          */
/* ---------------------------------------------------------------------- */
template<enum Fragment::Type TYPE>
class Header : public Header0
{
public:
   Header  (int          naux64,
            uint32_t      spec0,
            uint8_t       spec1,
            uint32_t     nbytes) :
      Header0 (static_cast<int>(TYPE), naux64, spec0, spec1, nbytes)
   {
      return;
   }

   static uint64_t compose (int      naux64,
                            uint32_t  spec0, 
                            uint8_t   spec1,
                            uint32_t nbytes);
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Template class for Fragment Trailers
                                                                          */
/* ---------------------------------------------------------------------- */
template<enum Fragment::Type TYPE>
class Trailer
{
public:
   static uint64_t compose (int       naxu64,
                            uint32_t   spec0,
                            uint8_t    spec1,
                            uint32_t nbytes);

public:
   uint64_t m_w64;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Specialized fragment header for Data
                                                                          */
/* ---------------------------------------------------------------------- */
template<>
class Header<Fragment::Type::Data> : public Header0
{

public:
   enum class RecType 
   {
      Reserved_0 = 0,
      TpcData    = 1,
      Originator = 2,
      Toc        = 3
   };


public:
   Header<Fragment::Type::Data> (RecType              rectype,
                                 Identifier const &identifier,
                                 uint32_t              nbytes) :
      Header0 (static_cast<int>(Fragment::Type::Data), 
               (sizeof (identifier))/ sizeof (uint64_t),
               static_cast<uint8_t>(rectype),
               Fragment::Pattern,
               nbytes),
      m_identifier (identifier)
      {
         return;
      }

   Header<Fragment::Type::Data> (RecType       rectype) :
      Header0 (static_cast<int>(Fragment::Type::Data), 
               sizeof (struct Identifier) / sizeof (uint64_t),
               static_cast<uint8_t>(rectype),
               Fragment::Pattern,
               0),
      m_identifier ()
      {
         return;
      }

   Header<Fragment::Type::Data> () :
      Header0 (static_cast<int>(Fragment::Type::Data), 
               sizeof (struct Identifier) / sizeof (uint64_t),
               0,
               Fragment::Pattern,
               0),
      m_identifier ()
      {
         return;
      }



   void construct (RecType              rectype,
                   Identifier const &identifier,
                   uint32_t              nbytes)
   {
      m_w64 = Header0::compose (static_cast<int>(Fragment::Type::Data),
                                sizeof (m_identifier) / sizeof (uint64_t),
                                static_cast<uint8_t>(rectype),
                                Fragment::Pattern,
                                nbytes);

      m_identifier = identifier;
      return;
   }


   void construct (RecType       rectype,
                   uint16_t         src0,
                   uint16_t         src1,
                   uint8_t          type,
                   uint32_t     sequence,
                   uint64_t    timestamp,
                   uint32_t       nbytes)
   {
      m_w64 = Header0::compose (static_cast<int>(Fragment::Type::Data),
                                sizeof (m_identifier) / sizeof (uint64_t),
                                static_cast<uint8_t>(rectype),
                                Fragment::Pattern,
                                nbytes);
      //fprintf (stderr, "Header.m_64 %16.16" PRIx64 "\n", m_w64);

      m_identifier.construct (src0, src1, type, sequence, timestamp);
      return;
   }


public:
   Identifier  m_identifier;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
class Version
{
public:
   Version () { return; }
   Version (uint32_t    firmware, 
            uint32_t    software) :
      m_w64 (compose (firmware, software))
   {
      return;
   }

   static uint64_t compose (uint32_t firmware, uint32_t software)
   {
      uint64_t version = firmware;
      version <<= 32;
      version  |= software;

      fprintf (stderr, "Version %16.16" PRIx64 "\n", version);
      return version;
   }

   void construct (uint32_t firmware, uint32_t software)
   {
      m_w64 = compose (firmware, software);
      return;
   }

   static uint32_t software (uint64_t m_64) { return m_64 >>  0; }
   static uint32_t firmware (uint64_t m_64) { return m_64 >> 32; }

   uint32_t software () const { return software (m_w64); }
   uint32_t firmware () const { return firmware (m_w64); }

public:
   uint64_t       m_w64;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class OriginatorBody
  \brief The hardware, software and geographic information about this
         datas orginator
                                                                          */
/* ---------------------------------------------------------------------- */
class OriginatorBody
{
public:
   OriginatorBody () { return; }

   int construct (uint32_t     fw_version,
                  uint32_t     sw_version,
                  char const    *rptSwTag,
                  uint8_t       nrptSwTag,
                  uint64_t   serialNumber,
                  uint32_t       location,
                  char const   *groupName,
                  uint8_t      ngroupName)
   {
      m_location     = location;
      m_version.construct (fw_version, sw_version);
      m_serialNumber = serialNumber;

      strncpy (m_strings, rptSwTag, nrptSwTag + 1);
      strncpy (m_strings + nrptSwTag + 1, groupName, ngroupName + 1);

      int size = sizeof (OriginatorBody) 
               - sizeof (m_strings) + nrptSwTag + ngroupName + 2;

      return size;
   }

   uint32_t location      () const { return     m_location; }
   uint64_t serialNumber  () const { return m_serialNumber; }
   Version const &version () const { return      m_version; }
   char const   *rptSwTag () const { return      m_strings; }
   char const  *groupName () const { return &m_strings [strlen(rptSwTag()) + 1]; }

public:
   uint32_t     m_location;
   uint64_t m_serialNumber;
   Version       m_version;
   char   m_strings[32+32];
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



class Originator : public Header1
{
public:
   static const int Version = 1;
public:
   Originator () { return; }

   int construct (uint32_t     fw_version,
                  uint32_t     sw_version,
                  char const    *rptSwTag,
                  uint8_t       nrptSwTag,
                  uint64_t   serialNumber,
                  uint32_t       location,
                  char const   *groupName,
                  uint8_t      ngroupName)
   {
      fprintf (stderr, "Fw/Sw = %8.8" PRIx32 " %8.8" PRIx32 "\n", fw_version, sw_version);
      int size = m_body.construct (fw_version,
                                   sw_version,
                                   rptSwTag,
                                   nrptSwTag,
                                   serialNumber,
                                   location,
                                   groupName,
                                   ngroupName);

      size += sizeof (Header1);
      Header1::construct (static_cast<int>
                          (Header<Fragment::Type::Data>::RecType::Originator), 
                          Version,
                          size);

      return size;
   }
public:
   OriginatorBody m_body;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class TocBody
  \brief Table of contents for the data pieces
                                                                          */
/* ---------------------------------------------------------------------- */
template<int MAX_ENTRIES>
class TocBody
{
public:

public:
   TocBody () { return; }

   void add (int idx, int format, int version, int src, uint32_t offset)
   {
      m_entries[idx].construct (format, version, offset);
   }

   /* ------------------------------------------------------------------- *//*!

      \class Master
      \brief Allows one to locate the toc entries for up to 3 sources
                                                                          */
   /* ------------------------------------------------------------------- */
   class Master
   {
   public:
      static const int VersionNumber = 1;

      /* ---------------------------------------------------------------- *//*!

        \enum  class Size
        \brief Enumerates the sizes of the Master bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Size: int
      {
         Format    =  4, /*!< Size of the format field                    */
         Version   =  4, /*!< Size of the record/frame version field      */
         Counts    = 24  /*!< Size of the counts field                    */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified offsets of the Master bit 
               fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Offset: int
      {
         Format    =  0, /*!< Offset of the format field                  */
         Version   =  4, /*!< Offset of the record/frame version field    */
         Counts    =  8, /*!< Offset of the counts field                  */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified masks of the Master bit 
               fields.
                                                                         */
      /* --------------------------------------------------------------- */
      enum class Mask: uint32_t
      {
         Format    =  0x0000000f,
         Version   =  0x0000000f,
         Counts    =  0x00ffffff,
      };
      /* --------------------------------------------------------------- */


      /* --------------------------------------------------------------- *//*!

        \brief Method to layout the Master index

        \param[in]  offset  A 24 bit number containing up to 3 source
                            ids.
                                                                         */
      /* --------------------------------------------------------------- */
      static uint64_t compose (uint32_t counts)
      {
         uint32_t w32 = 
             PDD_INSERT32 (Mask::Format,   Offset::Format,              1)
           | PDD_INSERT32 (Mask::Version,  Offset::Version, VersionNumber)
           | PDD_INSERT32 (Mask::Counts,   Offset::Counts,         counts);


         ///fprintf (stderr, "Toc.master = %8.8" PRIx32 "\n", w32);
         return w32;
      }
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \brief   Adds another count field
        \return  The updated count field
   
        \param[in]  counts The current counts field
        \param[in]   count The count field to add
                                                                          */
      /* ---------------------------------------------------------------- */
      static uint32_t add (uint32_t counts, uint8_t count)
      {
         
         counts <<= 8;
         counts  |= count;

         ///fprintf (stderr, "Toc.master.counts = %8.8" PRIx32 "%8.8" PRIx32 "\n", 
         ///         count, counts);

         return count;
      }
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \brief Constructs the TOC layout entry

        \param[in] counts  Up to 3 source id counts, devoting 8-bits per
                                                                          */
      /* ---------------------------------------------------------------- */
      void construct (uint32_t counts)
      {
         m_w32 = compose (counts);
         ///fprintf (stderr, "Toc.master = %8.8" PRIx32 "\n", m_w32);
         return;
      }
      /* ---------------------------------------------------------------- */

   public:
      uint32_t m_w32;
   };
   /* ------------------------------------------------------------------- */


 
   class Entry
   {
      /* ---------------------------------------------------------------- *//*!

        \enum  class Size
        \brief Enumerates the sizes of the Entry bit fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Size: int
      {
         Format    =  4, /*!< Size of the format field                    */
         Version   =  4, /*!< Size of the record/frame version field      */
         Offset64  = 24, /*!< Size of the offset field                    */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Offset
        \brief Enumerates the right justified offsets of the Entry bit 
               fields.
                                                                          */
      /* ---------------------------------------------------------------- */
      enum class Offset: int
      {
         Format    =  0, /*!< Offset of the format field                  */
         Type      =  4, /*!< Offset of the frame type field              */
         Version   =  8, /*!< Offset of the record/frame version field    */
         Offset64  = 12, /*!< Offset of the offset index field            */
      };
      /* ---------------------------------------------------------------- */


      /* ---------------------------------------------------------------- *//*!

        \enum  class Mask
        \brief Enumerates the right justified masks of the Entry bit 
               fields.
                                                                         */
      /* --------------------------------------------------------------- */
      enum class Mask: uint32_t
      {
         Format    =  0x0000000f,
         Type      =  0x0000000f,
         Version   =  0x0000000f,
         Offset64  =  0x000fffff,
      };
      /* --------------------------------------------------------------- */

   public:

      /* --------------------------------------------------------------- *//*!

        \brief Method to compose a generic Header1

        \param[in]    type  The fragment type
        \param[in] version  A 4-bit version number of this frame/record
        \param[in]  offset  The 64 bit offset to the entry
                                                                         */
      /* --------------------------------------------------------------- */
      static uint64_t compose (int            type,
                               int         version,
                               uint32_t     offset)
      {
         uint32_t w32 = PDD_INSERT32 (Mask::Format,   Offset::Format,        1)
                      | PDD_INSERT32 (Mask::Type,     Offset::Type,       type)
                      | PDD_INSERT32 (Mask::Version,  Offset::Version, version)
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
   /* ---------------------------------------------------------------- */


public:
   Master   m_master;
   Entry    m_entries[MAX_ENTRIES];
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



template<int MAX_ENTRIES>
class Toc : public Header1
{
public:
   static const int Version = 1;
public:
   Toc () { return; }

   void construct (uint32_t size)
   {
      Header1::construct (static_cast<int>
                          (Header<Fragment::Type::Data>::RecType::Toc), 
                          Version,
                          size);
   }

   uint32_t nbytes (int idx) const
   {
      uint32_t size = sizeof (*this) - (MAX_ENTRIES - idx) 
                    * sizeof (typename TocBody<MAX_ENTRIES>::Entry);
      return size;
   }

   uint32_t n64bytes (int idx) const
   {
      uint32_t size = (nbytes + sizeof (uint64_t) - 1) & ~(sizeof (uint64_t) - 1);
      return size;
   }


public:
   TocBody<MAX_ENTRIES> m_body;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */
}


template<enum Fragment::Type TYPE>
inline uint64_t Fragment::Header<TYPE>:: compose (int      naux64,
                                                  uint32_t  spec0, 
                                                  uint8_t   spec1,
                                                  uint32_t nbytes)
{
   uint64_t w64 = Header0::compose (static_cast<int>(TYPE),
                                    naux64,
                                    Fragment::Pattern,
                                    spec1,
                                    nbytes);
   return w64;
}


template<enum Fragment::Type TYPE>
inline uint64_t Fragment::Trailer<TYPE>::compose (int       naux64,
                                                  uint32_t   spec0,
                                                  uint8_t    spec1,
                                                  uint32_t  nbytes)
{
   uint64_t w64 = 
      PDD_INSERT64C (Header0::Mask::Format,    Header0::Offset::Format,        0)
    | PDD_INSERT64C (Header0::Mask::Type,      Header0::Offset::Type,       TYPE)
    | PDD_INSERT64C (Header0::Mask::NAux64,    Header0::Offset::NAux64,   naux64)
    | PDD_INSERT64C (Header0::Mask::Specific0, Header0::Offset::Specific0, spec0)
    | PDD_INSERT64C (Header0::Mask::Specific1, Header0::Offset::Specific1, spec1)
    | PDD_INSERT64C (Header0::Mask::Length,    Header0::Offset::Length,   nbytes);

   return w64;
}



#if 0
template<>
inline void FragmentHeader< FragmentType::Data>::construct (RecType       type,
                                                           uint32_t swversion,
                                                           uint32_t fwversion,
                                                           uint32_t    nbytes)
{
   m_w64 = Header0::compose (FragmentType::Data,
                             sizeof (struct Version) / sizeof (uint64_t),
                             FragmentPattern,
                             static_cast<uint8_t>(type),
                             nbytes);

   m_version.software = swversion;
   m_version.firmware = fwversion;
   return;
}
#endif


#undef PDD_INSERT
#undef PDD_INSERTC

#undef PDD_INSERT32
#undef PDD_INSERT32C

#undef PDD_INSERT64
#undef PDD_INSERT64C

}

#endif
