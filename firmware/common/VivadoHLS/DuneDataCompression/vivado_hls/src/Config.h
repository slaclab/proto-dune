// -*-Mode: C++;-*-

#ifndef _DUNE_DATA_COMPRESSION_CONFIG_H_
#define _DUNE_DATA_COMPRESSION_CONFIG_H_


/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     Config.h
 *  @brief    Defines static configuration parameters
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


#include "Parameters.h"
#include "ap_int.h"


/* ---------------------------------------------------------------------- *//*!
 *
 *   \enum  Mode_t
 *   \brief Enumeration of the data processing modes
 *
\* ---------------------------------------------------------------------- */
enum MODE_K
{
   MODE_K_DUMP       = 0,  /*!< Read and dispose of the data              */
   MODE_K_COPY       = 1,  /*!< Read and promote as is                    */
   MODE_K_TRANSPOSE  = 2,  /*!< Transpose channel and time order          */
   MODE_K_COMPRESS   = 3,  /*!< Compress the data                         */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \typedef Mode_t
 *  \brief   The data processing mode
 *
\* ---------------------------------------------------------------------- */
typedef ap_uint<2> Mode_t;
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct ChannelConfig
 *  \brief  The per channel configuration information
 *
\* ---------------------------------------------------------------------- */
struct ChannelConfig {
   bool disabled;        /*!< Is this channel disabled                    */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *  \struct ConfigModule
 *  \brief  The per module configuation information
 *
\* ---------------------------------------------------------------------- */
class ModuleConfig 
{
public:
   ModuleConfig () { return; }
   void copy (ModuleConfig const &src); 
   
public:   
    int32_t                            init;  /*!< Initialization flag    */
   uint32_t                            mode;  /*!< Data processing mode   */
   uint32_t                           limit;  /*!< Limits size of output
                                                   packet, units = 64bit
                                                   words                  */
   ChannelConfig    chns[MODULE_K_NCHANNELS]; /*!< Per channel config     */
};
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!
 *
 *   \brief  Make a copy of the configuraion block
 *
 *   \param[ in]  src The configuration block to copy
 *
\* ---------------------------------------------------------------------- */
inline void ModuleConfig::copy (ModuleConfig const &src)
{
    *this = src;
    return;
}
/* ---------------------------------------------------------------------- */



#endif
