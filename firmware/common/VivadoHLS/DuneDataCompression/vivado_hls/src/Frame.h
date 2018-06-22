// -*-Mode: C++;-*-

#ifndef _DUNE_DATA_COMPRESSION_FRAME_H_
#define _DUNE_DATA_COMPRESSION_FRAME_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     Frame.h
 *  @brief    Defines the output frame
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
 *  DUNE
 *
 *  @author
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  2018/04/18
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
   2018.04.18 jjr Created, split off from DuneDataCompressionTypes.h
   
\* ---------------------------------------------------------------------- */


#include <stdint.h>


/* ---------------------------------------------------------------------- *//*!
 *
 *  \namespace Frame
 *  \brief     Defines the layout and construction of the framing word
 *
 *  \par
 *   This layout is generic and fixed.  All versions of the header and
 *   trailer will use this layout, although certainly with different
 *   version words (by definition) and potentially different pattern
 *   words.
 *
 *   This done as a namespace so that new versions can be added to it.
 *   This takes advantage of the fact the namespaces are open, while
 *   classes are not.
 *
\* ---------------------------------------------------------------------- */
namespace Frame
{
   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Size
    *  \brief Defines the sizes of bit fields of the framing word.
    *
   \* ------------------------------------------------------------------- */
   enum class Size
   {
      VERSION =  4, /*!< Size, in bits, of the version field              */
      PATTERN = 28, /*!< Size, in bits, of the pattern field              */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Offset
    *  \brief Defines the right justified bit offset for the bit fields
    *         of the framing word.
    *
   \* ------------------------------------------------------------------- */
   enum class Offset
   {
      VERSION =  0, /*!< Right justified offset of the version  field     */
      PATTERN =  4  /*!< Right justified offset of the pattern field      */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Mask
    *  \brief Defines the right justified masks for the bit fields of
    *         the framing word.
    *
   \* ------------------------------------------------------------------- */
   enum class Mask
   {
      VERSION = 0xf,       /*!< Right justified mask of the version field */
      PATTERN = 0xfffffff, /*!< Right justified mask of the pattern field */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief  Constructs a framing word from the frame's version number
    *          and pattern word.
    *  \return The constructed framing word
    *
    *  \param[in] version The frame's format version number
    *  \param[in] pattern The frame's pattern word.
    *
   \* ------------------------------------------------------------------- */
   static inline uint32_t framer (uint32_t version, uint32_t pattern)
   {
      uint32_t w = ((pattern & static_cast<uint32_t>(Mask  ::PATTERN))
                            << static_cast<uint32_t>(Offset::PATTERN))
                 | ((version & static_cast<uint32_t>(Mask  ::VERSION))
                            << static_cast<uint32_t>(Offset::VERSION));
      return w;
   }
   /* ------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
} /* END: namespace FRAME                                                 */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \namespace Frame::V0
 *  \brief     Defines the layout and construction of V0 frame header
 *             and trailer.
 *
\* ---------------------------------------------------------------------- */
namespace Frame { namespace V0
{
   /* ------------------------------------------------------------------- *//*!
    *
    *  \var   Version
    *  \brief Defines this as version 0
    *
    *  \var   Pattern
    *  \brief Defines the pattern word for the version 0 frame
    *
    *  \var   Framer
    *  \brief Defines the framing word for the version 0 frame
    *
   \* ------------------------------------------------------------------- */
   const uint32_t Version =         0;
   const uint32_t Pattern = 0x8b309e2;
   const uint32_t Framer  =
                ((Pattern & static_cast<uint32_t>(Frame::Mask  ::PATTERN))
                         << static_cast<uint32_t>(Frame::Offset::PATTERN))

              | ((Version & static_cast<uint32_t>(Frame::Mask  ::VERSION))
                         << static_cast<uint32_t>(Frame::Offset::VERSION));
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class FrameType
    *  \brief Defines the various types of frames
    *
   \* ------------------------------------------------------------------- */
   enum class FrameType
   {
      DISCARD = 0,  /*!< Discard frame                                    */
      DATA    = 1   /*!< Data frame                                       */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Size
    *  \brief Defines the size, in bits, for the bit fields of the
    *         identifier.
    *
   \* ------------------------------------------------------------------- */
   enum class Size
   {
      NBYTES     = 24,  /*!< Size, in bits, of nbytes field               */
      FRAMETYPE  =  4,  /*!< Size, in bits, of frame type field           */
      RECORDTYPE =  4,  /*!< Size, in bits, of record type field          */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Offset
    *  \brief Defines the right justified bit offset for the bit fields
    *         of the identifier.
    *
   \* ------------------------------------------------------------------- */
   enum class Offset
   {
      NBYTES     =  0, /*!< Right justified offset of      nbytes field   */
      FRAMETYPE  = 24, /*!< Right justified offset of  frame type field   */
      RECORDTYPE = 28, /*!< Right justified offset of record type field   */
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class Mask
    *  \brief Defines the right justified masks for the bit fields of
    *         the identifier.
    *
   \* ------------------------------------------------------------------- */
   enum class Mask
   {
      NBYTES     = 0xffffff,/*!< Right justified mask, record type field  */
      FRAMETYPE  = 0xf,     /*!< Right justified mask,  frame type field  */
      RECORDTYPE = 0xf,     /*!< Right justified mask,  data type field   */

   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- */
   /* Generic, header, trailer and identifier construction routines       */
   /* ------------------------------------------------------------------- */
   template<typename RecordType>
   static uint32_t identifier (FrameType   frameType,
                               RecordType recordType,
                               uint32_t       nbytes);

   template<typename RecordType>
   static uint32_t identifier (FrameType    frameType,
                               RecordType  recordType);

   static uint64_t header     (uint32_t identifier);
   static uint64_t trailer    (uint32_t identifier);
   static uint64_t trailer    (uint32_t identifier, uint32_t nbytes);
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!
    *
    *  \enum  class DataType
    *  \brief Enumerates the valid record types when the frame type = DATA
   \* ------------------------------------------------------------------- */
   enum class DataType
   {
      RESERVED   = 0,  /*!< Unused, reserved                              */
      WIB        = 1,  /*!< Asis WIB format,unprocessed                   */
      TRANSPOSED = 2,  /*!< Transposed channel <-> time order             */
      COMPRESSED = 3,  /*!< Compressed                                    */
   };
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- */
   /* FrameType = DATA construction routines                              */
   /* ------------------------------------------------------------------- */
   static uint32_t identifier (DataType dataType);
   static uint32_t identifier (DataType dataType, uint32_t nbytes);
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for the specified frame and
    *         record type.
    *
    *  \param[in]  frameType The frame type
    *  \param[in] recordType The record type
    *  \param[in[     nbytes The number of bytes in this frame
    *
    *  \par
    *   This can only be used when the number of bytes is known. For
    *   example, When the frame is serially filled with a variable number
    *   of bytes, the number of bytes is generally not known until the last
    *   data has been written.  In that case, one should use the call
    *   that omits this parameter.
    *
    *  \warning
    *   It is up to the caller to ensure that the \a recordType is drawn
    *   from the set associated with the specified \a frameType. In
    *   general,these routines should not be called directly by the user,
    *   but by used as the basis of routines that onstruction frame type
    *   specific identifiers. Such routine can ensure that the \a recordType
    *   is consistent with the \a frameType
    *
   \* ------------------------------------------------------------------- */
   template<typename RecordType>
   static inline uint32_t identifier (FrameType   frameType,
                                      RecordType recordType,
                                      uint32_t       nbytes)
   {
      uint32_t id = ((uint32_t)( frameType) << (int)Offset::FRAMETYPE)
                  | ((uint32_t)(recordType) << (int)Offset::RECORDTYPE)
                  | ((uint32_t)(    nbytes) << (int)Offset::NBYTES);
      return id;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for the specified frame and
    *         record type.
    *
    *  \param[in]  frameType The frame type
    *  \param[in] recordType The record type
    *
    *  \warning
    *   It is up to the caller to ensure that the \a recordType is drawn
    *   from the set associated with the specified \a frameType. In
    *   general,these routines should not be called directly by the user,
    *   but by used as the basis of routines that onstruction frame type
    *   specific identifiers. Such routine can ensure that the \a recordType
    *   is consistent with the \a frameType
    *
   \* ------------------------------------------------------------------- */
   template<typename RecordType>
   static inline uint32_t identifier (FrameType    frameType,
                                      RecordType  recordType)
   {
      uint32_t id = ((uint32_t)( frameType) << (int)Offset::FRAMETYPE)
                  | ((uint32_t)(recordType) << (int)Offset::RECORDTYPE)
                  | ((uint32_t)(         0) << (int)Offset::NBYTES);
      return id;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief  Constructs a verion = 0 frame header
    *  \return The V0 frame header
    *
    *  \param[in] identifier  The previously construct V0 identifier
    *
   \* ------------------------------------------------------------------- */
   static inline uint64_t header  (uint32_t identifier)
   {
      uint64_t hdr = ((uint64_t)identifier << 32) | Framer;
      return   hdr;
   };
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief  Constructs a version = 0 frame trailer
    *  \return The V0 frame trailer
    *
    *  \param[in] identifier  The previously construct V0 identifier.
    *
    *  \warning
    *   It is the user's responsibility to ensure that the \a identifier
    *   is consistent with that used to construct the header.
    *
   \* ------------------------------------------------------------------- */
   static inline uint64_t trailer (uint32_t identifier)
   {
      uint64_t tlr = ~(((uint64_t)Framer << 32) | identifier);
      return   tlr;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
     *
     *  \brief  Constructs a version = 0 frame trailer with the length
     *          field.
     *  \return The V0 frame trailer
     *
     *  \param[in] identifier  The previously construct V0 identifier.
     *  \param[in]     nbytes  The length, in bytes, of the framee
     *
     *  \warning
     *   It is the user's responsibility to ensure that the \a identifier
     *   is consistent with that used to construct the header and that
     *   the NBYTES field is clear.
     *
    \* ------------------------------------------------------------------- */
   static inline uint64_t trailer (uint32_t identifier, uint32_t nbytes)
   {
      uint64_t tlr =  trailer (identifier | (nbytes << (int)Offset::NBYTES));
      return   tlr;
   }
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for a frame type of DATA
    *
    *  \param[in] dataType The type of data
    *
    *  \par
    *   This call should only be used when the number of bytes is not
    *   known when constructing the identifier.
    *
   \* ------------------------------------------------------------------- */
   static inline uint32_t identifier (DataType dataType)
   {
      uint32_t id = identifier<DataType> (FrameType::DATA, dataType);
      return id;
   }
   /* ------------------------------------------------------------------- */


   /* ------------------------------------------------------------------- *//*!
    *
    *  \brief Constructs the identifier for a frame type of DATA
    *
    *  \param[in] dataType The type of data
    *  \param[in[   nbytes The number of bytes in this frame
    *
    *  \par
    *   This can only be used when the number of bytes is known. For
    *   example, When the frame is serially filled with a variable number
    *   of bytes, the number of bytes is generally not known until the last
    *   data has been written.  In that case, one should use the call
    *   that omits this parameter.
    *   not
    *
   \* ------------------------------------------------------------------- */
   static uint32_t identifier (DataType dataType, uint32_t nbytes)
   {
      uint32_t id = identifier<DataType> (FrameType::DATA, dataType, nbytes);
      return id;
   }
   /* ------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
}}  /* END: namespace Frame::V0                                           */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 * \class   Identifier
 *  \brief  Identifies the contents of the record
 *
\* ---------------------------------------------------------------------- */
class Identifier
{
public:
   enum class FrameType
   {
      DISCARD = 0,  /*!< Discard frame                                   */
      DATA    = 1   /*!< Data frame                                      */
   };

public:
   Identifier () { return; }

   enum class DataType
   {
      RESERVED   = 0,  /*!< Unused, reserved                              */
      WIB        = 1,  /*!< Asis WIB format,unprocessed                   */
      TRANSPOSED = 2,  /*!< Transposed channel <-> time order             */
      COMPRESSED = 3,  /*!< Compressed                                    */
   };

   enum class Size
   {
      NBYTES     = 24,  /*!< Size, in bits, of nbytes field               */
      RECORDTYPE =  4,  /*!< Size, in bits, of record type field          */
      FRAMETYPE  =  4   /*!< Size, in bits, of frame type field           */
   };

   enum class Offset
   {
      NBYTES     =  0, /*!< Right justified offset of      nbytes field   */
      RECORDTYPE = 24, /*!< Right justified offset of   data type field   */
      FRAMETYPE  = 28  /*!< Right justified offset of record type field   */
   };

   enum class Mask
   {
      NBYTES     = 0xf,     /*!< Right justified mask, nbytes field       */
      RECORDTYPE = 0xf,     /*!< Right justified mask, data type field    */
      FRAMETYPE  = 0xffffff,/*!< Right justified mask, record type field  */
   };

   static uint32_t identifier (FrameType   frameType,
                               DataType     dataType,
                               uint32_t       nbytes);

   static uint32_t identifier (FrameType   frameType,
                               DataType     dataType);
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Completely generic initializer
 *
 *  \param[in] frameType  The frame type
 *  \param[in] recordType The record type.  This must be consistent
 *                        with the types allowed by the \a frameType,
 *  \param[in] nbytes     The number of bytes in this frame.
 *
\* ---------------------------------------------------------------------- */
inline uint32_t Identifier::identifier (Identifier::FrameType frameType,
                                        DataType               dataType,
                                        uint32_t                 nbytes)
{
   uint32_t id = ((uint32_t)(frameType) << (int)Identifier::Offset::FRAMETYPE)
               | ((uint32_t)( dataType) << (int)Identifier::Offset::RECORDTYPE)
               | ((uint32_t)(   nbytes) << (int)Identifier::Offset::NBYTES);
   return id;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Completely generic initializer, no length
 *
 *  \param[in] frameType  The frame type
 *  \param[in] recordType The record type.  This must be consistent
 *                        with the types allowed by the \a frameType,
 *
\* ---------------------------------------------------------------------- */
inline uint32_t Identifier::identifier (Identifier::FrameType frameType,
                                        DataType               dataType)
{
   uint32_t id = ((uint32_t)(frameType) << (int)Identifier::Offset::FRAMETYPE)
               | ((uint32_t)( dataType) << (int)Identifier::Offset::RECORDTYPE);
   return id;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *   \brief Defines the frame/record header word
 *   
\* ---------------------------------------------------------------------- */
class Header
{
private:
   Header (uint32_t identifier, uint32_t version);

public:
   class Version
   {
   public:
   /* ----------------------------------------------------------------------- *//*!
    *
    *  \enum  WF_VERSION_S
    *  \brief Bit size of the version number fields
    *
   \* ----------------------------------------------------------------------- */
   enum class Size
   {
      RELEASE = 8,  /*!< Size of the version release field, ==0 for production,
                         other values indicate a development version         */
      PATCH   = 8,  /*!< Size of the version   patch field                   */
      MINOR   = 8,  /*!< Size of the version   minor field                   */
      MAJOR   = 8,  /*!< Size of the version   major field                   */
   };
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
    *
    *   \enum  Offset
    *   \brief Bit offsets of the version number fields
    *
   \* ---------------------------------------------------------------------- */
   enum class Offset
   {
      RELEASE =  0, /*!< Bit offset of the version release field             */
      PATCH   =  8, /*!< Bit offset of the version   patch field             */
      MINOR   = 16, /*!< Bit offset of the version   minor field             */
      MAJOR   = 24, /*!< Bit offset of the version   major field             */
   };
   /* ---------------------------------------------------------------------- */



   /* ---------------------------------------------------------------------- *//*!
    *
    *   \enum  Mask
    *   \brief Right justified bit mask of the version number fields
    *
   \* ---------------------------------------------------------------------- */
   enum class Mask
   {
      RELEASE = 0xff, /*!< Bit offset of the version release field           */
      PATCH   = 0xff, /*!< Bit offset of the version   patch field           */
      MINOR   = 0xff, /*!< Bit offset of the version   minor field           */
      MAJOR   = 0xff, /*!< Bit offset of the version   major field           */
   };
   /* ---------------------------------------------------------------------- */


   public:
      static uint32_t version (uint8_t   major,
                               uint8_t   minor,
                               uint8_t   patch,
                               uint8_t release);
   };
   /* ---------------------------------------------------------------------- */


   /* ---------------------------------------------------------------------- *\
      -- Deprecated because of RSSI
   static uint64_t header  (enum Identifier::FrameType frametype,
                            enum Identifier::DataType   dataType,
                            uint32_t                      nbytes,
                            uint32_t                     version);

   static uint64_t header  (enum Identifier::DataType   datatype,
                            uint32_t                      nbytes,
                            uint32_t                     version);

   static uint64_t header  (uint32_t identifier,
                            uint32_t    version);
   \* ---------------------------------------------------------------------- */


   uint64_t      m_ui64;
};


/* ---------------------------------------------------------------------- *//*!
 * 
 *  \brief Defines the frame/record trailer word
 *  
\* ---------------------------------------------------------------------- */
class Trailer
{
public:
   Trailer (int nbytes);

   static const uint32_t Pattern = 0x708b309e;

   static uint64_t trailer (uint32_t version);

public:
   uint64_t m_trailer;
};
/* ---------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
/* ----------------------------------------------------------------------- */
#if 0
static inline void print_header (uint64_t hdr)
{
   std::cout << "Header: " << std::setw(16) << std::hex << hdr << std::endl;
   return;
}
#endif

static inline void print_trailer (uint64_t tlr)
{
   std::cout << "Trailer: " << std::setw(16) << std::hex << tlr << std::endl;
   return;
}
/* ----------------------------------------------------------------------- */
#else
/* ----------------------------------------------------------------------- */

// --------------------------------------------------------------
// These must be defined away.
// While Vivado understands how to ignore std::cout, it does not
// know how to ignore things like std::setw and std::hex
// --------------------------------------------------------------
#define  print_header(_hdr)
#define print_trailer(_tlr)
/* ----------------------------------------------------------------------- */
#endif
/* ----------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *   \def   VERSION_COMPOSE
 *   \brief Composes a full version number from its components
 *
 *   \param[in] _major  The major version number.  This should be bumped
 *                      when an incompatible change is made. Generally
 *                      this means that the receiving software must be
 *                      modified.
 *  \param[in]  _ninor  The minor version number.  This should be bumped
 *                      when a backwardly compatiable change is made. This
 *                      happens when a new feature is added, for example,
 *                      a new record type. Existing software can ignore
 *                      this new record type, and can continue to operate
 *                      as before. Naturally, it cannot take advantage or
 *                      use this new record type until that receiving
 *                      software has been upgraded.
 *  \param[in] _patch   The patch number. This should be used to indicate
 *                      a bug fix. This change does not effective the
 *                      interface but by bumping the patch number, a record
 *                      is of the change is maintained
 * \param[in] _release  This should be set to 0 for all production releases
 *                       and non 0 for development releases
 *
 * \par
 *  This is provided as a macro because the version number is a static
 *  concept as composed to a run time concept.
 *
\* ---------------------------------------------------------------------- */
#define VERSION_COMPOSE(_major, _minor, _patch, _release)                  \
                  ( ((  _major) << (int)Header::Version::Offset::MAJOR  )  \
                  | ((  _minor) << (int)Header::Version::Offset::MINOR  )  \
                  | ((  _patch) << (int)Header::Version::Offset::PATCH  )  \
                  | ((_release) << (int)Header::Version::Offset::RELEASE))
/* ---------------------------------------------------------------------- */




inline uint32_t Header::Version::version (uint8_t   major,
                                          uint8_t   minor,
                                          uint8_t   patch,
                                          uint8_t release)
{
   #pragma HLS INLINE

   uint32_t w = ((uint32_t)major   << (int)Header::Version::Offset::MAJOR)
              | ((uint32_t)minor   << (int)Header::Version::Offset::MINOR)
              | ((uint32_t)patch   << (int)Header::Version::Offset::PATCH)
              | ((uint32_t)release << (int)Header::Version::Offset::RELEASE);

   return w;
};


#if 0
inline Header::Header (uint32_t identifier,
                       uint32_t    version) :
   m_ui64 (header (identifier, version))
{
   #pragma HLS INLINE

   return;
}

inline uint64_t Header::header (uint32_t identifier,
                                uint32_t    version)
{
   #pragma HLS INLINE

   uint64_t w = ((uint64_t)version << 32) | identifier;
   return w;
}

inline uint64_t Header::header  (enum Identifier::DataType    dataType,
                                 uint32_t                       nbytes,
                                 uint32_t                      version)
{
   #pragma HLS INLINE

   uint32_t id = Identifier::identifier (Identifier::FrameType::DATA,
                                         dataType,
                                         nbytes);
   uint64_t w  = header (id, version);
   return w;

}

inline uint64_t Header::header  (enum Identifier::FrameType  frameType,
                                 enum Identifier::DataType    dataType,
                                 uint32_t                       nbytes,
                                 uint32_t                      version)
{
   #pragma HLS INLINE

   uint32_t id = Identifier::identifier (frameType, dataType, nbytes);
   uint64_t w = header (id, version);
   return w;
}
#endif

inline uint64_t Trailer::trailer (uint32_t version)
{
   #pragma HLS INLINE

   uint64_t w = ((uint64_t)(Trailer::Pattern) << 32) | version;
   return w;
}


/* ----------------------------------------------------------------------- */
/* Convenience Methods                                                     */
/* ----------------------------------------------------------------------- */
/* Deprecated -- RSSI
static void         prologue (AxisOut          &mAxis,
                              int                &odx,
                              bool          writeFlag,
                              int           moduleIdx);
*/

static void         epilogue (AxisOut          &mAxis,
                              int                &odx,
                              uint64_t         status);
/* ----------------------------------------------------------------------- */



#if 0
/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  If this is the first word, add the packet header
 *
 *  \param[   out]     mAxis  The output AXI stream
 *  \param[in:out]       odx  The 64-bit packet index
 *  \param[in    ] writeFlag  Write enable flags
 *  \param[in    ] moduleIdx  Which module, \e i.e. which group of
 *                            128 channels being service
 *
\* ----------------------------------------------------------------------- */
static inline void prologue (AxisOut &mAxis,
                             int       &odx,
                             bool writeFlag,
                             int  moduleIdx)
{
   #pragma HLS INLINE

   #define WIB_VERSION VERSION_COMPOSE (0, 0, 0, 1)

   if (odx == 0)
   {
      // --------------------------------------------------------------
      // Because the output packet maybe terminated at anytime, the
      // size of the packet cannot be known at this time. This field
      // in the record header will have to be filled in by the receiving
      // software.
      //
      // For integrity checking, the module index will be stuffed
      // into this unused field. Its kind of a kludge, but maybe useful.
      // --------------------------------------------------------------
      uint32_t  id = Identifier::identifier (Identifier::FrameType::DATA,
                                             Identifier::DataType::WIB,
                                             moduleIdx);

      uint64_t      hdr = Header::header (id, WIB_VERSION);
      print_header (hdr);
      commit       (mAxis, odx, writeFlag, hdr, 2, 0);
   }
}
/* ----------------------------------------------------------------------- */
#endif


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Does the processing after a full frame has been readin
 *
 *  \param[   out]     mAxis  The output AXI stream
 *  \param[in:out]       odx  The 64-bit packet index
 *  \param[    in]    status  The summeary status
 *
 *
\* ----------------------------------------------------------------------- */
static inline void epilogue (AxisOut    &mAxis,
                             int          &odx,
                             uint32_t   status)
{
   #pragma HLS INLINE

   #define PACKET_VERSION VERSION_COMPOSE (1, 0, 0, 0)


   std::cout << "Outputting @ odx = " << odx << std::endl;

   uint64_t   statusId;
   int   nbytes = odx * sizeof (uint64_t) + sizeof (statusId) + sizeof (Trailer);
    uint32_t  id = Identifier::identifier (Identifier::FrameType::DATA,
                                           Identifier::DataType::WIB,
                                           nbytes);

   statusId   = status;
   statusId <<=     32;
   statusId  |=     id;


   uint64_t tlr = Trailer::trailer (PACKET_VERSION);
   print_trailer (tlr);


   commit (mAxis, odx, true, statusId, 0, 0);
   commit (mAxis, odx, true, tlr,      0, 1);

   return;
}
/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Does the processing after a full frame has been readin
 *
 *  \param[   out]     mAxis  The output AXI stream
 *  \param[in:out]       odx  The 64-bit packet index
 *  \param[    in] frameType  The frame type (eg Identifier::FrameType::DATA)
 *  \param[    in]    status  The summeary status
 *
 *
\* ----------------------------------------------------------------------- */
static inline void epilogue (AxisOut                &mAxis,
                             int                      &odx,
                             Identifier::DataType dataType,
                             uint32_t               status)
{
   #pragma HLS INLINE

   #define PACKET_VERSION VERSION_COMPOSE (1, 0, 0, 0)


   std::cout << "Outputting @ odx = " << odx << std::endl;

   uint64_t   statusId;
   int   nbytes = odx * sizeof (uint64_t) + sizeof (statusId) + sizeof (Trailer);
    uint32_t  id = Identifier::identifier (Identifier::FrameType::DATA, dataType, nbytes);


   statusId   = status;
   statusId <<=     32;
   statusId  |=     id;


   uint64_t tlr = Trailer::trailer (PACKET_VERSION);
   print_trailer (tlr);


   commit (mAxis, odx, true, statusId, 0, 0);
   commit (mAxis, odx, true, tlr,      0, 1);

   return;
}
/* ----------------------------------------------------------------------- */

#endif
