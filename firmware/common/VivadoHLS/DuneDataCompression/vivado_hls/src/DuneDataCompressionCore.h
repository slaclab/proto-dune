// -*-Mode: C++;-*-

#ifndef _DUNE_DATA_COMPRESSION_CORE_H_
#define _DUNE_DATA_COMPRESSION_CORE_H_

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     DuneDataCompressionCore.h
 *  @brief    Interface file for the DUNE compresssion
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
 *  2016.06.1
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

#include "DuneDataCompressionTypes.h"


void DuneDataCompressionCore(AxisIn                        &sAxis,
                             AxisOut                       &mAxis,
                             ModuleIdx_t                moduleIdx,
                             ModuleConfig                  config,
                             MonitorModule               &monitor);
#endif

