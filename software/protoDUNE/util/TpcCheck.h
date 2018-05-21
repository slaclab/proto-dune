#ifndef _TPC_CHECK_H_
#define _TPC_CHECK_H_

#include <inttypes.h>
#include <stdio.h>

struct _Ctx;


static inline unsigned int checkHeader (struct _Ctx    *ctx,
                                        uint8_t const *data,
                                        int           ndata);

static inline unsigned int checkData (struct _Ctx      *ctx,
                                      uint8_t  const *bytes,
                                      int            nbytes);



/* ---------------------------------------------------------------------- *//*!

  \brief  Checks the integrity of a header packet
  \retval == 0, all is okay
  \retval != 0, a bit map of the errors

  \param[in]   ctx  The check context
  \param[in]  data  The data to check
  \param[in] ndata  The number of bytes in the header
                                                                          */
/* ---------------------------------------------------------------------- */
static inline unsigned int checkHeader (struct _Ctx    *ctx,
                                        uint8_t const *data,
                                        int           ndata)
{
   unsigned int reason = 0;

   //uint64_t header = get_w64 (data);


   return reason;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief  Checks the integrity of a data packet
  \retval == 0, all is okay
  \retval != 0, a bit map of the errors

  \param[in]   ctx  The check context
  \param[in]  data  The data to check
  \param[in] ndata  The number of bytes in the data
                                                                          */
/* ---------------------------------------------------------------------- */
static inline unsigned int checkData (struct _Ctx      *ctx,
                                      uint8_t  const *bytes,
                                      int            nbytes)
{
#  define N64_PER_FRAME 30
#  define NBYTES (unsigned int)(((1 + N64_PER_FRAME * 1024) + 1) * sizeof (uint64_t))


   struct History_s
   {
      uint64_t  timestamp;
      uint16_t convert[2];
   };

   static struct History_s History[2] = { {0, {0, 0}}, {0, {0,0}} };
   static unsigned int        Counter = 0;
   uint64_t const               *data = (uint64_t const *)bytes;
   int                            n64 = nbytes / sizeof (*data) - 1;
   int                           dest = 0;  


   // ------------------------------------
   // Check for the correct amount of data
   // ------------------------------------
   if (nbytes != NBYTES)
   {
      printf ("Aborting @ %2u.%6u %u != %u incorrect amount of data received\n",
              dest, Counter, nbytes, NBYTES);
      return 1;
   }

   // SKip FPGA header
   data += 1;
   n64  -= 1;

   // -------------------------------------------------------------
   // Seed predicted sequence number with either
   //   1) The GPS timestamp of the previous packet
   //   2) The GPS timestamp of the first frame
   // -------------------------------------------------------------
   uint16_t   predicted_cvt_0;
   uint16_t   predicted_cvt_1;
   uint64_t   predicted_ts = History[dest].timestamp;
   if (predicted_ts)
   {
      predicted_ts    = data[ 1];
      predicted_cvt_0 = data[ 2] >> 48;
      predicted_cvt_1 = data[16] >> 48;
   }
   else
   {
      predicted_cvt_0 = History[dest].convert[0];
      predicted_cvt_1 = History[dest].convert[1];
   }


   // --------------------
   // Loop over each frame
   // --------------------
   unsigned int    frame = 0;
   for (int idx = 0; idx < n64; idx += N64_PER_FRAME)
   {
      uint64_t d = data[idx];

      // -------------------------
      // Check the comma character
      // -------------------------
      if ((d & 0xff) != 0xbc)
      {
         printf ("Error frame @ %2u.%6u.%4u: %16.16" PRIx64 "\n",
                 dest, Counter, frame, d);
      }

      // -----------------------------------
      // Form and check the sequence counter
      // ----------------------------------
      uint64_t timestamp = data[idx+1];
      uint16_t     cvt_0 = data[idx +  2] >> 48;
      uint16_t     cvt_1 = data[idx + 16] >> 48;

      int error = ((timestamp != predicted_ts   ) << 0)
                | ((cvt_0     != predicted_cvt_0) << 1)
                | ((cvt_1     != predicted_cvt_1) << 2);

      if (error)
      {
         printf ("Error  seq @ %2u.%6u.%4u: "
                 "ts: %16.16" PRIx64 " %c= %16.16" PRIx64 " "
                 "cvt: %4.4" PRIx16 " %c= %4.4" PRIx16 " "
                 "cvt: %4.4" PRIx16 " %c= %4.4" PRIx16 "\n",
                 dest, Counter, frame, 
                 timestamp, (error&1) ? '!' : '=', predicted_ts,
                 cvt_0    , (error&2) ? '!' : '=', predicted_cvt_0,
                 cvt_1    , (error&4) ? '!' : '=', predicted_cvt_1);

         // -----------------------------------------------------
         // In case of error, resynch the predicted to the actual
         // -----------------------------------------------------
         predicted_ts    = timestamp;
         predicted_cvt_0 = cvt_0;
         predicted_cvt_1 = cvt_1;
      }
      else if (1 && ((Counter % (1024)) == 0) && ((frame % 256) == 0))
      {
         // --------------------------------------
         // Print reassuring message at about 2 Hz
         // --------------------------------------
         printf ("Spot check @ %2u.%6u.%4u: "
                 "ts: %16.16" PRIx64 " == %16.16" PRIx64 " "
                 "cvt: %4.4" PRIx16 " == %4.4" PRIx16 " "
                 "cvt: %4.4" PRIx16 " == %4.4" PRIx16 "\n",
                 dest, Counter, frame, 
                 timestamp, predicted_ts,
                 cvt_0    , predicted_cvt_0,
                 cvt_1    , predicted_cvt_1);
      }


      // -------------------------------------
      // Advance the predicted sequence number
      // Advance the frame counter
      // -------------------------------------
      predicted_ts     = timestamp + 500;
      predicted_cvt_0 += 1;
      predicted_cvt_1 += 1;
      frame           += 1;
   }

   // -----------------------------------------------
   // Keep track of the number of time called and
   // the expected sequence number of the next packet
   // for this destination.
   // -----------------------------------------------
   Counter                 += 1;
   History[dest].timestamp  = predicted_ts    +  500;
   History[dest].convert[0] = predicted_cvt_0 +    1;
   History[dest].convert[1] = predicted_cvt_1 +    1;

   return 0;
}
/* ---------------------------------------------------------------------- */

#endif
