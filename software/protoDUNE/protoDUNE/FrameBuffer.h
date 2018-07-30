//-----------------------------------------------------------------------------
// File          : FrameBuffer.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 09/18/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    Class to contain and track a microslice buffer.
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
//
//       DATE WHO WHAT 
// ---------- --- ------------------------------------------------------------
// 2018.02.06 jjr Added a status mask to accumulate error/status bits
// 2017.07.11 jjr Moved many methods from .cpp file to be inlined
//                Added method to get the frame address as a 64 bit number
// 2017.05.27 jjr Added setData to set fields separately.  
// 2016.11.05 jjr Added receive frame sequence number
//
//
// 09/18/2014: created
//-----------------------------------------------------------------------------

#ifndef __FRAME_BUFFER_H__
#define __FRAME_BUFFER_H__

#include "TimingClockTicks.h"

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

class FrameBuffer {
public:
   FrameBuffer  ();
  ~FrameBuffer  ();

   enum StatusMask
   {
      Missing = 1    /*!< Frame has missing time samples */
   };

   /* ------------------------------------------------------------------- *//*!

      \enum   Type
      \brief  Enumerates the different types of frames that are delivered
              from the firmware
                                                                          */
   /* ------------------------------------------------------------------- */
   enum class Type
   {
      Unknown = -1,
      Data    =  1
   };
   /* ------------------------------------------------------------------- */



   /* ------------------------------------------------------------------- *//*!

      \enum   DataType
      \brief  Enumerates the different record types that can occur in a 
              data frame
                                                                          */
   /* ------------------------------------------------------------------- */
   enum class DataType
   {
      WibFrame   = 1,
      Transposed = 2,
      Compressed = 3
   };
   /* ------------------------------------------------------------------- */

        

   // -------------------------------------------------------
   // Setters
   // -------
   // The values that are set are thus that are used both
   // on the receive side and the transmit side.  Values
   // that are contained in the data packet itself, primarily
   // in the trailer words, for efficiency reasons, are not
   // extracted until they are needed in the transmitted. Given
   // that most packets are not transmitted this is a win.
   // -------------------------------------------------------
   void         setData (uint32_t        index, 
                         uint8_t         *data, 
                         uint32_t         size, 
                         uint32_t rx_sequence);
   void        setIndex (uint32_t       index);
   void        setData  (uint8_t        *data);
   void        setSize  (uint32_t        size);
   void   setRxSequence (uint32_t rx_sequence);
   void    setTimeRange (uint64_t          beg, 
                         uint64_t         end);

        
   void  addStatus (StatusMask bit)
   {
      _status |= static_cast<uint32_t>(bit);
   }


   // ------------------------------------------------------------
   // Getters
   // -------
   // The getters return data both stored within the frame buffer
   // buffer and extracted from, primarily the two trailer words.
   // ------------------------------------------------------------
   uint16_t  getCsf        () const;
   uint32_t  getStatus     () const;
   int32_t   getIndex      () const;
   uint8_t  *getBaseAddr   () const;
   uint64_t *getBaseAddr64 () const;
   uint32_t  getReadSize   () const;
   uint32_t  getWriteSize  () const;
   uint32_t  getRxSequence () const;
   uint8_t   getDataFormat () const;


public:
   static uint64_t                     getTrailer (uint64_t const *d64,
                                                   int32_t      nbytes);

   static enum Type                  getFrameType (uint64_t        tlr);
   static enum DataType               getDataType (uint64_t        tlr);

   static bool                   getWibIdentifier (uint16_t     *wibId,
                                                   uint64_t const *d64,
                                                   int32_t      nbytes);

   static bool                  getTimestampRange (uint64_t   range[2],
                                                   uint64_t const *d64,
                                                   int32_t      nbytes);

   static void        getCompressedTimestampRange (uint64_t   range[2],
                                                   uint64_t const *d64);


   static void          getWibFrameTimestampRange (uint64_t   range[2],
                                                   uint64_t const *d64,
                                                   int32_t      nbytes);

private:
   uint64_t const *getTrailerAddr64 () const;

public:
   uint64_t   _ts_range[2];  /*!< The time spanned (beginning and ending  */


private:
   uint8_t          *_data;  /*!< Pointer to the data                     */
   int32_t          _index;  /*!< The DMA index, used to free the frame   */
   uint32_t          _size;  /*!< The size, in bytes, of the frame        */
   uint32_t   _rx_sequence;  /*!< The received sequence count             */
   uint32_t        _status;
};
/* ====================================================================== */




/* ====================================================================== */
/* BEGIN: IMPLEMENTATION                                                  */
/* ---------------------------------------------------------------------- */
/* BEGIN: GETTERs                                                         */
/* ---------------------------------------------------------------------- *//*!

  \brief Set the basic parameters of an incoming frame
  
  \param[in]       index  The DMA buffer index of the frame
  \param[in]        data  The virtual address  of the frame
  \param[in]        size  The size, in bytes,  of the frame
  \param[in] rx_sequence  The received sequence number of the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setData (uint32_t       index, 
                                  uint8_t        *data,
                                  uint32_t        size,
                                  uint32_t rx_sequence)
{
    _index       = static_cast<int32_t>(index);
    _data        = data;
    _size        = size;
    _rx_sequence = rx_sequence;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief     Sets the DMA frame buffer index
  \param[in] index  The DMA buffer index of the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setIndex (uint32_t index) { _index = index; }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief     Sets the DMA frame buffer virtual address
  \param[in] data  The virtual address  of the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setData (uint8_t *data) { _data  =  data; }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief     Sets the size, in bytes, of the frame buffer
  \param[in] size  The size, in bytes, of the frame buffer
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setSize (uint32_t size) { _size  =  size; } 
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setRxSequence (uint32_t rx_sequence) 
{
   _rx_sequence = rx_sequence; 
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Sets the time range spanned by this frame
  
  \param[in] beg  The beginning time
  \param[in] end  The endding   time

  \note
   The ending time is generally defined to be the last timestamp plus
   the sampling time, i.e. 1 sample beyond the last strobe time. Doing
   so makes the difference of the beginning and ending times the duration.
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::setTimeRange (uint64_t beg, uint64_t end)
{
   _ts_range[0] = beg;
   _ts_range[1] = end;
   _status      =   0;
}
/* ---------------------------------------------------------------------- */
/* END: SETTERs                                                           */
/* ====================================================================== */



/* ====================================================================== */
/* BEGIN: GETTERs                                                         */
/* ---------------------------------------------------------------------- *//*!

  \brief   Returns a const pointer to first of the two trailer words
  \return  A const pointer to first of the two trailer words
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t const *FrameBuffer::getTrailerAddr64 () const
{
   uint64_t const *p64 = getBaseAddr64 () 
                       + getReadSize () / sizeof (uint64_t) 
                       - 2;
   return p64;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Return the frame buffer's DMA index
  \return  The frame buffer's DMA index
                                                                          */
/* ---------------------------------------------------------------------- */
inline int32_t FrameBuffer::getIndex() const { return(_index); }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Return the base address of the frame buffer as an 8-bit pointer
  \return  The base address of the frame as a 8-bit pointer
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint8_t *FrameBuffer::getBaseAddr () const { return (_data); }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Return the frame buffer size as read, in bytes
  \return The frame buffer size, in bytes

  \note
   This is the size of the frame as it was read from by the DMA transfer.
   It includes any header or trailer words.
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::getReadSize () const { return(_size); }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Return the frame buffer size, as to be written, in bytes
  \return The frame buffer size, in bytes

  \note
   This is the size of the frame to be written. This excludes any header
   or trailer words.
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::getWriteSize () const 
{ 
   // Remove the two trailer words
   return(_size) - 2 * sizeof (uint64_t);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Return the frame buffer's received sequence number
  \return The frame buffer's received sequence number
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::getRxSequence () const { return _rx_sequence; }
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Return the base address of the frame as a 64-bit pointer
  \return  The base address of the frame as a 64-bit pointer
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t *FrameBuffer::getBaseAddr64 () const 
{
   return reinterpret_cast<uint64_t *>(_data);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Gets status of this frame expressed as a mask of error bits.
  \return The frame status
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint32_t FrameBuffer::getStatus () const
{
   ///uint64_t const *p64 = getTrailerAddr64 ();
   ///uint32_t status = p64[0] >> 32;
   
   return _status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Gets format as the data format
  \return The data format
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint8_t FrameBuffer::getDataFormat () const
{
   uint64_t const *p64 = getTrailerAddr64 ();
   uint8_t      format = (p64[0] >> 24) & 0xf;

   return format;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 
   \brief  Extracts the WIB Crate.Slot.Fiber identifier from the data 
   \return The reformatted identifier reformatted as 
           Crate(5).Slot(3).Fiber(3)
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint16_t FrameBuffer::getCsf () const
{
   // ---------------------------------------------------------------
   // 2018.07.24 -- jjr
   // -----------------
   // Corrected the extraction mask from 0xfff -> 0x3ff
   //
   // 2018.05.07 -- jjr
   // -----------------
   // With elimination of the header word, the csf is in word 0
   // Corrected error which limited the fiber to 2 bits, should be 3
   //
   // Need to swap slot and crate
   // Id = slot.3 | crate.5 | fiber.3 - Crate.5 | Slot.3 | Fiber.3
   // --------------------------------------------------------------

   uint64_t const *p64 = reinterpret_cast<decltype(p64)>(_data);
   

   uint16_t    id = (p64[0] >> 13) & 0x3ff;
   uint16_t crate = (id     >>  3) &  0x1f;
   uint16_t  slot = (id     >>  8) &  0x07;
   uint16_t fiber = (id     >>  0) &  0x07;
   uint16_t   csf = (crate << 6) | (slot << 3) | fiber;
   
   return csf;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief  Find and return the trailer word
  \return The trailer word containing the frame and record types

  \param[in]    d64  Pointer to the frame
  \param[in] nbytes  The number of bytes in the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline uint64_t  FrameBuffer::getTrailer (uint64_t const *d64,
                                          int32_t      nbytes)
{
   uint64_t  tlr =  d64[nbytes/sizeof (*d64) - 2];
   return tlr;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Extracts the frame type from the frame's trailer word
  \return The frame type

  \param[in]  tlr  The trailer word
                                                                          */
/* ---------------------------------------------------------------------- */
inline enum FrameBuffer::Type FrameBuffer::getFrameType (uint64_t tlr)
{
   enum Type type = static_cast<Type>((tlr >> 28) & 0xf);
   return    type;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Extracts the record type from the frame's trailer word
  \return The record type.

  \param[in]  tlr  The trailer word

  \par
   The frame type must have been previously been verified that it is 
   a data frame.
                                                                          */
/* ---------------------------------------------------------------------- */
inline enum FrameBuffer::DataType 
            FrameBuffer::getDataType (uint64_t tlr)
{
   enum DataType type = static_cast<enum DataType>((tlr >> 24) & 0xf);
   return        type;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Return the WIB identifier for the specified data
  \retval == true,  successfully located  the WIB identifier
  \retval == false, unsuccess in locating the WIB identifier

  \param[out]  wibId  Pointer to the WIB identifier
  \param[ in]    d64  Beginning of the frame
  \param[ in] nbytes  The number of bytes in the frame
                                                                          */
/* ---------------------------------------------------------------------- */
inline bool FrameBuffer::getWibIdentifier (uint16_t      *wibId,
                                           uint64_t const  *d64,
                                           int32_t       nbytes)
{
   // -------------------------------------
   // Locate the word containing the WIB ID
   // -------------------------------------
   uint64_t                     tlr = getTrailer  (d64, nbytes);
   enum FrameBuffer::Type frameType = getFrameType (tlr);
   
   if (frameType == FrameBuffer::Type::Data)
   {
      // ---------------------------------------------
      // 2018.07.24 -- jjr
      // -----------------
      // Corrected extraction mask from 0xfff -> 0x3ff
      // ---------------------------------------------
      enum DataType dataType = getDataType (tlr);
  
      if (dataType == DataType::WibFrame)
      {
         *wibId = (d64[0] >> 13) & 0x3ff;
         return true;
      }
      else if (dataType == DataType::Compressed)
      {
         *wibId = (d64[1] >> 13) & 0x3ff;
         return true;
      }
   }

   return false;
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
inline bool FrameBuffer::getTimestampRange (uint64_t     range[2],
                                            uint64_t const   *d64,
                                            int32_t        nbytes)
{
   uint64_t                     tlr =  getTrailer (d64, nbytes);
   enum FrameBuffer::Type frameType =  getFrameType (tlr);


   if ( frameType == Type::Data)
   {
      enum DataType recType = getDataType (tlr);
      if (recType == DataType::Compressed)
      {
         //// fprintf (stderr, "TpcData:Compress:");
         getCompressedTimestampRange (range, d64);
      }
      else if (recType == FrameBuffer::DataType::WibFrame)
      {
         //// fprintf (stderr, "TpcData:WibFrame:");
         getWibFrameTimestampRange (range, d64, nbytes);
      }
      else
      {
         fprintf (stderr, "Bad trailer identifier = %16.16" PRIx64 "\n",
                  tlr);
         return false;
      }

      /// fprintf (stderr, "%16.16" PRIx64 " %16.16" PRIx64 "%8.8" PRIx32 "\n", 
      ///         range[0], range[1], nbytes);

      return true;

   }
   else
   {
      fprintf (stderr, "Bad trailer identifier = %16.16" PRIx64 "\n",
               tlr);

      return false;
   }

}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Hokey routine to get the timestamp range of the packet.
  \return  The  timestamp of the initial WIB frame

  \param[in]  range The timestamp range of this packet
  \param[in]    d64 64-bit pointer to the data packet
                                                                          */
/* ---------------------------------------------------------------------- */
inline void FrameBuffer::getCompressedTimestampRange (uint64_t   range[2],
                                                      uint64_t const *d64)
{
   range[0] = d64[2];
   range[1] = d64[3] + TimingClockTicks::PER_SAMPLE;

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
inline void FrameBuffer::getWibFrameTimestampRange(uint64_t     range[2],
                                                   uint64_t const   *d64,
                                                   int32_t        nbytes)
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
/* END: GETTERs                                                           */
/* ---------------------------------------------------------------------- */
/* END: IMPLEMENTATION                                                    */
/* ====================================================================== */

#endif

