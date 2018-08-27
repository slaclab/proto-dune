// -*-Mode: C++;-*-

#ifndef _DUNE_DATA_COMPRESSION_AXIS_H_
#define _DUNE_DATA_COMPRESSION_AXIS_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     Axis.h
 *  @brief    Defines the AXI input and output streams
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


#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <stdint.h>


// --------------------------------------------------------------------
// Configure the AXIS bus to be the same as 
// RCEG3_AXIS_DMA_CONFIG_C (RceG3Pkg.vhd)
//   constant RCEG3_AXIS_DMA_CONFIG_C : AxiStreamConfigType := (
//      TSTRB_EN_C    => false,
//      TDATA_BYTES_C => 8,
//      TDEST_BITS_C  => 4,
//      TID_BITS_C    => 0,
//      TKEEP_MODE_C  => TKEEP_COMP_C,
//      TUSER_BITS_C  => 4,
//      TUSER_MODE_C  => TUSER_FIRST_LAST_C);
// --------------------------------------------------------------------


///////////////////////////////////////////
//       AXIS_STREAM Definition          //
///////////////////////////////////////////
typedef     ap_axiu<64,4,1,1>  AxisIn_t;
typedef     ap_axiu<64,4,1,1>  AxisOut_t;
typedef hls::stream<AxisIn_t>  AxisIn;
typedef hls::stream<AxisOut_t> AxisOut;
typedef     ap_uint<64>        AxisWord_t;
// tdata[063:00] = DMA Data
// tKeep[007:00] = DMA Byte Enable
// tStrb[007:00] = Not Used
// tUser[003:03] = Run enable                (first)
// tUser[002:02] = Flush                     (tlast)
// tUser[001:01] = Start of Frame            (first)
// tUser[000:00] = End of Frame with Errors  (tlast)
// tLast[000:00] = End of Frame
//   tId[000:00] = Not Used
// tDest[000:00] = Not Used
// Note:
///////////////////////////////////////////
//   template<int D, int U, int TI, int TD>
//   struct ap_axis{
//      ap_int<D>    data;
//      ap_uint<D/8> keep;
//      ap_uint<D/8> strb;
//      ap_uint<U>   user;
//      ap_uint<1>   last;
//      ap_uint<TI>  id;
//      ap_uint<TD>  dest;
//   };


/* ---------------------------------------------------------------------- *//*!
 *
 * \enum   class AxisUserFirst
 * \brief  Enumeates the meaning of the bits in AxisIn user field when
 *         it is in the first word of the frame.
 *
\* ---------------------------------------------------------------------- */
enum class AxisUserFirst
{
   Sof       = 1,  /*!< Always set on the first word of the frame         */
   RunEnable = 3   /*!< Run enable, effect means keep the data            */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \enum  class AxisUserLast
 * \brief Enumerates the meaning of the bits in AxisIn user field when
 *        it is in the last word the frame.
\* ---------------------------------------------------------------------- */
enum class AxisUserLast
{
   EofErr   = 0,  /*!< End of frame with errors                           */
   Flush    = 2   /*!< Flush the output frame                             */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \enum  class AxisLast
 * \brief Enumerates the meaing of the ibits in Axis last field.
 *
\* ---------------------------------------------------------------------- */
enum class AxisLast
{
   Eof     = 0 /*!< End of frame word                                     */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/* Convenience Methods                                                    */
/* ---------------------------------------------------------------------- */
static void           commit (AxisOut       &axis,
                              int            &odx,
                              bool          write,
                              uint64_t       data,
                              int            user,
                              int            last);

static void           commit (AxisOut       &axis,
                              int            &odx,
                              AxisOut_t      &out);

static void      commit_last (AxisOut      &mAxis,
                              int             odx);
/* ---------------------------------------------------------------------- */


#undef WRITE_DATA_PRINT

/* ---------------------------------------------------------------------- */
#if WRITE_DATA_PRINT
/* ---------------------------------------------------------------------- */
static void print_write_data (int odx, uint64_t data)
{
   std::cout << "Writing[" << std::setw(5) << odx << "] = " << std::setw(16) << std::hex << data << std::endl;
}
/* ---------------------------------------------------------------------- */
#else
/* ---------------------------------------------------------------------- */
#define print_write_data(_odx, _data)
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Commits a fully filled out AxisOut_t word to the output stream
 *
 *  \param[out] axis  The output AXI stream
 *  \param[ in]  odx  The output index (diagnostic only)
 *  \param[ in]  out  The output word
 *
\* ---------------------------------------------------------------------- */
static void commit (AxisOut &axis, int &odx, AxisOut_t &out)
{
   #pragma HLS INLINE
   //#pragma HLS PIPELINE

   print_write_data (odx, out.data);
   axis.write (out);
   odx += 1;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Commits 1 64-bit value to the output stream
 *
 *   \param[   out]  axis  The output AXI stream
 *   \param[in:out]   odx  The output index
 *   \param[    in] write  Flag to do the write (if false, no write)
 *   \param[    in]  data  The 64-bit data to commit
 *   \param[    in[  user  The value of the user field
 *
\* ---------------------------------------------------------------------- */
static inline void commit (AxisOut &axis,
                           int      &odx,
                           bool    write,
                           uint64_t data,
                           int      user,
                           int      last)
{
   #pragma HLS inline
   //#pragma HLS PIPELINE

   if (write)
   {
      AxisOut_t out;
      out.data = data;
      out.keep = 0xFF;
      out.strb = 0x0;
      out.user = user;
      out.last = last;
      out.id   = 0x0;
      out.dest = 0x0;

      print_write_data (odx, data);


      axis.write (out);
      odx         += 1;
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Commits the last word to the output stream
 *
 *  \param[in]  axis The output stream
 *  \param[in]   tag A 32-bit value that is placed in the lower 32-bits
 *                   to the last 64-bit word. This is typically the
 *                   number of valid bits written.
 *
\* ---------------------------------------------------------------------- */
static inline void commit_last (AxisOut &axis,  int mdx)
{
   #pragma HLS inline

   AxisOut_t out;

   out.data = ((uint64_t)0xFFFFFFFF << 32) | mdx;
   out.keep = 0xFF;
   out.strb = 0x0;
   out.user = 0x0;
   out.last = 0x1;
   out.id   = 0x0;
   out.dest = 0x0;

   axis.write (out);
   mdx += 1;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#ifndef __SYNTHESIS__
#define WRITE_DATA_PRINT 1
#endif
/* ---------------------------------------------------------------------- */
#endif
