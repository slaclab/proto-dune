//-----------------------------------------------------------------------------
// File          : Data.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic data container
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
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __DATA_H__
#define __DATA_H__

#include <string>
#include <stdint.h>

using namespace std;

#ifdef __CINT__
#define uint32_t unsigned int
#endif

//! Class to contain generic register data.
class Data {

      // Allocation
      uint32_t alloc_;

   protected:

      // Data container
      uint32_t *data_;

      // Size value
      uint32_t size_;

      // Update frame state
      virtual void update();

   public:

      // Data types. 
      // Count is n*32bits for type = 0, byte count for all others
      enum DataType {
         RawData     = 0,
         XmlConfig   = 1,
         XmlStatus   = 2,
         XmlRunStart = 3,
         XmlRunStop  = 4,
         XmlRunTime  = 5
      };

      //! Constructor
      /*! 
       * \param data Data pointer
       * \param size Data size
      */
      Data ( uint32_t *data, uint32_t size );

      //! Constructor
      Data ();

      //! Deconstructor
      virtual ~Data ();

      //! Read data from file descriptor
      /*! 
       * \param fd File descriptor
       * \param size Data size
      */
      bool read ( int32_t fd, uint32_t size );

      //! Copy data from buffer
      /*! 
       * \param data Data pointer
       * \param size Data size
      */
      void copy ( uint32_t *data, uint32_t size );

      //! Get pointer to data buffer
      uint32_t *data ( );

      //! Get data size
      uint32_t size ( );

};
#endif
