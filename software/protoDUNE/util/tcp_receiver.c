// -*-Mode: C;-*-

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <inttypes.h>
#include <getopt.h>




/* ---------------------------------------------------------------------- *//*!

  \struct _Prms
  \brief  The configuration parameters
                                                                          *//*!
  \typedef Prms
  \brief   Typedef for struct _Prms
                                                                          */
/* ---------------------------------------------------------------------- */
struct _Prms
{
   int portNumber;   /*!< The port to connect on the RCE side            */
   int    rcvSize;   /*!< Size of the receiver buffer in bytes           */
   int       data;   /*!< The number of data words to print              */
   int    nodelay;   /*!< Value of the TCP_NODELAY parameter             */
   int  nfailures;  /*!< Maximum number of failure messages              */
   char   chkData;   /*!< Perform the data check                         */
};
/* ---------------------------------------------------------------------- */
typedef struct _Prms Prms;
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief  Retrieve of set defaults for the governing parameters
                                                                          */
/* ---------------------------------------------------------------------- */
static void getPrms (Prms *prms, int argc, char *const argv[])
{
    int c;
    int  portNumber =       8991;
    int  rcvSize    = 128 * 1024;
    int  data       =          0;
    char chkData    =          0;
    int  nodelay    =          0;
    int  nfailures  =         25;


    while ( (c = getopt (argc, argv, "f:p:r:xd:n:")) != EOF)
    {
       if       (c == 'f') nfailures = strtoul (optarg, NULL, 0);
       else if  (c == 'p') portNumber = strtoul (optarg, NULL, 0);
       else if  (c == 'r') rcvSize    = strtoul (optarg, NULL, 0);
       else if  (c == 'd') data       = strtoul (optarg, NULL, 0);
       else if  (c == 'n') nodelay    = strtoul (optarg, NULL, 0);
       else if  (c == 'x') chkData    = 1;
    }


    prms->portNumber = portNumber;
    prms->rcvSize    = rcvSize;
    prms->data       = data;
    prms->chkData    = chkData;
    prms->nfailures  = nfailures;
    prms->nodelay    = nodelay != 0;

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
typedef struct RcvProfile_s
{
   unsigned int       cnt;  /*!< The number of reads                      */
   unsigned int       tot;  /*!< Total number of bytes read               */
   unsigned int hist[256];  /*!< The size, in bytes of each read          */
}
RcvProfile;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Prints the rcv read history

  \param[in] reads   The read history
  \param[in] title   String to label the history
                                                                          */
/* ---------------------------------------------------------------------- */
static void rcvProfile_print (RcvProfile const *profile, char const *title)
{
   int          idx;
   int          tot = 0;
   unsigned int cnt = profile->cnt;


   /* Limit the number of history items to print to no more than the max */
   if (cnt > sizeof (profile->hist) / sizeof (profile->hist[0]))
   {
      cnt = sizeof (profile->hist) / sizeof (profile->hist[0]);
   }

   
   /* 
    | Calculate the total received, this must match profile->tot 
    | unless there was an error of the total number of reads exceeded
    | the number of history entries/
   */
   for (idx = 0; idx < cnt; idx++) tot += profile->hist[idx];
   printf ("%s  nreads = %u %u/%u\n", title, cnt, tot, profile->tot);
   

   /* Print the read size history */
   int col = 0;
   for (idx = 0;  idx < cnt; idx++)
   {
      col = idx % 10;
      if (col == 0) printf (" %3u:", idx);
      printf (" %6.6x", profile->hist[idx]);
      if (col == 9) putchar ('\n');
   }

   if (col != 9) putchar ('\n');
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */

typedef struct TraceInfo_s
{
   int          err;   /*!< Does this have an error or not                */
   int     nretries;   /*!< The number rcv retries                        */
   uint32_t wrds[4];   /*!< The information                               */
}
TraceInfo;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
typedef struct Trace_s
{
    int              idx;  /*!< Current index       */
    int              edx;  /*!< Last erroring index */
    TraceInfo   info[32];  /*!< Trace information   */
}
Trace;
/* ---------------------------------------------------------------------- */
        


/* ---------------------------------------------------------------------- *//*!

  \brief  Returns the count of elements in the trace buffer
  \return The count of elements in the trace buffer
                                                                          */
/* ---------------------------------------------------------------------- */
static inline int trace_count ()
{
    int cnt = ((sizeof (((Trace *)(0))->info) 
            /   sizeof (((Trace *)(0))->info[0])));

    return cnt;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Increment a trace buffer index
  \return  The incremented trace buffer index

  \param[in] idx  The index to increment
                                                                          */
/* ---------------------------------------------------------------------- */
static inline int trace_increment (int idx)
{
    idx = (idx + 1) & (trace_count () - 1);
    return idx;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Reduce the index, \a idx, to a valid trace index
  \return The reduced index

  \param[in] idx  The index to limit
                                                                          */
/* ---------------------------------------------------------------------- */
static inline int trace_limit (int idx)
{
    idx &= (trace_count () - 1);
    return idx;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief Commit the data to the next available slot in thetrace buffer

  \param[out]       trace  The trace buffer
  \param[ int         err  The error status of the header check
  \param[ in]        data  The received data
  \param[ in]       ndata  The number of bytes to commit
  \param[ in]    nretries  The number of retries on the rcv
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void trace_commit (Trace        *trace, 
                                 int             err,
                                 uint8_t const *data,
                                 int           ndata,
                                 int        nretries)
{
    int idx = trace->idx;


    /* Set the erroring index */
    if (err) 
    {
       trace->edx = idx;
    }

    if (ndata > sizeof (trace->info) / sizeof (trace->info[0]) )
    {
       ndata = sizeof (trace->info) / sizeof (trace->info[0]);
    }


    trace->info[idx].err      = err;
    trace->info[idx].nretries = nretries;
    memcpy (trace->info[idx].wrds, data, ndata);
    trace->idx = trace_increment (idx);
    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Check if the trace buffer is full from the previous error, if 
          any
  \retval True,  if it is full
  \retval False, if it is not yet full

  \param[in]  trace The trace buffer to check
                                                                          */
/* ---------------------------------------------------------------------- */
static inline char trace_full (Trace const *trace)
{
    int edx = trace->edx;
    int idx = trace->idx;


    return (edx >= 0) && (trace_limit (idx - edx) == trace_count ()/2);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Clear the last trace buffer error

  \param[in]  trace  The trace buffer
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void trace_reset (Trace *trace)
{
    trace->edx = -1;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Prints the trace buffer

  \param[in] trace The trace buffer to print
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void trace_print (Trace const *trace, uint32_t const *history)
{
    int cnt = trace_count ();
    int edx = trace->edx;
    int jdx = trace_limit (edx - cnt/2 - 1);

    printf ("Dumping at %d\n", trace->idx);

    int idx;
    for (idx = 0; idx < cnt; idx++, jdx = trace_increment (jdx))
    {
        uint32_t const *wrds = trace->info[jdx].wrds;


        printf ("%2u: %c %8.8x %4d "
                "%8.8" PRIx32 " %8.8" PRIx32 " %8.8" PRIx32 " %8.8" PRIx32,
                jdx,
                jdx == edx ? '*' : ' ',
                trace->info[jdx].err,
                trace->info[jdx].nretries,
                wrds[0], wrds[1], wrds[2], wrds[3]);

        if (jdx == edx)
        {
           int idy;
           uint32_t sidx;
           for (idy = 0; ; idy++)
           {
              sidx = (wrds[idy] >> 8) & 0xff;
              if ( ((wrds[idy] >> 24) & 0xff) == sidx) 
              {
                 printf (" %8.8" PRIx32 "\n", history[sidx]);
                 break;
              }

              if (idx == 6) 
              {
                 putchar ('\n');
                 break;
              }
           }
        }
        else
        {
           putchar ('\n');
        }

    }

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static inline void trace_init (Trace *trace)
{
    trace->idx  = 0;
    trace->edx = -1;
    memset (trace->info, 0, sizeof (trace->info));


    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void trace_test () __attribute ((unused));
/* ---------------------------------------------------------------------- */
static void trace_test ()
{
    Trace trace;
    static uint32_t history[256] = {0};
    int     cnt = trace_count ();

    trace_init (&trace);

    int idx;
    for (idx = 0; idx < cnt + 100;  idx++)
    {
        uint32_t buf[7];
        int         jdx;
        for (jdx = 0; jdx < 7; jdx++) buf[jdx] = idx + jdx;

        if (trace_full (&trace))
        {
           trace_print (&trace, history);
           trace_reset (&trace);
        }


        trace_commit (&trace, 
                      idx == cnt/2, 
                      (uint8_t const *)buf, 
                      sizeof (buf),
                      0);


    }

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \enum  ERR_M
   \brief  Enumerates error bits used when checking the header and data
           for integrity
                                                                          */
/* ---------------------------------------------------------------------- */
enum ERR_M
{
   ERR_M_SEQNUM = (1 << 0),  /*!< Error bit for bad sequence number       */
   ERR_M_DATSIZ = (1 << 1),  /*!< Error bit for bad data size             */
   ERR_M_DATVAL = (1 << 2),  /*!< Error in data value                     */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \struct _Statistics 
  \brief   Keeps track of the statistics
                                                                          *//*!
  \typedef Statistics 
  \brief   Typedef for struct _Statistics
                                                                          */
/* ---------------------------------------------------------------------- */
struct _Statistics 
{
   uint32_t    hdrCnt;  /*!< The number of header packets seen            */
   uint32_t    hdrSiz;  /*!< The number of header bytes   received        */
   uint32_t    datCnt;  /*!< The number of data   packets seen            */
   uint32_t    datSiz;  /*!< The number of data   bytes   received        */
   uint32_t    rcvSiz;  /*!< Number of bytes received                     */
   uint32_t    lstSiz;  /*!< Size, in bytes of the last packet received   */
};
/* ---------------------------------------------------------------------- */
typedef struct _Statistics Statistics;
/* ---------------------------------------------------------------------- */
   


/* ---------------------------------------------------------------------- *//*!

  \struct _Errors 
  \brief   Keeps track of the errors
                                                                          *//*!
  \typedef Errors
  \brief   Typedef for struct _Errors
                                                                          */
/* ---------------------------------------------------------------------- */
struct _Errors 
{
   uint32_t seqNum;  /*!< Number of sequence errors                       */
   uint32_t datSiz;  /*!< Number of bad sizes                             */
   uint32_t datVal;  /*!< Number of times data packet had a bad data value*/
};
/* ---------------------------------------------------------------------- */
typedef struct _Errors Errors;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \struct _Ctx
  \brief   The context used to track the data
                                                                          *//*!
  \typedef Ctx
  \brief   Typedef for struct _Ctx
                                                                          */
/* ---------------------------------------------------------------------- */
struct _Ctx
{
   uint32_t       seqNum;  /*!< The packet sequence number                */
   uint32_t       datMax;  /*!< The maximum size of a data packet         */
   time_t      timestamp;  /*!< The time of the last read                 */
   Statistics      stats;  /*!< The receive statistics                    */
   Errors           errs;  /*!< The error statistics                      */
   int         nfailures;  /*!< Number of failures                        */
   uint32_t history[256];  /*!< History of sequence number by index       */
};
/* ---------------------------------------------------------------------- */
typedef struct _Ctx Ctx;
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \brief Invalidates the checking. This is typically done as part of the
         initialization or after an error.

  \param[in] ctx The check context to reset
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void invalidate_ctx (Ctx *ctx)
{
   ctx->seqNum = 0xffffffff;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Clears the statistics counters

  \param[in] statistics The statistics to clear
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void clear_statistics (Statistics *statistics)
{
   memset (statistics, 0, sizeof (*statistics));
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Clears the error counters

  \param[in] errors The errors to clear
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void clear_errors (Errors *errors)
{
   memset (errors, 0, sizeof (*errors));
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Clears the statistics and error counters

  \param[in] ctx The check context to clear
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void clear_ctx (Ctx *ctx)
{
   clear_statistics (&ctx->stats);
   clear_errors     (&ctx->errs);
   return;
}
/* ---------------------------------------------------------------------- */
           


/* ---------------------------------------------------------------------- *//*!

  \brief Initializes the data tracking context

  \param[in] ctx The context to initialize
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void init_ctx (Ctx *ctx)
{
   ctx->datMax    = 0x4075c;
   ctx->nfailures =       0;
   invalidate_ctx     (ctx);
   clear_ctx          (ctx);
   time   (&ctx->timestamp);

   memset (ctx->history, 0xff, sizeof (ctx->history));
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Extracts one 32-bit word from the \a data stream
  \return The extract 32-bit word

  \param[in] data  The data stream
                                                                          */
/* ---------------------------------------------------------------------- */
static inline uint32_t get_word (uint8_t const *data)
{
   uint32_t w = *(uint32_t const *)data;
   return w;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief   Extracts the 64 time from a time sample
  \return  The 64-bit time (well, technically 56-bits)

  \param[in]  w  The time sample
                                                                          */
/* ---------------------------------------------------------------------- */
static uint64_t get_time (uint32_t const *w)
{
    uint64_t tim = *(uint64_t const *)w;
    return tim;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Checks the integrity of a header packet
  \retval == 0, all is okay
  \retval != 0, a bit map of the errors

  \param[in]   ctx  The check context
  \param[in]  data  The data to check
  \param[in] ndata  The number of bytes in the header
                                                                          */
/* ---------------------------------------------------------------------- */
static inline unsigned int checkHeader (Ctx            *ctx,
                                        uint8_t const *data,
                                        int           ndata)
{
   unsigned int reason = 0;

   uint32_t datSiz = get_word (data + 0);
   uint32_t seqNum = get_word (data + 4);
   
   ctx->stats.hdrCnt += 1;
   ctx->stats.hdrSiz += ndata;


   if ((seqNum != ctx->seqNum + 1) && (ctx->seqNum != 0xffffffff))
   {
      reason           |= ERR_M_SEQNUM;
      ctx->errs.seqNum += 1;
      ctx->seqNum       = 0xffffffff;
   }
   else
   {
      ctx->seqNum = seqNum;
   }

   if (datSiz > ctx->datMax)
   {
      ctx->errs.datSiz += 1;
      reason           |= ERR_M_DATSIZ;
   }

   /*
   if (datSiz != 0x1c)
   {
      uint32_t const *w = (uint32_t const *)data;
      printf ("Header: %8.8"PRIx32" %8.8"PRIx32" %8.8"PRIx32
              " %8.8"PRIx32" %8.8"PRIx32" %8.8"PRIx32" %8.8"PRIx32"\n",
              w[0], w[1], w[2], w[3], w[4], w[5], w[6]);
   }
   */

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
static inline unsigned int checkData (Ctx            *ctx,
                                      uint8_t const *data,
                                      int           ndata)
{
   uint32_t const *w = (uint32_t const *)data;
   int         nwrds = ndata /sizeof (*w);
   uint64_t     xtim = get_time (w);
   int         nerrs = 0;
   int          npkt = 0;
   uint32_t      inc = 0x00020002;
   uint32_t      beg = 0x00020001;

   int seqIdx = (w[2] >> 24) & 0xff;
   ctx->history [seqIdx] = ctx->seqNum;
   uint32_t   tb = ((seqIdx & 0xff) << 24);

   while (nwrds > 0)
    {
       int      cnt;

       uint64_t gtim = get_time (w);
       
       w     += 2;
       nwrds -= 2;
       

       uint32_t xdat = beg | tb; ///// | ((npkt & 0xff) << 8);


       if (xtim != gtim)
       {
          printf ("Error at pkt %4u %16.16"PRIx64" != %16.16"PRIx64"\n",
                  npkt, xtim, gtim);
       }



       if (nwrds < 64)
       {
          printf ("Error at pkt %4u partial record, only %4u adcs\n",
                  npkt, 2*nwrds);
          cnt = (nwrds + 1)/ 2;
       }
       else
       {
          cnt = 128/2;
       }


       int idx;
       for (idx = 0; idx < cnt; idx++)
       {
          uint32_t gdat = *w++;
          if ((gdat ^ xdat) & 0x00ff00ff)
          {
             int      gidx = (gdat >> 24) & 0xff;
             int      xidx = (xdat >> 24) & 0xff;
             uint32_t gseq = ctx->history[gidx];
             uint32_t xseq = ctx->history[xidx];

             if (nerrs == 0) 
             {
                putchar ('\n');
             }

             nerrs += 1;

             printf ("Error at pkt %4u w[%4u] = %8.8"PRIx32" %8.8"PRIx32
                     " %2.2x:%8.8"PRIx32" %2.2x:%8.8"PRIx32"\n",
                     npkt, idx, 
                     gdat, xdat,
                     gidx, gseq, xidx, xseq);
             if (nerrs > 50) break;
          }
          
          xdat += inc;
       }
       
       npkt  += 1;
       nwrds -= idx;
       xtim  += 0x20;
    }


   if (nerrs) 
   {
      uint32_t const *hdr = (uint32_t const *)(data - 28);
      printf ("Hdr:"
              " %8.8"PRIx32" %8.8"PRIx32" %8.8"PRIx32" %8.8"PRIx32
              " %8.8"PRIx32" %8.8"PRIx32" %8.8"PRIx32"\n",
              hdr[0], hdr[1], hdr[2], hdr[3], hdr[4], hdr[5], hdr[6]);

      ctx->errs.datVal += 1;
      return ERR_M_DATVAL;
   }
   else
   {
      return 0;
   }
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief Puts of the statistics/status title line
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void print_statistics_title ()
{
   puts (
   "   Rate        Bps SeqErrs DatErrs BadData Nevents   Total      Bytes\n"
    " ------ ---------- ------- --------------- ------- ------- ----------");

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Prints the periodic statistics

  \param[in] cur  The current statistics
  \param[in] prv  The previous statistics; used to form differences and
                  rates
  \param[in] eol  Either a '\n' or '\r'
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_statistics (Ctx const *cur, Ctx const *prv, char eol)
{
   uint32_t curSiz = cur->stats.hdrSiz + cur->stats.datSiz;
   uint32_t prvSiz = prv->stats.hdrSiz + prv->stats.datSiz;
   
   printf (" %6"PRIu32" %10"PRIu32" %7"PRIu32" %7"PRIu32" %7"PRIu32
           " %7"PRIu32" %7"PRIu32" %10"PRIu32"%c",
           cur->stats.hdrCnt - prv->stats.hdrCnt,
           (curSiz - prvSiz) * 8,
           cur->errs.seqNum,
           cur->errs.datSiz,
           cur->errs.datVal,
           cur->stats.datCnt,
           cur->stats.hdrCnt,
           curSiz,
           eol);

   fflush (stdout);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/* LOCAL PROTOTYPES                                                       */
/* ---------------------------------------------------------------------- */
static int     open_client       (int         portno, 
                                  int        rcvSize, 
                                  int        nodelay, 
                                  int        *servFd);

static int     connect_client    (int       listenFd, 
                                  int         portno, 
                                  int        rcvSize,
                                  int        nodelay);

static int     reconnect_client  (ssize_t      nread,
                                  int       clientFd,
                                  int        serverFd,
                                  Prms const   *prms,
                                  char const    *msg);

static void    print_socketopts  (int             fd);

static void            print_hdr (uint8_t const *data);

static ssize_t read_data        (int              fd,
                                 uint8_t       *data,
                                 int           ndata,
                                 RcvProfile *profile);

static char    if_newline       (char        need_lf);
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \brief  Code to test TCP/IP reception

  \param[in]  argc   Command line argument count
  \param[in]  argv   Vecto of command line parameters
                                                                          */
/* ---------------------------------------------------------------------- */
int main(int argc, char *const argv[])
{
    uint32_t              headerSize = 4 * sizeof (uint32_t);  //bytes
    uint32_t		    maxWords = 100000; ///66002 + 7 - 1;
                                        //in units of 32-bit words
                                       // ....header says the size is 
                                       // 264028 instead of 264036 


    uint8_t  rxData[maxWords*sizeof(uint32_t)]; //byte array
    uint32_t                             eject = 60;
    char                               need_lf =  0;

    Prms      prms;
    getPrms (&prms, argc, argv);

    Trace trace;
    trace_init (&trace);


    bzero (rxData, sizeof (rxData));
    Ctx           ctx;
    Ctx           prv;
    init_ctx   (&ctx);
    init_ctx   (&prv);


    unsigned int retries[128];
    memset (retries, 0, sizeof (retries));


    int         srvFd;
    int         rcvFd  = open_client (prms.portNumber, 
                                      prms.rcvSize, 
                                      prms.nodelay,
                                      &srvFd);

    print_socketopts  (rcvFd);
    print_statistics_title ();

    while (1)
    {
       RcvProfile hdrRcv;
       RcvProfile datRcv;


       /* Check for errors */
       if (trace_full (&trace))
       {
          trace_print (&trace, ctx.history);
          trace_reset (&trace);
       }

       /* 
        | Obtain the header, the hdrRcv keeps track of how many
        | separate rcv calls where made and the number of bytes
        | each read.  One will see a similar thing when reading
        | the data.  Of course there it is more interesting, since
        | the typical header reader will complete in one call to
        | rcv, while the data read will take many, depending on the
        | size of the TCP receive buffer
       */
       ssize_t nread = read_data (rcvFd, rxData, headerSize, &hdrRcv);
   

       /* Check if had error or a disconnect */
       if (nread <= 0)
       {
          need_lf = if_newline (need_lf);
          puts ("Error");
          rcvProfile_print (&hdrRcv, "Hdr");
          rcvProfile_print (&datRcv, "Dat");
          print_statistics (&ctx, &prv, '\n');          
          rcvFd   = reconnect_client (nread, 
                                      rcvFd,
                                      srvFd,
                                      &prms, 
                                      "reading header");
          invalidate_ctx (&ctx);
          continue;
       }
       else if (nread != headerSize)
       {
          printf ("Did not read enough data %zu\n", nread);
          rcvProfile_print (&hdrRcv, "Hdr");
          exit (-1);
       }


       uint32_t failures = checkHeader (&ctx, rxData, headerSize);
       trace_commit (&trace, failures, rxData, headerSize, hdrRcv.cnt);

       if (failures)
       {
          int nfailures = ctx.nfailures++;
          if (nfailures  < prms.nfailures)
          {
             need_lf = if_newline (need_lf);
             printf ("Failure = %8.8"PRIx32"\n", failures);
             rcvProfile_print (&hdrRcv, "Hdr");
             trace_print (&trace, ctx.history);
             trace_reset (&trace);
             print_hdr (rxData);
          }
       }
      
       /* If the header is OK, read the 'data' associated with it */
       if (failures == 0)
       {
          ssize_t  received = nread;
          uint32_t dataSize = get_word (rxData + 0);

          /* Ensure that the buffer can handle the data volume */
          if (dataSize > sizeof (rxData))
          {
             printf ("Packet too large  %u > %zu\n", dataSize, sizeof (rxData));
          }
          
          if (received < dataSize)
          {
             static int Count = 0;
             uint8_t  *data =   rxData + headerSize;
             uint32_t ndata = dataSize - headerSize;
             nread          = read_data (rcvFd, data, ndata, &datRcv);

             if (received != headerSize)
             {
                Count += 1;
             }

             /* 
              | Make a cheap histogram of the number of rcv calls
              | in an attempt to see if this at all correlates 
              | with the failures.
             */
             retries[datRcv.cnt] += 1;

             /* Check if had error or disconnect */
             if (nread <= 0)
             {
                need_lf = if_newline (need_lf);
                printf ("Error reading %u bytes\n", ndata);
                print_hdr (rxData);
                rcvProfile_print (&hdrRcv, "Hdr");
                rcvProfile_print (&datRcv, "Dat");
                print_statistics (&ctx, &prv, '\n'); 
                trace_print (&trace, ctx.history);
                rcvFd = reconnect_client (nread,
                                          rcvFd,
                                          srvFd,
                                          &prms, 
                                          "reading data");
                invalidate_ctx (&ctx);
                continue;
             }

             if (prms.chkData) 
             {
                unsigned int err = checkData (&ctx, data, ndata);
                if (err)
                {
                   rcvProfile_print (&hdrRcv, "Hdr");
                   rcvProfile_print (&datRcv, "Dat");
                }
             }

             ctx.stats.datCnt += 1;
             ctx.stats.datSiz += ndata;

          }
       }


       /* 
        | Periodically (once a second) output the accumuated statistics
        | and status 
       */
       time_t prvTime = ctx.timestamp;
       time (&ctx.timestamp);
       if (ctx.timestamp != prvTime)
       {
          /*
          printf ("Cur:Prv %u:%u\n", 
                  (unsigned int)ctx.timestamp,
                  (unsigned int)prvTime);
          */


          char c;
          if (eject-- == 0)
          {
             // Reset the failure count
             ctx.nfailures = 0;

             c       = '\n';
             need_lf = 0;
             eject   = 60;
          }
          else
          {
             c       = '\r';
             need_lf = 1;
          }

          print_statistics (&ctx, &prv, c);

          need_lf = 1;
          prv     = ctx;
       }
    }
   
    close (srvFd);
    close (rcvFd);

    return 0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief   Opens a socket on the specified port 
  \return  The opened socket

  \param[ in]   portno  The port number
  \param[ in]  rcvSize  Size, in bytes of the TCP receive buffer
  \param[out]    srvFd  Returned as the server socket

   This is a very cheap implementation.  It expects that the other side
   has is already setup.  As such, this executable must be started after
   the client side has been started.
                                                                          */
/* ---------------------------------------------------------------------- */
static int open_client (int portno, int rcvSize, int nodelay, int *srvFd)
{
    int     listenFd;

    struct sockaddr_in srvAdr;
    printf  ("Open socket\n");


    // Init structures
    listenFd = socket (AF_INET,  SOCK_STREAM,  0);
    memset (&srvAdr, 0, sizeof (srvAdr));

    printf ("Got listening socket %d\n", listenFd);

    srvAdr.sin_family      = AF_INET;
    srvAdr.sin_addr.s_addr = INADDR_ANY;
    srvAdr.sin_port        = htons(portno);


    if (bind (listenFd, (struct sockaddr *) &srvAdr, sizeof(srvAdr)) < 0) 
    {
       printf ("ControlServer::startListen -> Failed to bind socket %5d\n",
               portno);
        exit(-1);
    }
   
    int cliFd;
    cliFd  = connect_client (listenFd, portno, rcvSize, nodelay);
   *srvFd = listenFd;
    return cliFd;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static int connect_client (int listenFd, 
                           int   portno, 
                           int  rcvSize,
                           int  nodelay)
{
    socklen_t cliLen;
    int        cliFd;
    struct sockaddr_in  cliAddr;


    // Start listen
    printf ("Listening for a connection on port %5d\n", portno);
    listen (listenFd, 5);
    
   
    cliLen = sizeof (cliAddr);
    cliFd  = accept (listenFd, (struct sockaddr *)&cliAddr, &cliLen);
    if(cliFd < 0)
    {
       puts ("Error on Accept");
    }    
    else
    {
       puts ("Accepted connection from client");
       setsockopt (cliFd, SOL_SOCKET, SO_RCVBUF, &rcvSize, sizeof (int));


       int iss;
       iss = setsockopt (cliFd, SOL_TCP, TCP_NODELAY, &nodelay, sizeof (int));
       if (iss < 0)
       {
          printf ("Error setting NODELAY flag %d\n", h_errno);
       }

    }



    return cliFd;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Tries to reconnect to the client after a disconnect
  \return The reconnected socket fd

  \parampin]    nread  The last return value from rcv
  \param[in] clientFd  The old client socket fd (to be closed)
  \param[in] serverFd  The socket fd to accept the new connection
  \param[in]     prms  The connection parameters
  \param[in]      msg  A string indicating the reason for disconnecting
                                                                          */
/* ---------------------------------------------------------------------- */
static int reconnect_client (ssize_t     nread,
                             int      clientFd,
                             int      serverFd,
                             Prms const  *prms,
                             char  const  *msg)
{
   if (nread == 0)
   {
      printf ("Disconnect while %s\n"
              "Try reconnecting\n",  msg);

      close (clientFd);
      clientFd = connect_client (serverFd, 
                                 prms->portNumber, 
                                 prms->rcvSize,
                                 prms->nodelay);
   }
   else if (nread < 0)
   {
      printf ("Receive error while %s, quitting\n", msg);
      exit (-1);
   }

   
   return clientFd;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Provides a newline if one is needed
  \retval == 0, always

  \param[in] need_lf  If non-zero, then a newline is output to stdout
                                                                          */
/* ---------------------------------------------------------------------- */
static inline char if_newline (char need_lf)
{
   if (need_lf) putchar ('\n');
   return 0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Print a couple of the critical options for the specified socket

  \param[in] fd  The socket's fd
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_socketopts (int fd)
{
    int       rcvSize;
    int       nodelay;
    socklen_t optSize;

    /* Retrieve the socket's receive buffer size */
    optSize = sizeof (rcvSize);
    getsockopt (fd, SOL_SOCKET, SO_RCVBUF, &rcvSize, &optSize);


    /* Retrieve whether the ACK's are delayed (Nagle Algorithm) or not */
    optSize = sizeof (nodelay);
    getsockopt (fd, SOL_TCP, TCP_NODELAY, &nodelay, &optSize);


    printf ("SO_RCVBUF   = %8.8x\n", rcvSize);
    printf ("TCP_NODELAY = %8d\n",   nodelay);

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Prints the frame header

  \param[in]  data   The header data
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_hdr (uint8_t const *data)
{
   printf ("Hdr: %8.8" PRIx32 " %8.8" PRIx32 " %8.8"  PRIx32 " %8.8" PRIx32 "\n",
           get_word (data +  0),
           get_word (data +  4),
           get_word (data +  8),
           get_word (data + 12));
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static ssize_t read_data (int              fd, 
                          uint8_t       *data,
                          int           ndata,
                          RcvProfile *profile)
{
   int       cnt = 1;
   ssize_t nread = recv (fd, data, ndata, 0);
   ssize_t left  = ndata;

   profile->hist[0] = nread;

   while ( (left -= nread) > 0)
   {
      unsigned int idx;
      data     += nread;

      nread = recv (fd, data , left, 0);

      idx = (cnt >= sizeof (profile->hist) / sizeof (profile->hist[0]))
         ? (        sizeof (profile->hist) / sizeof (profile->hist[0]) - 1)
          : cnt;


      profile->hist[idx] = nread;

      cnt += 1;
      if (nread <= 0) goto EXIT;
   }

   nread = ndata;

EXIT:
   profile->cnt = cnt;
   profile->tot = ndata - left;

   return nread;
}
/* ---------------------------------------------------------------------- */

   

