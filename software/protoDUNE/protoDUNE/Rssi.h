#ifndef _RSSI_H_
#define _RSSI_H_


// ----------------------------------------------------------------------
// 
// HISTORY
// 
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2018.05.08 jjr Created
//                Modify TimingMsg to match the V4 firmware
//
// ----------------------------------------------------------------------


#include <DaqBuffer.h>
#include <AxisDriver.h>
#include <poll.h>
#include <cinttypes>



/* ====================================================================== */
/* FORWARD REFERENCES                                                     */
/* ---------------------------------------------------------------------- */
class RssiIovec;
/* ====================================================================== */




/* ====================================================================== */
/* LOCAL PROTOTYPES                                                       */
/* ---------------------------------------------------------------------- */
static inline constexpr uint32_t setFlags (uint32_t fuser, 
                                           uint32_t luser, 
                                           uint32_t  cont);

/* ====================================================================== */




/* ---------------------------------------------------------------------- *//*!
   
   \brief  Constructs the AxisFlags
   \return The costructed flags
   
   \param[in] fuser  The first flag
   \param[in] luser  The last  flag
   \param[in]  cont  The continuation flag
   
   \warning
   This is a kludge.  This should be done with axisSetFlags, but could
   not get the routine changed to a constexpr to satistfy the needs of 
   initializing the flag enumeration
                                                                          */
/* ---------------------------------------------------------------------- */
constexpr static uint32_t setFlags (uint32_t fuser, 
                                    uint32_t luser, 
                                    uint32_t  cont)
{
   return  ((cont  &  0x1) << 16) |
      ((luser & 0xFF) <<  8) |
      ((fuser & 0xFF) <<  0);
}
/* ---------------------------------------------------------------------- */





/* ====================================================================== */
/* BEGIN:DEFINITIONS                                                      */
/* ---------------------------------------------------------------------- *//*!

  \class  RssiHdr
  \brief  Provides functionality for transmitting RSSI buffers similar to
          msghdr for sendmsg
                                                                          */
/* ---------------------------------------------------------------------- */
class RssiHdr
{
public:
   explicit RssiHdr () { return; }

public:
   size_t send (int fd) const;

public:
   int32_t   rssi_iovlen;  /*!< The count of Rssi iovecs                  */
   RssiIovec   *rssi_iov;  /*!< The array of Rssi iovecs                  */
};
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \class RssiIovec
  \brief  Provides functionality for transmitting RSSI buffers similar to
          iovec for sendmsg

  \par
   The field member effectively define the paramters to the two RSSI
   DMA methods.  A simple enumeration steers which method to use.
                                                                          */
/* ---------------------------------------------------------------------- */
class RssiIovec
{
public:
   explicit RssiIovec () { return; }

public:
   enum Flags
   {
      Only   = setFlags (2, 0, 0),   /*!< Only buffer, SOF only       */
      First  = setFlags (2, 0, 1),   /*!< First in a sequence         */
      Middle = setFlags (0, 0, 1),   /*!< Middle of a sequence        */
      Last   = setFlags (0, 0, 0)    /*!< Last in a sequence          */
   };


public:
   void  construct (void   *base, size_t len, uint32_t flags, uint32_t tdest);
   void  construct (uint32_t idx, size_t len, uint32_t flags, uint32_t tdest);

   void  construct (void   *base, size_t len, uint32_t flags);
   void  construct (uint32_t idx, size_t len, uint32_t flags);

   void   increase (size_t len);
   void    setLast ();
   size_t     send (int32_t fd) const;

private:
   void   print    (size_t ret) const;

private:
   /* ------------------------------------------------------------------- *//*!

     \enum  Method
     \brief Enumerates the RSSI DMA methods
                                                                          */
   /* ------------------------------------------------------------------- */
   enum class Method
   {
      Memory = 0,        /*!< Dma using the direct memory method          */
      Index  = 1         /*!< Dma via the index method                    */
   };
   /* ------------------------------------------------------------------- */


public:
   void     *iov_base;  /*!< The address of the buffer, valid if Memory   */
   uint32_t   iov_idx;  /*!< The index   of the buffer, valid if Index    */
   size_t     iov_len;  /*!< The number of bytes to transfer              */
   Method  iov_method;  /*!< Selects which method                         */
   uint32_t iov_flags;  /*!< The DMA flags                                */
   uint32_t iov_tdest;  /*!< The DMA destination channel                  */
};
/* ---------------------------------------------------------------------- */
/* END: DEFINITIONS                                                       */
/* ====================================================================== */






/* ====================================================================== */
/* BEGIN: IMPLEMENTATION                                                  */
/* ---------------------------------------------------------------------- *//*!

  \brief  Send the buffers defined by the RssiHdr to the output DMA
          methods
  \return The number of bytes transmitted or -1 if in error

  \param[in]   fd  The output file descriptor
                                                                          */
/* ---------------------------------------------------------------------- */
inline size_t RssiHdr::send (int32_t fd) const
{
   int32_t nbytes = 0;


   //uint32_t flags = axisSetFlags(2,0,0);// axisSetFlags(fuser=2(SOF),luser=0,cont=0)
   

   RssiIovec const *rssi_iov = this->rssi_iov;
   size_t        rssi_iovlen = this->rssi_iovlen;

   for (size_t idx = 0; idx < rssi_iovlen; idx++) 
   {
      size_t ret = rssi_iov[idx].send (fd);

      if (ret < 0)
      {
         return ret;
      }

      nbytes += ret;
   }

   return nbytes;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Populates one Rssi iovec with the information needed to send the
         data via the direct memory method that defaults the channel to 0

  \param[in]  base  The base address of the buffer to be sent
  \param[in]   len  The number of bytes in the buffer to be sent
  \param[in] flags  The Axis flags.  This must contain an SOF for the first 
                    buffer and a continuation flags for all but the last
                    buffer.
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::construct (void *base, size_t len, uint32_t flags)
{
   construct (base, len, flags, 0);
   return;

}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Populates one Rssi iovec with the information needed to send the
         data via the frame index method

  \param[in]   idx  The frame index of the buffer to be sent
  \param[in]   len  The number of bytes in the buffer to be sent
  \param[in] flags  The Axis flags.  This must contain an SOF for the first 
                    buffer and a continuation flags for all but the last buffer
  \param[in] tdest  The destination channel
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::construct (uint32_t idx, size_t len, uint32_t flags)
{
   construct (idx, len, flags, 0);
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Populates one Rssi iovec with the information needed to send the
         data via the direct memory method

  \param[in]  base  The base address of the buffer to be sent
  \param[in]   len  The number of bytes in the buffer to be sent
  \param[in] flags  The Axis flags.  This must contain an SOF for the first 
                    buffer and a continuation flags for all but the last buffer
  \param[in] tdest  The destination channel
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::construct (void     *base, 
                                  size_t     len,
                                  uint32_t flags,
                                  uint32_t tdest)
{
   iov_base   = base;
   iov_len    =  len;
   iov_method = RssiIovec::Method::Memory;
   iov_flags  = flags;
   iov_tdest  = tdest;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Populates one Rssi iovec with the information needed to send the
         data via the frame index method

  \param[in]   idx  The frame index of the buffer to be sent
  \param[in]   len  The number of bytes in the buffer to be sent
  \param[in] flags  The Axis flags.  This must contain an SOF for the first 
                    buffer and a continuation flags for all but the last buffer
  \param[in] tdest  The destination channel
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::construct (uint32_t   idx, 
                                  size_t     len,
                                  uint32_t flags, 
                                  uint32_t tdest)
{
   iov_idx    =  idx;
   iov_len    =  len;
   iov_method = RssiIovec::Method::Index;
   iov_flags  = flags;
   iov_tdest  = tdest;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Increase the size of the transfer by \a len bytes

  \param[in] len The number of bytes to add onto the transfer.  Note this
                 may also be negative, so it can also decrease the 
                 transfer size.
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::increase (size_t len)
{
   this->iov_len += len;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief Sets the flags to indicate that this is the last iovec in a
          sequence.
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::setLast ()
{
   this->iov_flags = RssiIovec::Last;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  DMA the data described by this RSSI iovec
  \return The number of bytes transmitted or -1 if in error

  \param[in] rssi  The Rssi header specifying the data to be sent
                                                                          */
/* ---------------------------------------------------------------------- */
inline size_t RssiIovec::send (int32_t fd) const
{
   enum RssiIovec::Method method = iov_method;
   auto                      len = iov_len;
   auto                    flags = iov_flags;
   auto                    tdest = iov_tdest;

   
   // Dispatch to the proper transfer method
   if (method == RssiIovec::Method::Memory)
   {
      uint8_t *base = reinterpret_cast<uint8_t *>(iov_base);
      size_t    ret = dmaWrite (fd, base, len, flags, tdest);

      // If no write buffer was available, wait for one and retry
      if (ret == 0)
      {
         DaqDmaDevice::wait (fd);
         ret = dmaWrite (fd, base, len, flags, tdest);
      }
      
      // Diagnostic only, generally neutered
      print (ret);

      // Check for success
      if (ret != len)
      {
         fprintf (stderr,"RSSI Write error (memory method)! %8.8zx != %8.8zx\n",
                  ret, len);
      }

      return ret;
   }

   else if (method == RssiIovec::Method::Index)
   {
      uint32_t idx = this->iov_idx;
      size_t   ret = dmaWriteIndex (fd, idx, len, flags, tdest);


      // Diagnostic only, generally neutered
      print (ret);

      if (ret != len) 
      {
         fprintf (stderr,"RSSI Write error (index  method)! %8.8zx != %8.8zx\n",
                  ret, len);
      }

      return ret;
   }

   return 0;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief Diagnostic print out context of one sent RSSI iovec

   \param[in] ret The returned value from the dmaWrite method
                                                                          */
/* ---------------------------------------------------------------------- */
inline void RssiIovec::print (size_t ret) const
{
   //return;

   if (iov_method == RssiIovec::Method::Memory)
   {
      printf ("Rssi Memory, %8.8zx %10p %8.8x %8.8x %8.8x\n", 
               ret, (void *)iov_base, iov_len, iov_flags, iov_tdest);
   }
   else if (iov_method == RssiIovec::Method::Index)
   {
      
      printf ("Rssi  Index, %8.8zx   %8.8x %8.8x %8.8x %8.8x\n", 
             ret, iov_idx, iov_len, iov_flags, iov_tdest);
   }
   else
   {
      printf ("Rssi Unknown method %d\n", static_cast<int>(iov_method));
   }
      

   return;
}
/* ---------------------------------------------------------------------- */
/* END: IMPLEMENTATION                                                    */
/* ====================================================================== */


#endif
