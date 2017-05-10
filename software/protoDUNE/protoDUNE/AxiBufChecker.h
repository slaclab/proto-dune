// -*-Mode: C++;-*-

#ifndef __AXIBUFCHECKER_H__
#define __AXIBUFCHECKER_H__

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


#include <stdint.h>
#include <stddef.h>



/* ---------------------------------------------------------------------- *//*!
 *
 *  \class  AxiBufStatus
 *  \brief  Captures the current status of a given AXI buffer
 *
\* ---------------------------------------------------------------------- */
class AxiBufChecker
{
public:
   AxiBufChecker () :
      m_status (Unknown),
      m_irx    (      0)
   {
      return;
   }


public:
   int check_buffer (uint8_t       *buf, 
                     int            idx,
                     unsigned int nsize);

   bool begin_range (uint32_t       beg);
   bool   end_range (uint32_t       end);


   static int check_buffers   (uint16_t      *bad,
                               uint8_t     **bufs, 
                               unsigned int nbufs, 
                               unsigned int nsize);
      

   static int extreme_vetting (AxiBufChecker *abc,
                               int             fd,
                               int          trial,
                               uint8_t     **bufs,
                               int        nrxbufs,
                               int          nbufs,
                               int           size);


   struct Range
   {
      uint16_t   beg;
      uint16_t   end;
   };

   /* ------------------------------------------------------------------- *//*!
    *
    * \enum   Status
    * \brief  The current status of the buffer
    *
   \* --------------------------------------------------------------------*/
   enum Status
   {
      Bad     = -1, /*!< Status is bad, there are one or more bad ranges  */
      Unknown =  0, /*!< Status is unknown                                */
      Skipped =  1, /*!< Buffer skipped for checking                      */
      Good    =  2, /*!< Status is good                                   */
   };
   /* --------------------------------------------------------------------*/


   Status   m_status;  /*!< The buffer status                             */
   uint8_t     m_irx;  /*!< The number of bad ranges                      */
   Range m_ranges[4];  /*!< The first 4 bad ranges                        */
};
/* ---------------------------------------------------------------------- */

#endif
