// -*-Mode: C;-*-

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     tcp_receiver.c
 *  @brief    Toy TCP/IP receiver for data from the RCEs
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
 *  pdd
 *
 *  @author
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  <2017/06/19>
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
   2018.05.16 jjr Major refactoring. Moved record checking and printing 
                  into their own files so that they can be shared with
                  the rssi_receiver.
   2018.01.30 jjr Redid the title line to more accurately reflect the 
                  quantities.  For example Bps -> bps since this is the
                  traditional nomenclature for bits per second.
   2017.10.19 jjr Remove execute flag when creating the output file
                  Format the last packet descriptor as the terminator.
   2017.08.28 jjr Fix position of trigger type in auxilliary block to 
                  match the documentation
   2017.08.29 jjr Added documentation and history block
  
\* ---------------------------------------------------------------------- */

#include "TpcPrinter.h"
#include "TpcCheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <inttypes.h>
#include <getopt.h>
#include <errno.h>



/* ---------------------------------------------------------------------- *//*!

  \enum   Mode
  \brief  Describes the operational modes
                                                                          */
/* ---------------------------------------------------------------------- */
enum Mode
{
   MODE_K_MONITOR = 0,  
   MODE_K_DUMP    = 1
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \class _Prms
  \brief  The configuration parameters
                                                                          *//*!
  \typedef Prms
  \brief   Typedef for struct _Prms
                                                                          */
/* ---------------------------------------------------------------------- */
struct _Prms
{

   enum Mode        mode;   /*!< Operational mode                         */
   int        portNumber;   /*!< The port to connect on the RCE side      */
   int           rcvSize;   /*!< Size of the receiver buffer in bytes     */
   int              data;   /*!< The number of data words to print        */
   int           nodelay;   /*!< Value of the TCP_NODELAY parameter       */
   int         nfailures;   /*!< Maximum number of failure messages       */
   char          chkData;   /*!< Perform the data check                   */
   char const *ofilename;   /*!< Output file name                         */
};
/* ---------------------------------------------------------------------- */
typedef struct _Prms Prms;
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



/* ---------------------------------------------------------------------- */
struct RcvProfile_s
{
   unsigned int       cnt;  /*!< The number of reads                      */
   unsigned int       tot;  /*!< Total number of bytes read               */
   unsigned int hist[256];  /*!< The size, in bytes of each read          */
};
/* ---------------------------------------------------------------------- */
typedef struct RcvProfile_s RcvProfile;
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




/* ---------------------------------------------------------------------- */
/* LOCAL PROTOTYPES                                                       */
/* ---------------------------------------------------------------------- */

static void    getPrms           (Prms         *prms,
                                  int           argc, 
                                  char *const argv[]);

static int     monitor           (Prms const   *prms);
static int      dump             (Prms const   *prms);


static int     create_file       (char const *filename);

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

static int     rnd64             (unsigned int  nbytes);

static ssize_t read_data         (int              fd,
                                  uint8_t       *data,
                                  int           ndata,
                                  RcvProfile *profile);

static inline char     if_newline (char        need_lf);
static inline int      rnd64      (unsigned int nbytes);
static inline uint64_t get_w64    (uint8_t const *data);


static void    rcvProfile_print (RcvProfile const *profile, 
                                 char const         *title);

static inline void  clear_errors  (Errors *errors);

static inline void       init_ctx (Ctx *ctx);
static inline void      clear_ctx (Ctx *ctx);
static inline void invalidate_ctx (Ctx *ctx);


static void clear_statistics        (Statistics *statistics);
static int  print_statistics_update (Ctx       *cur,
                                     Ctx       *prv,
                                     int    need_lf);
static void print_statistics_title  ();
static void print_statistics        (Ctx const *cur, 
                                     Ctx const *prv, 
                                     char       eol);
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief  Code to test TCP/IP reception

  \param[in]  argc   Command line argument count
  \param[in]  argv   Vecto of command line parameters
                                                                          */
/* ---------------------------------------------------------------------- */
int main(int argc, char *const argv[])
{
   int status = 0;
   Prms      prms;
   getPrms (&prms, argc, argv);

   // -----------------------------------------------
   // Dispatch to the appropriate data reader/handler
   // -----------------------------------------------
   if (prms.mode == MODE_K_DUMP)
   {
      status = dump (&prms);
   }

   else if (prms.mode == MODE_K_MONITOR)
   {
      status = monitor (&prms);
   }

   else
   {
      fprintf (stderr,
               "Error: Unrecognized operational mode: %d\n",
               (int)prms.mode);
   }

    return status;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Receives, monitors and optionally writes it to an output file

  \param  prms  The control parameters
                                                                          */
/* ---------------------------------------------------------------------- */
static int monitor (Prms const *prms)
{
    uint32_t headerSize = sizeof (uint64_t);
    uint32_t   maxBytes = 8*1024*1024;
    uint8_t     *rxData = malloc (maxBytes); 
    char        need_lf = 0;

    bzero (rxData, maxBytes);
    Ctx           ctx;
    Ctx           prv;
    init_ctx   (&ctx);
    init_ctx   (&prv);

    // -----------------------------------------
    // If requested, create a binary output file
    // -----------------------------------------
    int fd = (prms->ofilename) 
           ? create_file (prms->ofilename) 
           : -1;


    unsigned int retries[128];
    memset (retries, 0, sizeof (retries));


    int         srvFd;
    int         rcvFd  = open_client (prms->portNumber, 
                                      prms->rcvSize, 
                                      prms->nodelay,
                                      &srvFd);

    uint8_t *dataStatic = rxData + headerSize;

    print_socketopts  (rcvFd);
    print_statistics_title ();

    while (1)
    {
       RcvProfile hdrRcv;
       RcvProfile datRcv;

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
                                      prms, 
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


       ctx.stats.hdrCnt += 1;
       ctx.stats.hdrSiz += headerSize;

       uint32_t failures = checkHeader (&ctx, rxData, headerSize);


       if (failures)
       {
          int nfailures = ctx.nfailures++;
          if (nfailures  < prms->nfailures)
          {
             need_lf = if_newline (need_lf);
             printf ("Failure = %8.8"PRIx32"\n", failures);
          }
       }
      
       /* If the header is OK, read the 'data' associated with it */
       if (failures == 0)
       {
          ssize_t  received = nread;
          uint64_t header   = get_w64 (rxData);
          uint32_t dataSize = ((header >> 8) & 0xffffff) * sizeof (uint64_t);


          /* Ensure that the buffer can handle the data volume */
          if (dataSize > maxBytes)
          {
             printf ("Packet too large  %x > %" PRIx32 "\n", dataSize, maxBytes);
          }
          
          if (received < dataSize)
          {
             static int Count = 0;
             uint8_t  *data =   rxData + headerSize;
             uint32_t ndata = dataSize - headerSize;
             nread          = read_data (rcvFd, data, ndata, &datRcv);

             if (data != dataStatic)
             {
                printf ("Error data pointer %p != %p\n",
                        data, dataStatic);
             }

             if (received != headerSize)
             {
                Count += 1;
             }

             /* 
              | Make a cheap histogram of the number of rcv calls
              | in an attempt to see if this at all correlates 
              | with the failures.
             */
             {
                int cnt = datRcv.cnt;
                if (cnt > sizeof (retries) / sizeof (retries[0]))
                {
                   cnt = sizeof (retries) / sizeof (retries[0]) - 1;
                }
                retries[cnt] += 1;
             }

             /* Check if had error or disconnect */
             if (nread <= 0)
             {
                need_lf = if_newline (need_lf);
                printf ("Error reading %u bytes\n", ndata);
                printf ("\n");
                print_hdr (get_w64 (rxData));
                printf ("\n");
                rcvProfile_print (&hdrRcv, "Hdr");
                rcvProfile_print (&datRcv, "Dat");
                print_statistics (&ctx, &prv, '\n'); 
                rcvFd = reconnect_client (nread,
                                          rcvFd,
                                          srvFd,
                                          prms, 
                                          "reading data");
                invalidate_ctx (&ctx);
                continue;
             }


             if (fd >= 0)
             {
                ssize_t nwrite = headerSize + nread;
                ssize_t nwrote = write (fd, rxData, nwrite);
                if (nwrote != nwrite)
                {
                   fprintf (
                      stderr,
                      "Error %d writing output %zd != %zd bytes to write\n",
                      errno,
                      nwrite, nwrote);
                   exit (-1);
                }
             }


             uint64_t const *pTrailer = (uint64_t const *)
                                        (data + ndata - sizeof (*pTrailer));

             if (prms->chkData) 
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

             //uint64_t trailer = *pTrailer;
             //printf ("Trailer: %16.16" PRIx64 "\n", trailer);

          }
       }

       need_lf = print_statistics_update (&ctx, &prv, need_lf);
    }
   
    close (srvFd);
    close (rcvFd);

    return 0;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Receives, prints out a summary of the received event and
         optionally writes it to an output file

  \param  prms  The control parameters
                                                                          */
/* ---------------------------------------------------------------------- */
static int dump (Prms const *prms)
{
    uint32_t              headerSize = sizeof (uint64_t);
    uint32_t		    maxBytes = 8*1024*1024;


    uint8_t  *rxData = malloc (maxBytes); 
    char     need_lf =  0;

    bzero (rxData, maxBytes);
    Ctx           ctx;
    Ctx           prv;
    init_ctx   (&ctx);
    init_ctx   (&prv);

    // -----------------------------------------
    // If requested, create a binary output file
    // -----------------------------------------
    int fd = (prms->ofilename) 
           ? create_file (prms->ofilename) 
           : -1;


    unsigned int retries[128];
    memset (retries, 0, sizeof (retries));


    int         srvFd;
    int         rcvFd  = open_client (prms->portNumber, 
                                      prms->rcvSize, 
                                      prms->nodelay,
                                      &srvFd);

    uint8_t *dataStatic = rxData + headerSize;

    print_socketopts  (rcvFd);
    print_statistics_title ();

    while (1)
    {
       RcvProfile hdrRcv;
       RcvProfile datRcv;

       puts ("\n ------ NEW EVENT ------");

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
                                      prms, 
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
       print_hdr (get_w64 (rxData));


       if (failures)
       {
          int nfailures = ctx.nfailures++;
          if (nfailures  < prms->nfailures)
          {
             need_lf = if_newline (need_lf);
             printf ("Failure = %8.8"PRIx32"\n", failures);
          }
       }
      
       /* If the header is OK, read the 'data' associated with it */
       if (failures == 0)
       {
          ssize_t  received = nread;
          uint64_t header   = get_w64 (rxData);
          uint32_t dataSize = ((header >> 8) & 0xffffff) * sizeof (uint64_t);


          /* Ensure that the buffer can handle the data volume */
          if (dataSize > maxBytes)
          {
             printf ("Packet too large  %x > %" PRIx32 "\n", dataSize, maxBytes);
          }
          
          if (received < dataSize)
          {
             static int Count = 0;
             uint8_t  *data =   rxData + headerSize;
             uint32_t ndata = dataSize - headerSize;
             nread          = read_data (rcvFd, data, ndata, &datRcv);

             if (data != dataStatic)
             {
                printf ("Error data pointer %p != %p\n",
                        data, dataStatic);
             }

             if (received != headerSize)
             {
                Count += 1;
             }

             /* 
              | Make a cheap histogram of the number of rcv calls
              | in an attempt to see if this at all correlates 
              | with the failures.
             */
             {
                int cnt = datRcv.cnt;
                if (cnt > sizeof (retries) / sizeof (retries[0]))
                {
                   cnt = sizeof (retries) / sizeof (retries[0]) - 1;
                }
                retries[cnt] += 1;
             }

             /* Check if had error or disconnect */
             if (nread <= 0)
             {
                need_lf = if_newline (need_lf);
                printf ("Error reading %u bytes\n", ndata);
                printf ("\n");
                print_hdr (get_w64 (rxData));
                printf ("\n");
                rcvProfile_print (&hdrRcv, "Hdr");
                rcvProfile_print (&datRcv, "Dat");
                print_statistics (&ctx, &prv, '\n'); 
                rcvFd = reconnect_client (nread,
                                          rcvFd,
                                          srvFd,
                                          prms, 
                                          "reading data");
                invalidate_ctx (&ctx);
                continue;
             }

             print_id ((uint64_t const *)data);

             if (fd >= 0)
             {
                ssize_t nwrite = headerSize + nread;
                ssize_t nwrote = write (fd, rxData, nwrite);
                if (nwrote != nwrite)
                {
                   fprintf (
                      stderr,
                      "Error %d writing output %zd != %zd bytes to write\n",
                      errno,
                      nwrite, nwrote);
                   exit (-1);
                }
             }

             print_record ((uint64_t const *)data, ndata/sizeof (uint64_t));


             if (prms->chkData) 
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

             uint64_t const *pTrailer = (uint64_t const *)
                                        (data + ndata - sizeof (*pTrailer));

             uint64_t trailer = *pTrailer;
             printf ("Trailer: %16.16" PRIx64 "\n", trailer);

          }
       }

       need_lf = print_statistics_update (&ctx, &prv, need_lf);
    }
   
    close (srvFd);
    close (rcvFd);

    return 0;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- */
static int print_statistics_update (Ctx *cur, Ctx *prv, int need_lf)
{
   static int Eject = 60;

   /* 
    | Periodically (once a second) output the accumuated statistics
    | and status 
   */
   time_t prvTime = cur->timestamp;
   time   (&cur->timestamp);

   if (cur->timestamp != prvTime)
   {
      /*
        printf ("Cur:Prv %u:%u\n", 
        (unsigned int)cur->timestamp,
        (unsigned int)prvTime);
      */

      char c;
      if (Eject-- == 0)
      {
         // Reset the failure count
         cur->nfailures = 0;
         
         c       = '\n';
         need_lf = 0;
         Eject   = 60;
      }
      else
      {
         c       = '\r';
         need_lf = 1;
      }

      print_statistics (cur, prv, c);
      
      need_lf =    1;
     *prv     = *cur;
   }

   return need_lf;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief  Retrieve of set defaults for the governing parameters
                                                                          */
/* ---------------------------------------------------------------------- */
static void getPrms (Prms *prms, int argc, char *const argv[])
{
    int c;
    int         portNumber =           8991;
    int         rcvSize    =     128 * 1024;
    int         data       =              0;
    char        chkData    =              0;
    int         nodelay    =              0;
    int         nfailures  =             25;
    char const *ofilename  =           NULL;
    enum Mode   mode       = MODE_K_MONITOR;


    while ( (c = getopt (argc, argv, "dmo:f:p:r:xd:n:")) != EOF)
    {
       if       (c == 'f') nfailures  = strtoul (optarg, NULL, 0);
       else if  (c == 'm') mode       = MODE_K_MONITOR;
       else if  (c == 'd') mode       = MODE_K_DUMP;
       else if  (c == 'd') data       = strtoul (optarg, NULL, 0);
       else if  (c == 'p') portNumber = strtoul (optarg, NULL, 0);
       else if  (c == 'n') nodelay    = strtoul (optarg, NULL, 0);
       else if  (c == 'o') ofilename  = optarg;
       else if  (c == 'r') rcvSize    = strtoul (optarg, NULL, 0);
       else if  (c == 'x') chkData    = 1;
    }

    prms->mode       = mode;
    prms->portNumber = portNumber;
    prms->rcvSize    = rcvSize;
    prms->data       = data;
    prms->chkData    = chkData;
    prms->nfailures  = nfailures;
    prms->ofilename  = ofilename;
    prms->nodelay    = nodelay != 0;

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Create the output file
  \return The file descriptor or -1 in case the file could not be created.

  \param[in] filename The name of the file to create
                                                                          */
/* ---------------------------------------------------------------------- */
static int create_file (char const *filename)
{
    int fd = -1;

    fd = creat (filename,  S_IRUSR | S_IWUSR 
                         | S_IRGRP | S_IWGRP
                         | S_IROTH);
    if (fd >= 0)
    {
       fprintf (stderr, "Output file is: %s\n", filename);
    }
    else
    {
       fprintf (stderr, "Error opening output file: %s err = %d\n",
                filename, errno);
    }

    return fd;
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



/* ---------------------------------------------------------------------- *//*!

  \brief  Round the specified number of bytes up to the next 64-bit value
  \return The rounded value

  \param[in] nbytes  The value to round up
                                                                          */
/* ---------------------------------------------------------------------- */
static inline int rnd64 (unsigned int  nbytes)
{
   int n64 = (nbytes + (sizeof (uint64_t) - 1)) / sizeof (uint64_t);
   return n64;
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
static inline uint64_t get_w64 (uint8_t const *data)
{
   uint64_t w = *(uint64_t const *)data;
   return w;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Puts of the statistics/status title line
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void print_statistics_title ()
{
   puts (
   "   Rate        bps SeqErrs DatErrs BadData NHeader   NData     Bytes\n"
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
