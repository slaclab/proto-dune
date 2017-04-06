// -*-Mode: C++;-*-


#ifndef _DUNE_DATA_COMPRESSION_PACKET_H_
#define _DUNE_DATA_COMPRESSION_PACKET_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionPacket.h
 *  @brief    Interface for a circular buffer of packets, where a packet is
 *            typically 1024 timesamples for 1 channel
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
 *  russell@slac.stanford.edu
 *
 *  @par Date created:
 *  2016.06.14
 *
 *  @par Last commit:
 *  \$Date: $ by \$Author: $.
 *
 *  @par Revision number:
 *  \$Revision: $
 *
 *  @par Location in repository:
 *  \$HeadURL: $
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
   2017.02.09 jjr Removed unnecessary include of DuneDataCompressionTypes.h
   2016.11.30 jjr Removed the members m_first, m_last in favor of the
                  methods is_first, is_last
   2016.06.14 jjr Created

\* ---------------------------------------------------------------------- */



//////////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Data compression'.
// It is subject to the license terms in the LICENSE.txt file found in the
// top-level directory of this distribution and at:
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
// No part of 'DUNE Data compression', including this file,
// may be copied, modified, propagated, or distributed except according to
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////




/* ---------------------------------------------------------------------- *//*!
 *
 *  \def      PACKET_B_COUNT
 *  \brief    The number of packets in the circular buffer, express as
 *            bit count
 *
 *  \def      PACKET_K_COUNT
 *  \brief    The number of packets in the circular buffer
 *
\* ---------------------------------------------------------------------- */
#define PACKET_B_COUNT 1
#define PACKET_K_COUNT 2
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \def     PACKET_B_ADC
 * \brief   The number of bits in an ADC when stored in a packet
 *
\* ---------------------------------------------------------------------- */
#define PACKET_B_ADC         12
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 * \typedef Adcin_t
 * \brief   Typedef for an input ADC word
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint<PACKET_B_ADC>         AdcIn_t;
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 * \class Packets
 * \brief The context used to manage a circular ring of packets
 *
 * \par
 *  The circular ring of packet to be managed is composed of PACKET_K_COUNT
 *  of PACKETs of size PACKET_K_NSAMPLES. To make the implementation
 *  efficient both PACKET_K_CNT and PACKET_K_NSAMPLES are a power of 2.
 *
 *  As each time sample is acquired, it is stored in this circular buffer.
 *  Since both PACKET_K_CNT and PACKET_K_NSAMPLES are a power of 2, a simple
 *  flat index can be used as addressing. By defining a type, Cidx_t, that
 *  has the exactly the number of bits of size of the circular buffer, this
 *  index naturally wraps-around when the end of the circular buffer is
 *  reached.
 *
\* ---------------------------------------------------------------------- */
class Packets
{
public:
   Packets   ();
   void init ();

   /*
    *  MaxIndex of ADCs kept in the circular buffer
    *    -- This most be a power of 2
    */
   static const int Count    = PACKET_K_COUNT;
   static const int NSamples = PACKET_K_NSAMPLES;


   typedef ap_uint<PACKET_B_COUNT>      Pidx_t;
   typedef ap_uint<PACKET_B_NSAMPLES>   Oidx_t;

public:
   /*! Get the next PACKET buffer number/index */
   static Pidx_t packet_next     (Pidx_t pidx);
   Pidx_t        packet_next     ()     const;
   Pidx_t        set_pidx        (WibTimestamp_t timestamp);

   /* Get the previous packet buffer number/index */
   static Pidx_t packet_previous (Pidx_t pidx);
   Pidx_t        packet_previous () const;

   static WibTimestamp_t bof   (WibTimestamp_t timestamp);
   /*!< The beginning of packet timestamp                                 */

   static WibTimestamp_t eof   (WibTimestamp_t timestamp);
   /*!< The end of packet timestamp                                       */

   static Pidx_t packet_index  (WibTimestamp_t timestamp);
   /*!< Only the packet number/index                                      */

   static Oidx_t packet_offset (WibTimestamp_t timestamp);
   /*!< Only the offset from the timestamp                                */

   static bool is_first        (WibTimestamp_t timestamp);
   /*!< Is this timestamp associated with the first frame of the packet   */

   static bool is_last         (WibTimestamp_t timestamp);
   /*!< Is this timestamp associated with the last  frame of the packet   */

public:
   Pidx_t                    m_pidx; /*!< The current packet number/index */
};
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- */
/* IMPLEMENTATION                                                         */
/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief Packest constructor
 *
\* ---------------------------------------------------------------------- */
inline Packets::Packets ()
{
   #pragma HLS inline

   init ();
   return;
}
/* ---------------------------------------------------------------------- */



/* ----------------------------------------------------------------------- *//*!
 *
 *  \brief  Initializes the Packets
 *
\* ---------------------------------------------------------------------- */
inline void Packets::init ()
{
   #pragma HLS inline

   m_pidx = PACKET_K_COUNT - 1;
   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief  Return the next packet number/index starting from \a pidx
 *  \return The next packet number/index
 *
 *  \param[in] pidx  The packet index to advance
\* ---------------------------------------------------------------------- */

 Packets::Pidx_t inline Packets::packet_next (Pidx_t pidx)
 {
    #pragma HLS INLINE
    return pidx + 1;
 }
 /* --------------------------------------------------------------------- */



 /* --------------------------------------------------------------------- *//*!
  *
  *  \brief  Return the next packet number/index
  *  \return The next packet number/index
  *
 \* --------------------------------------------------------------------- */

  Packets::Pidx_t inline Packets::packet_next () const
  {
     #pragma HLS INLINE
     return packet_next (m_pidx);
  }
  /* --------------------------------------------------------------------- */



 /* --------------------------------------------------------------------- *//*!
  *
  *  \brief  Return the previous packet number/index relative to \a pidx
  *  \return The previous frame buffer number/index
  *
  *  \param[in] pidx  The packet index to retard
  *
 \* --------------------------------------------------------------------- */
 Packets::Pidx_t inline Packets::packet_previous (Pidx_t pidx)
 {
    #pragma HLS INLINE
    return pidx - 1;
 }
 /* --------------------------------------------------------------------- */



 /* --------------------------------------------------------------------- *//*!<
  *
  *  \brief  Return the previous packet number/index
  *  \return The previous packet number/index
  *
 \* --------------------------------------------------------------------- */
 Packets::Pidx_t inline Packets::packet_previous () const
 {
    #pragma HLS INLINE
    return packet_previous (m_pidx);
 }
 /* --------------------------------------------------------------------- */



 /* ---------------------------------------------------------------------- *//*!
  *
  *  \brief  Return the timestamp corresponding to the beginning of
  *          packet time.
  *  \return The beginning of packet timestamp
  *
  *  \param[in]  timestamp  The timestamp
 \* --------------------------------------------------------------------- */
 WibTimestamp_t inline Packets::bof (WibTimestamp_t timestamp)
 {
    #pragma HLS INLINE
    return timestamp & (~PACKET_M_NSAMPLES);
 }
 /* --------------------------------------------------------------------- */



 /* ---------------------------------------------------------------------- *//*!
  *
  *  \brief  Return the timestamp corresponding to the end of
  *          packet time.
  *  \return The end of frame time stamp
  *
  *  \param[in]  timestamp  The timestamp
  *
 \* --------------------------------------------------------------------- */
 WibTimestamp_t inline Packets::eof (WibTimestamp_t timestamp)
 {
    #pragma HLS INLINE
    return timestamp | (PACKET_M_NSAMPLES);
 }
 /* --------------------------------------------------------------------- */



 /* ---------------------------------------------------------------------- *//*!
  *
  * \brief  Extract the packet index from the full circular buffer
  *         index
  * \return The packet index corresponding to the timestamp
  *
  *  \param[in]  timestamp  The timestamp
  *
 \* ---------------------------------------------------------------------- */
 Packets::Pidx_t inline Packets::packet_index (WibTimestamp_t timestamp)
 {
    #pragma HLS INLINE
    return timestamp >> (PACKET_B_NSAMPLES);
 }
 /* ---------------------------------------------------------------------- */



 /* ---------------------------------------------------------------------- *//*!
  *
  * \brief  Extract the packet offset from the timestamp
  * \return The full circular buffer offset
  *
  *  \param[in]  timestamp  The timestamp
  *
 \* ---------------------------------------------------------------------- */
 Packets::Oidx_t inline Packets::packet_offset (WibTimestamp_t timestamp)
 {
    #pragma HLS INLINE

    // The offset is just the low bits of the timestamp, so this
    // extraction vhappens when the timestamp is transformed to the return
    // type of Oidx_t.  So while it looks as nothing is being done,
    // the low bits are correctly extracted.
    return timestamp;
 }
 /* ---------------------------------------------------------------------- */


 /* ---------------------------------------------------------------------- *//*!
  *
  *   \brief  Test if this timestamp is associated with the first frame of
  *           a packet.
  *   \return A boolean indicating whether this timestamp is associated
  *           with the first frame
  *
  *   \param[in] timestamp The timestamp to test
 \* ---------------------------------------------------------------------- */
 bool inline Packets::is_first (WibTimestamp_t timestamp)
 {
    bool   flag = (timestamp & PACKET_M_NSAMPLES) == 0;
    return flag;
 }
 /* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Test if this timestamp is associated with the last frame of
 *           a packet.
 *   \return A boolean indicating whether this timestamp is associated
 *           with the last frame
 *
 *   \param[in] timestamp The timestamp to test
\* ---------------------------------------------------------------------- */
bool inline Packets::is_last (WibTimestamp_t timestamp)
{
   bool   flag  = (timestamp & PACKET_M_NSAMPLES) == (PACKET_K_NSAMPLES - 1);
   return flag;
}
/* ---------------------------------------------------------------------- */

#endif

