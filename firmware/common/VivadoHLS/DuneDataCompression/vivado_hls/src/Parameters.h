// -*-Mode: C++;-*-

#ifndef _DUNE_DATA_COMPRESSION_PARAMETERS_H_
#define _DUNE_DATA_COMPRESSION_PARAMETERS_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     Parameters.h
 *  @brief    Defines the basic parameters, things like the number of time
 *            samples in a packet, number of channels in a WIB frame, etc.
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

#include "WibFrame.h"

/* ---------------------------------------------------------------------- *//*!
 *
 * \def    MODULE_B_NCHANNELS
 * \brief  The number of channels in a module expressed as a bit count.
 *         This is typically 5, giving 32 channels in a module, but for
 *         testing purposes may be can any number resulting in a multiple
 *         of 4 channels.
 *
 * \def    MODULE_K_NCHANNELS
 * \brief  The number of channels in a module as a integer count. This is
 *         typically 32, but, for testing can be any multiple 4. This
 *         must be consistent with MODULE_B_NCHANNELS, but is expressed
 *         literally for readability.
 *
 * \def    RCE_B_NCHANNELS
 * \brief  The number of channels serviced by an RCE expressed as a bit
 *         count.
 * \par
 *         Since the number of channels serviced by an RCE is only constrained
 *         to be a multiple of 32, this value must berounded up.  For example
 *         if an RCE service 384 channels, then this value must be set to 9.
 *
 * \def    RCE_K_NCHANNELS
 * \brief  The total number of channels processed in one RCE. This value
 *         is typically a multiple of 32.
 *
 * \def    RCE_B_NMODULES
 * \brief  The number of modules serviced by a module expressed as a bit count.
 *
 * \note
 *         If the number of modules is not a power of 2, then this value must
 *         be rounded up.
 *
 * \def    RCE_K_NMODULES
 * \brief  The number of modules in one RCE. This is formally
 *         RCE_K_NCHANNELS / MODULE_K_NCHANNELS. With 256 channels and
 *         32 channels per module, this gives 8 modules.
 *
\* ---------------------------------------------------------------------- */
#define MODULE_B_NCHANNELS             7
#define MODULE_K_NCHANNELS           128//// !!!KLUDGE 32

#define RCE_B_NCHANNELS                8
#define RCE_K_NCHANNELS              256

#define RCE_B_NMODULES              (RCE_B_NCHANNELS - MODULE_B_NCHANNELS)
#define RCE_K_NMODULES              (RCE_K_NCHANNELS / MODULE_K_NCHANNELS)
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \def    PACKET_B_NSAMPLES
 *  \brief  The number of ADCs per packet, expressed in bits
 *
 *  \def    PACKET_K_NSAMPLES
 *  \brief  The number of ADCs per packet
 *
 *  \def   PACKET_M_NSAMPLES
 *  \brief The number of ADCs per packet, expressed as a bit mask
 *
\* ---------------------------------------------------------------------- */
#define PACKET_B_NSAMPLES   10
#define PACKET_K_NSAMPLES 1024
#define PACKET_M_NSAMPLES (PACKET_K_NSAMPLES - 1)
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \def    MODULE_K_MAXSIZE_IB
 *  \brief  Maximum size of an inbound frame
 *
 *  \def    MODULE_K_MAXSIZE_OB
 *  \brief  Maximum size of an outbound packet
 *
\* ---------------------------------------------------------------------- */
#define MODULE_K_MAXSIZE_IB  (sizeof (WibFrame) / sizeof (uint64_t))
#define MODULE_K_MAXSIZE_OB  (MODULE_K_NCHANNELS * PACKET_K_NSAMPLES/4 + 100)
/* ---------------------------------------------------------------------- */

#endif