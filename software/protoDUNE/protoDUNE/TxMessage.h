#ifndef _TXMESSAGE_H_
#define _TXMESSAGE_H_

#include "Rssi.h"
#include <sys/types.h>
#include <cinttypes>



/* ---------------------------------------------------------------------- *//*!

  \brief  Captures the information needed to send the data by either
          sendmsg or RSSI
                                                                          */
/* ---------------------------------------------------------------------- */
template <int NIOVECS>
class TxMessage
{
public:
   TxMessage (void *name);
   TxMessage (void *name, uint32_t tdest);

public:
   void add (int iovidx, void *base,                 size_t len, uint32_t flags);
   void add (int iovidx, void *base, uint32_t index, size_t len, uint32_t flags);

   void addt(int iovidx, void *base,                 size_t len, uint32_t flags,
                                                                 uint32_t tdest);
   void addt(int iovidx, void *base, uint32_t index, size_t len, uint32_t flags,
                                                                 uint32_t tdest);

   void init          ();
   void terminateRssi ();
   void terminateMsg  ();
   void setIovlen     (uint32_t iovlen);
   void setRssiLast   (uint32_t iovlen);
   void increaseLast  (uint32_t nbytes);
   void increase      (uint32_t iovlen, 
                       uint32_t nbytes);

   size_t         getIovlen  () const;
   struct msghdr &getMsgHdr  () const;
   RssiHdr       &getRssiHdr () const;

   size_t sendTcp  (int fd, size_t txSize);
   size_t sendRssi (int fd, size_t txSize);

public:
   size_t      m_iovlen;

   struct msghdr    m_msg;
   struct iovec     m_msg_iov[NIOVECS];

   RssiHdr          m_rssi;
   RssiIovec        m_rssi_iov[NIOVECS];

   uint32_t         m_tdest;
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \class TxMessage
  \brief Encapsulates enough information to send the data either via a
         TCP/IP sendmsg or the RSSI DMA methods
                                                                          */
/* ---------------------------------------------------------------------- */
template<int NIOVECS>
TxMessage   <NIOVECS>::TxMessage (void * name) 
{
   m_iovlen           = 0;

   m_msg.msg_name       = name;
   m_msg.msg_namelen    = sizeof (struct sockaddr_in);
   m_msg.msg_iov        = m_msg_iov;
   m_msg.msg_iovlen     = 0;
   m_msg.msg_control    = NULL;
   m_msg.msg_controllen = 0;
   m_msg.msg_flags      = 0;

   m_rssi.rssi_iovlen   = 0;
   m_rssi.rssi_iov      = m_rssi_iov;

   m_tdest              = 0;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class TxMessage
  \brief Encapsulates enough information to send the data either via a
         TCP/IP sendmsg or the RSSI DMA methods
                                                                          */
/* ---------------------------------------------------------------------- */
template<int NIOVECS>
TxMessage   <NIOVECS>::TxMessage (void * name, uint32_t tdest) 
{
   m_iovlen           = 0;

   m_msg.msg_name       = name;
   m_msg.msg_namelen    = sizeof (struct sockaddr_in);
   m_msg.msg_iov        = m_msg_iov;
   m_msg.msg_iovlen     = 0;
   m_msg.msg_control    = NULL;
   m_msg.msg_controllen = 0;
   m_msg.msg_flags      = 0;

   m_rssi.rssi_iovlen   = 0;
   m_rssi.rssi_iov      = m_rssi_iov;

   m_tdest              = tdest;
   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
template  <int NIOVECS>
void TxMessage<NIOVECS>::add (int iovidx, void *base, size_t len, uint32_t flags)
{
   // ----------------
   // Standard sendmsg
   // ----------------
   m_msg_iov[iovidx].iov_base = base;
   m_msg_iov[iovidx].iov_len  =  len;

   
   // -----------------------------
   // RSSI - send by buffer address
   // -----------------------------
   m_rssi_iov[iovidx].construct (base, len, flags, m_tdest);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
template  <int NIOVECS>
void TxMessage<NIOVECS>::add (int     iovidx, 
                              void     *base, 
                              uint32_t   idx, 
                              size_t     len, 
                              uint32_t flags)
{
   // ----------------
   // Standard sendmsg
   // ----------------
   m_msg_iov[iovidx].iov_base = base;
   m_msg_iov[iovidx].iov_len  =  len;
   
   // --------------------
   // RSSI - send by index
   // --------------------
   m_rssi_iov[iovidx].construct (idx, len, flags, m_tdest);


   return;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- */
template  <int NIOVECS>
void TxMessage<NIOVECS>::addt (int     iovidx, 
                               void     *base, 
                               size_t     len,
                               uint32_t flags,
                               uint32_t tdest)
{
   // ----------------
   // Standard sendmsg
   // ----------------
   m_msg_iov[iovidx].iov_base = base;
   m_msg_iov[iovidx].iov_len  =  len;

   
   // -----------------------------
   // RSSI - send by buffer address
   // -----------------------------
   m_rssi_iov[iovidx].construct (base, len, flags, tdest);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
template  <int NIOVECS>
void TxMessage<NIOVECS>::addt (int     iovidx,
                               void     *base, 
                               uint32_t   idx,
                               size_t     len,
                               uint32_t flags,
                               uint32_t tdest)
{
   // ----------------
   // Standard sendmsg
   // ----------------
   m_msg_iov[iovidx].iov_base = base;
   m_msg_iov[iovidx].iov_len  =  len;
   
   // --------------------
   // RSSI - send by index
   // --------------------
   m_rssi_iov[iovidx].construct (idx, len, flags, tdest);


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Initializes the class for a new set of iovecs
                                                                          */
/* ---------------------------------------------------------------------- */
template         <int NIOVECS>
inline void TxMessage<NIOVECS>::init ()
{
   m_iovlen = 0;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief  Increases the number of bytes in the last transfer
 
   \param[in] nbytes  The number of bytes to increase the transfer by.
                                                                          */
/* ---------------------------------------------------------------------- */
template <int NIOVECS>
inline void TxMessage<NIOVECS>::increaseLast  (uint32_t nbytes)
{
     m_msg_iov[m_iovlen - 1].iov_len += nbytes;
    m_rssi_iov[m_iovlen - 1].increase (nbytes);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief  Increases the number of bytes in the specified transfer
 
   \param[in] nbytes  The number of bytes to increase the transfer by.
                                                                          */
/* ---------------------------------------------------------------------- */
template <int NIOVECS>
inline void TxMessage<NIOVECS>::increase  (uint32_t iovidx, uint32_t nbytes)
{
     m_msg_iov[iovidx].iov_len += nbytes;
    m_rssi_iov[iovidx].increase  (nbytes);
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \brief Terminates this set of rssi iovecs 
                                                                          */
/* ---------------------------------------------------------------------- */
template         <int NIOVECS>
inline void TxMessage<NIOVECS>::terminateRssi ()
{
   m_rssi.rssi_iovlen = m_iovlen;
   setRssiLast (m_iovlen - 1);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Terminates this set of msg iovecs (the standard sendmsg ones)
                                                                          */
/* ---------------------------------------------------------------------- */
template         <int NIOVECS>
inline void TxMessage<NIOVECS>::terminateMsg  ()
{
   m_msg.msg_iovlen = m_iovlen;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Sets the number of iovecs

   \param[in] iovlen  The number of iovecs
                                                                          */
/* ---------------------------------------------------------------------- */
template         <int NIOVECS>
inline void TxMessage<NIOVECS>::setIovlen (uint32_t iovlen)
{
   m_iovlen = iovlen;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief Sets the specified rssi iovec to be the last one

   \param[in] iovlen  The Rssi iovec to set
                                                                          */
/* ---------------------------------------------------------------------- */
template         <int NIOVECS>
inline void TxMessage<NIOVECS>::setRssiLast (uint32_t iovlen)
{
   m_rssi.rssi_iov[iovlen].setLast ();
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Returns the count of iovecs
  \return The count of iovecs
                                                                          */
/* ---------------------------------------------------------------------- */
template           <int NIOVECS>
inline size_t TxMessage<NIOVECS>::getIovlen  () const
{
   return m_iovlen;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Returns a reference to the standard sendmsg msghdr
  \return A pointer to the standard sendmsg msghdr
                                                                          */
/* ---------------------------------------------------------------------- */
template<int NIOVECS>
inline struct msghdr &TxMessage<NIOVECS>::getMsgHdr () const
{
   return m_msg;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Returns a reference to the RSSI message header
  \return A pointer to the RSSI message header
                                                                          */
/* ---------------------------------------------------------------------- */
template<int NIOVECS>
inline RssiHdr &TxMessage<NIOVECS>::getRssiHdr () const
{
   return m_rssi;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief Send via TCP

   \param[in] fd      The file descriptor to send on
   \param[in] txSize  The expected size
                                                                          */
/* ---------------------------------------------------------------------- */
template<int NIOVECS>
inline size_t TxMessage<NIOVECS>::sendTcp (int fd, size_t txSize)
{
   // --------------------------------
   // Prepares TCP msghdr for sending
   // (basically sets iovlen

   terminateMsg ();
   size_t  size = sendmsg (fd, &m_msg, 0);
   
   
   if (size != txSize)
   {
      /// Error
   }

   return size;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief  Primitive hex dump routine

  \param[in]   d Pointer to the data to be dumped
  \param[in]   n The number of 64-bit words to dump
                                                                          */
/* ---------------------------------------------------------------------- */
static void dump (uint64_t const *d, int n)
{
   for (int idx = 0; idx < n; idx++)
   {
      if ( (idx & 0x3) == 0) printf ("%2x:", idx);
      
      printf (" %16.16" PRIx64 ,  d[idx]);

      if ((idx & 0x3) == 3) putchar ('\n');
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief  Convenience method to send the current stuff via RSSI

   \param[in] fd      The file descriptor to send on
   \param[in] txSize  The expected size
                                                                          */
/* ---------------------------------------------------------------------- */
template<int NIOVECS>
inline size_t TxMessage<NIOVECS>::sendRssi (int fd, size_t txSize)
{

   // Mark the final iovec as the last one
   terminateRssi ();
   size_t  size  = m_rssi.send (fd);

   dump ((uint64_t const *)m_msg.msg_iov[0].iov_base, 8);

   //printf ("RSSI %8.8zx : %8.8zx\n", size, txSize);

   if (size != txSize)
   {
      size_t chkSize = 0;
      for (size_t idx = 0; idx < m_iovlen; idx++)
      {
         chkSize += m_rssi.rssi_iov[idx].iov_len;
      }

      // Error
      fprintf (stderr, "Error: Rssi transport failed %8.8zx:%8.8zx:%8.8x\n", 
               size, txSize, chkSize);
   }
   
   return size;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
#if 0

//  Used for tracking down the TCP/IP error when sendmsg returns the value 1
//  Protocol specific transmission routines
static inline bool transmitTcpIp (int            fd, 
                                  const msghdr *msg,
                                  int32_t    txSize);


/* ---------------------------------------------------------------------- */
static inline bool transmitTcpIp (int fd, const msghdr *msg, int32_t txSize)
{
   ssize_t ret = sendmsg(fd, msg, 0);

   if (ret != txSize)
   {
      /// Error:: Retry 
      fprintf (stderr, 
               "sendmsg: %zu errno: %d\n"
               "Header; %8.8" PRIx32 "\n"
               "msg_iovlen = %2zu\n"
               "0. iov_base: %p  iov_len: %zu\n"
               "1. iov_base: %p  iov_len: %zu\n",
               ret, errno,
               txSize, 
               msg->msg_iovlen,
               msg->msg_iov[0].iov_base, msg->msg_iov[0].iov_len, 
               msg->msg_iov[1].iov_base, msg->msg_iov[1].iov_len);

      for (unsigned int idx = 0; idx < msg->msg_iovlen; idx++)
      {
         unsigned int len = msg->msg_iov[idx].iov_len;
         uint8_t     *ptr = (uint8_t *)msg->msg_iov[idx].iov_base;
         
                
         AxiBufChecker x;
         int iss = x.check_buffer (ptr, idx, len);
         if (iss)
         {
            fprintf (stderr,
                     "Iov[%2d] %p for %u bytes -> BAD\n",
                     idx, ptr, len);
         }
         else
         {
            fprintf (stderr,
                     "Iov[%2d] %p for %u bytes -> okay\n",
                     idx, ptr, len);
         }
      }      


      fragment_dump (event);
         
      ret = sendmsg (fd, msg, 0);
      if (ret != (int32_t)txSize)
      {
         fprintf (stderr, "Resend failed %d != %d\n", (int)ret, (int)txSize);
      }
      else
      {
         fprintf (stderr, "Resend succeeded %d == %d\n", (int)ret, (int)txSize);
      }
         
      /// --------------------------------
      ///  --- This was just for debugging
      /// --------------------------------
      int disconnect_wait = 10;
      fprintf (stderr,
               "Have error, waiting %d seconds to disconnect\n",
               disconnect_wait);
      ///sleep (disconnect_wait);
      ///disableTx ();
      ///fputs ("Disconnected", stderr);
    
      return false;
   }
   else
   {
      // Successfully sent
      return true;
   }
}
/* ---------------------------------------------------------------------- */
#endif
/* ---------------------------------------------------------------------- */


#endif
