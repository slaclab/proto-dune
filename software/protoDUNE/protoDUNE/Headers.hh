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
   2018.02.06 jjr Added methods to add the source specifiers (i.e. the WIB
   2017.08.28 jjr Fix position of trigger type in auxilliary block to 
                  match the documentation
   2017.08.29 jjr Stripped debugging print statements.
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
   Header0 (int         type,
            uint8_t  subtype, 
            uint32_t  bridge, 
            int       naux64, 
            uint32_t     n64) :
      m_w64 (compose (type, subtype, bridge, naux64, n64))
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
      NAux64    =  4, /*!< Size of the auxillary length field            */
      SubType   =  4, /*!< Size of the record's subtype field            */
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
      Format    =  0, /*!< Offset to the format field                    */
      Type      =  4, /*!< Offset to the frame type field                */
      Length    =  8, /*!< Offset to the length field                    */
      NAux64    = 32, /*!< Offset to the auxillary length field          */
      SubType   = 36, /*!< Offset to the record's subtype field          */
      Bridge    = 40  /*!< Offset to the bridge word                     */
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
      NAux64    =  0x0000000f,
      SubType   =  0x0000000f,
      Bridge    =  0x00ffffff
   };
   /* ------------------------------------------------------------------ */

public:
   static uint64_t compose (int            type,
                            uint8_t     subtype,
                            uint32_t     bridge,
                            int          naux64,
                            uint32_t        n64);

   void          construct (int            type,
                            uint8_t     subtype,
                            uint32_t     bridge,
                            int          naux64,
                            uint32_t        n64)
   {
      m_w64 = compose (type, subtype, bridge, n64, naux64);
   }


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


   static uint32_t subtype (uint64_t w64)
   { 
      return PDD_EXTRACT64 (w64, Mask::SubType, Offset::SubType);
   }


   static uint32_t naux64 (uint64_t w64) 
   { 
      return PDD_EXTRACT64 (w64,   Mask::NAux64,   Offset::NAux64);
   }


   static uint32_t bridge (uint64_t w64)
   { 
      return PDD_EXTRACT64 (w64, Mask::Bridge, Offset::Bridge);
   }


public:
   uint64_t m_w64;
}  __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Method to compose a generic Header0 
  \param[in]    type The fragment type
  \param[in] subtype The  4-bit record subtype
  \param[in] naux64  The number of 64 bit auxillary words
  \param[in]  spec1  The 24-bit type specific word
  \param[in]    n64  The length of this fragment in units of 64-bit words
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t Header0::compose (int          type,
                                  uint8_t   subtype,
                                  uint32_t   bridge, 
                                  int        naux64,
                                  uint32_t      n64)
{
   /*
   fprintf (stderr, 
            "Header0::compose type=%d subtype=%d bridge=%8.8" PRIx32 " "
            "naux64=%d n64=%8.8" PRIx32 "\n",
            type, subtype, bridge, naux64, n64);
   */
             
   uint64_t w64 = PDD_INSERT64 (Mask::Format,  Offset::Format,        0)
                | PDD_INSERT64 (Mask::Type,    Offset::Type,       type)
                | PDD_INSERT64 (Mask::Length,  Offset::Length,      n64)
                | PDD_INSERT64 (Mask::SubType, Offset::SubType, subtype)
                | PDD_INSERT64 (Mask::NAux64,  Offset::NAux64,   naux64)
                | PDD_INSERT64 (Mask::Bridge,  Offset::Bridge,   bridge);

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
public:
   Header1 () { return; }


   Header1 (int        type, 
            uint32_t    n64,
            uint32_t bridge) :
      m_w64 (compose (type, n64, bridge))
  {
     return;
  }

   int n64 () { return (m_w64 >> static_cast<     int>(Offset::Length))
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
      Length    = 24, /*!< Size of the length field                      */
      Bridge    = 32  /*!< Size of the bridge field                      */
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
      Length    =  8, /*!< Offset of the length field                    */
      Bridge    = 32  /*!< Offset of the bridge field                    */
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
      Length    =  0x00ffffff,
      Bridge    =  0xffffffff
   };
   /* ------------------------------------------------------------------ */

public:
   static uint64_t compose (int            type,
                            uint32_t        n64,
                            uint32_t     bridge);

   void construct (int        type,
                   uint32_t    n64,
                   uint32_t bridge)
    {
       Header1::m_w64 = Header1::compose (type, n64, bridge);
       return;
    }

   uint64_t retrieve () const
   {
      return m_w64;
   }

   static uint32_t type (uint64_t w64)
   {
      return PDD_EXTRACT64 (w64, Mask::Type, Offset::Type);
   }

   static uint32_t n64 (uint64_t w64)
   {
         return PDD_EXTRACT64 (w64, Mask::Length, Offset::Length);
   }

   static uint32_t bridge (uint64_t w64)
   {
      return PDD_EXTRACT64 (w64, Mask::Bridge, Offset::Bridge);
   }


public:
   uint64_t m_w64;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Method to compose a generic Header1

  \param[in]    type  The record type
  \param[in]     n64  The length of this fragment in units of 64-bit words
  \param[in]  bridge  An arbitrary 32-bit bridge word. Its meaning is
                      record \a type specific
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t Header1::compose (int        type,
                                  uint32_t    n64,
                                  uint32_t bridge)
{
   /*
   fprintf (stderr, 
            "Header1::compose Format:%d Type:%d N64:%8.8" PRIx32 ""
            " Bridge:%8.8" PRIx32 "\n",
            1, type, n64, bridge);
   */

   uint64_t w64 = PDD_INSERT64 (Mask::Format, Offset::Format,      1)
                | PDD_INSERT64 (Mask::Type,   Offset::Type,     type)
                | PDD_INSERT64 (Mask::Length, Offset::Length,    n64)
                | PDD_INSERT64 (Mask::Bridge, Offset::Bridge, bridge);

   return w64;
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class  Header 2
  \brief  Generic format = 1 header
                                                                          */
/* ---------------------------------------------------------------------- */
class Header2
{
public:
   Header2 () { return; }


   Header2 (int            type, 
            unsigned int    n64,
            unsigned int bridge) :

      m_w32 (compose (type, n64, bridge))
  {
     return;
  }

   int n64 () { return (m_w32 >> static_cast<     int>(Offset::Length))
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
      Length    = 12, /*!< Size of the length field                      */
      Bridge    = 12 /*!< Size of the bridge word field                 */
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
      Length    =  8, /*!< Offset of the length field                    */
      Bridge    = 20, /*!< Offset of the bridge word field               */
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
      Length    =  0x00000fff,
      Bridge    =  0x00000fff, 
   };
   /* ------------------------------------------------------------------ */

public:
   static uint32_t compose (unsigned int   type,
                            unsigned int    n64,
                            unsigned int bridge);


   void          construct (unsigned int   type,
                            unsigned int    n64,
                            unsigned int bridge)
   {
      /*
      printf ("Header2::construct type = %u n64 = %u  bridge = %8.8x\n",
              type, n64, bridge);
      */
      Header2::m_w32 = Header2::compose (type, n64, bridge);
      return;
   }

   uint32_t retrieve () const
   {
      return m_w32;
   }

   static uint32_t type (uint64_t w32)
   {
      return PDD_EXTRACT32 (w32, Mask::Type, Offset::Type);
   }

   static uint32_t n64 (uint32_t w32)
   {
         return PDD_EXTRACT32 (w32, Mask::Length, Offset::Length);
   }

   static uint32_t bridge (uint32_t w32)
   {
      return PDD_EXTRACT32 (w32, Mask::Bridge, Offset::Bridge);
   }


public:
   uint32_t m_w32;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Method to compose a generic Header2

  \param[in]    type  The record type
  \param[in]     n64  The length of this fragment in units of 64-bit words
  \param[in   bridge  An 12-bit type specific bridge field

                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t Header2::compose (unsigned int   type,
                                  unsigned int    n64,
                                  unsigned int bridge)
{
   /*
   fprintf (stderr, 
            "Header2::compose Format:%d Type:%d N64:%3.3x Bridge:%3.3x\n",
            1, type, n64, bridge);
   */

   uint32_t w32 = PDD_INSERT64 (Mask::Format,    Offset::Format,        2)
                | PDD_INSERT64 (Mask::Type,      Offset::Type,       type)
                | PDD_INSERT64 (Mask::Length,    Offset::Length,      n64)
                | PDD_INSERT64 (Mask::Bridge,    Offset::Bridge,   bridge);

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
   explicit Trailer () { return; }
public:
   Trailer (uint64_t  header) : m_w64 (~header) { return; }
   static   uint64_t compose  (uint64_t header) { return  ~header; }
   void            construct  (uint64_t header) 
   { 
      m_w64 = ~header; 
      /*
      fprintf (stderr, "Trailer: %16.16" PRIx64 " -> %16.16" PRIx64 "\n", 
               header, m_w64);
      */
   }

public:
   uint64_t m_w64;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */





namespace fragment
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
   Identifier (uint8_t        type,
               uint16_t        src,
               uint32_t   sequence,

               uint64_t  timestamp) : 
      Identifier (FormatType::_0, type, src,     0, sequence, timestamp) { return; }

   Identifier (uint16_t       type,
               uint16_t       src0, 
               uint16_t       src1,

               uint32_t   sequence,
               uint64_t  timestamp) :
      Identifier (FormatType::_1, type, src0, src1,  sequence, timestamp) { return; }

   Identifier (enum FormatType format,
               uint8_t           type,
               uint16_t          src0,
               uint16_t          src1,
               uint32_t      sequence,
               uint64_t     timestamp) :
      m_w64       (compose (static_cast<int>(format), type, src0, src1, sequence)),
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
      Type     =  4, /*!< Trigger Type                                   */
      Src0     = 12, /*!< Channel bundle 0 source identifier (from WIB)  */
      Src1     = 12, /*!< Channel bundle 1 source identified (from WIB)  */
      Sequence = 32  /*!< Overall sequence number                        */
   };

   enum class Mask : uint32_t
   {
      Format   = 0x0000000f,
      Type     = 0x0000000f,
      Src0     = 0x00000fff,
      Src1     = 0x00000fff,
      Sequence = 0xffffffff
   };

   enum Offset
   {
      Format    = 0,
      Type      = 4,
      Src0      = 8,
      Src1     = 20,
      Sequence = 32
   };

   void construct (uint8_t        type,
                   uint16_t       src0, 
                   uint16_t       src1,                  
                   uint32_t   sequence,
                   uint64_t  timestamp)
      {
         m_w64 = compose (static_cast<int>(FormatType::_1),
                          type,
                          src0,
                          src1,
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
                            uint8_t       type,
                            uint16_t      src0, 
                            uint16_t      src1,
                            uint32_t  sequence)
   {
      uint64_t w64 = 
               PDD_INSERT64 (Mask::Format,   Offset::Format,     format)
             | PDD_INSERT64 (Mask::Type,     Offset::Type,         type)
             | PDD_INSERT64 (Mask::Src0,     Offset::Src0,         src0)
             | PDD_INSERT64 (Mask::Src1,     Offset::Src1,         src1)
             | PDD_INSERT64 (Mask::Sequence, Offset::Sequence, sequence);
 
      return w64;
   }

   void construct (uint16_t src0, uint16_t src1)
   {
      m_w64 &= ~static_cast<uint64_t>(Mask  ::Src0) 
            <<  static_cast<uint64_t>(Offset::Src0);

      m_w64 &= ~static_cast<uint64_t>(Mask  ::Src1) 
            <<  static_cast<uint64_t>(Offset::Src1);

      m_w64 = PDD_INSERT64 (Mask::Src0,     Offset::Src0,         src0)
            | PDD_INSERT64 (Mask::Src1,     Offset::Src1,         src1);

      return;
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
template<enum fragment::Type TYPE>
class Header : public Header0
{
public:
   Header  (uint8_t     subtype,
            uint32_t     bridge,
            int          naux64,
            uint32_t        n64) :
      Header0 (static_cast<int>(TYPE), subtype, bridge, naux64, n64)
   {
      return;
   }

   static uint64_t compose (uint8_t subtype,
                            uint8_t  naux64,
                            uint32_t   n64);
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Template class for Fragment Trailers
                                                                          */
/* ---------------------------------------------------------------------- */
template<enum fragment::Type TYPE>
class Trailer
{
public:
   static uint64_t compose (uint8_t subtype,
                            int      naxu64,
                            uint32_t    n64);

public:
   uint64_t m_w64;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Specialized fragment header for Data
                                                                          */
/* ---------------------------------------------------------------------- */
template<>
class Header<fragment::Type::Data> : public Header0
{

public:
   enum class RecType 
   {
      Reserved_0   = 0,  /*!< Reserved for future use                     */
      Originator   = 1,  /*!< Originator record type                      */
      TpcNormal    = 2,  /*!< Normal  TPC data, \e i.e. no errors         */
      TpcDamaged   = 3   /*!< Damaged TPC data, \e i.e. has errors        */
   };


public:
   Header<fragment::Type::Data> (RecType              rectype,
                                 Identifier const &identifier,
                                 uint32_t                 n64) :
      Header0 (static_cast<int>(fragment::Type::Data), 
               static_cast<uint8_t>(rectype),
               fragment::Pattern,
               (sizeof (identifier))/ sizeof (uint64_t),
               n64),
      m_identifier (identifier)
      {
         return;
      }

   Header<fragment::Type::Data> (RecType       rectype) :
      Header0 (static_cast<int>(fragment::Type::Data), 
               static_cast<uint8_t>(rectype),
               fragment::Pattern,
               sizeof (struct Identifier) / sizeof (uint64_t),
               0),
      m_identifier ()
      {
         return;
      }

   Header<fragment::Type::Data> () :
      Header0 (static_cast<int>(fragment::Type::Data), 
               sizeof (struct Identifier) / sizeof (uint64_t),
               0,
               fragment::Pattern,
               0),
      m_identifier ()
      {
         return;
      }



   void construct (RecType              rectype,
                   Identifier const &identifier,
                   uint32_t                 n64)
   {
      m_w64 = Header0::compose (static_cast<int>(fragment::Type::Data),
                                static_cast<uint8_t>(rectype),
                                fragment::Pattern,
                                sizeof (m_identifier) / sizeof (uint64_t),
                                n64);

      m_identifier = identifier;
      return;
   }

   void construct (uint16_t         src0,
                   uint16_t         src1)
   {
      m_identifier.construct (src0, src1);
      return;
   }


   void construct (RecType       rectype,
                   uint8_t          type,
                   uint16_t         src0,
                   uint16_t         src1,
                   uint32_t     sequence,
                   uint64_t    timestamp,
                   uint32_t          n64)
   {
      m_w64 = Header0::compose (static_cast<int>(fragment::Type::Data),
                                static_cast<uint8_t>(rectype),
                                fragment::Pattern,
                                sizeof (m_identifier) / sizeof (uint64_t),
                                n64);
      //fprintf (stderr, "Header.m_64 %16.16" PRIx64 "\n", m_w64);

      m_identifier.construct (type, src0, src1, sequence, timestamp);
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
      uint64_t version = software;
      version <<= 32;
      version  |= firmware;

      //fprintf (stderr, "Version %16.16" PRIx64 "\n", version);


      return version;
   }

   void construct (uint32_t firmware, uint32_t software)
   {
      m_w64 = compose (firmware, software);
      return;
   }

   static uint32_t firmware (uint64_t m_64) { return m_64 >>  0; }
   static uint32_t software (uint64_t m_64) { return m_64 >> 32; }

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

   uint32_t      location () const { return     m_location; }
   uint64_t  serialNumber () const { return m_serialNumber; }
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



/* ---------------------------------------------------------------------- *//*!

  \class Originator
  \brief Information about the physical entity (RCE) producing the data
         and the software and firmware running on it
                                                                          */
/* ---------------------------------------------------------------------- */
class Originator : public Header2
{
   static const int Version = 0;
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
      //fprintf (stderr, "Fw/Sw = %8.8" PRIx32 " %8.8" PRIx32 "\n", fw_version, sw_version);
      int size = m_body.construct (fw_version,
                                   sw_version,
                                   rptSwTag,
                                   nrptSwTag,
                                   serialNumber,
                                   location,
                                   groupName,
                                   ngroupName);

      size += sizeof (Header2);
      Header2::construct (static_cast<int>
                          (Header<fragment::Type::Data>::RecType::Originator), 
                          (size + sizeof (uint64_t) - 1) / sizeof (uint64_t),
                          Version);

      return size;
   }
public:
   OriginatorBody m_body;
} __attribute__ ((packed));
/* ---------------------------------------------------------------------- */
}


template<enum fragment::Type TYPE>
inline uint64_t fragment::Header<TYPE>:: compose (uint8_t  subtype,
                                                  uint8_t   naux64,
                                                  uint32_t     n64)
{
   uint64_t w64 = Header0::compose (static_cast<int>(TYPE),
                                    subtype,
                                    fragment::Pattern,
                                    naux64,
                                    n64);
   return w64;
}


template<enum fragment::Type TYPE>
inline uint64_t fragment::Trailer<TYPE>::compose (uint8_t   subtype,
                                                  int        naux64,
                                                  uint32_t      n64)
{
   uint32_t bridge = fragment::Pattern;
   uint64_t w64 = 
      PDD_INSERT64C (Header0::Mask::Format,  Header0::Offset::Format,        0)
    | PDD_INSERT64C (Header0::Mask::Type,    Header0::Offset::Type,       TYPE)
    | PDD_INSERT64C (Header0::Mask::NAux64,  Header0::Offset::NAux64,   naux64)
    | PDD_INSERT64C (Header0::Mask::SubType, Header0::Offset::SubType, subtype)
    | PDD_INSERT64C (Header0::Mask::Bridge,  Header0::Offset::Bridge,   bridge)
    | PDD_INSERT64C (Header0::Mask::Length,  Header0::Offset::Length,      n64);

   return w64;
}


/*
#undef PDD_INSERT
#undef PDD_INSERTC

#undef PDD_INSERT32
#undef PDD_INSERT32C

#undef PDD_INSERT64
#undef PDD_INSERT64C
*/
}

#endif
