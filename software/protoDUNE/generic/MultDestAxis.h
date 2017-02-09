//-----------------------------------------------------------------------------
// File          : MultDestAxis.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// AXIS Destination container for MultLink class.
// 
// Destination Field usage:
// bits 7:0 = Index (ignored)
// bits 15:8 = AXIS dest field for register transactions
// bits 23:16 = AXIS dest field for commands
// bits 31:24 = AXIS dest field for data
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 06/18/2014: created
//-----------------------------------------------------------------------------
#ifndef __MULT_DEST_AXIS_H__
#define __MULT_DEST_AXIS_H__
#include <MultDest.h>
#include <string>
#include <stdint.h>
using namespace std;

//! Class to contain PGP communications link
class MultDestAxis : public MultDest {

      string path_;

   public:

      //! Constructor
      MultDestAxis (string path);

      //! Deconstructor
      ~MultDestAxis ( );

      // The remaining functions are called only from the MultLink class.

      //! Open link
      void open ( uint32_t idx, uint32_t maxRxTx );

      //! Transmit data.
      /*! 
       * \param type  Type enum
       * \param ptr   Pointer to data of type
       * \param size  The size of the data pointed at
       * Returns the amount of data transmitted in bytes, or -1 on error
      */
      int32_t transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config );

      // Receive data
      /*! 
       * \param type  Pointer to type, updated with type received
       * \param ptr   Pointer to update with received data
       * Returns the amount of data received in bytes, or -1 on error
      */
      int32_t receive ( MultType *type, void **ptr, uint32_t *context);

};
#endif
