// -*-Mode: C;-*-

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     rssi_receiver.cpp
 *  @brief    Toy RSSI receiver for data from the RCEs
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
 *  util
 *
 *  @author
 *  <russell@slac.stanford.edu>
 *
 *  @par Date created:
 *  <2018/06/05>
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
   2018.06.05 jjr Added documentation/history header.
                  Modified the copying/accessing of the data in acceptFrame
                  to use a faster access.  This allowed the rate to go to
                  at least 1.7Gbps
  
\* ---------------------------------------------------------------------- */

// This must go first in order to get things like PRIx32 defined
#include <cinttypes>

#include "TpcPrinter.h"

#include <rogue/protocols/udp/Core.h>
#include <rogue/protocols/udp/Client.h>
#include <rogue/protocols/rssi/Client.h>
#include <rogue/protocols/rssi/Transport.h>
#include <rogue/protocols/rssi/Application.h>
#include <rogue/protocols/packetizer/CoreV2.h>
#include <rogue/protocols/packetizer/Core.h>
#include <rogue/protocols/packetizer/Transport.h>
#include <rogue/protocols/packetizer/Application.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/interfaces/stream/Buffer.h>
#include <rogue/Logging.h>


#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>


/* ---------------------------------------------------------------------- *//*!

   \brief Class to parse and capture the command line parameters
                                                                          */
/* ---------------------------------------------------------------------- */
class Parameters
{
public:
   Parameters (int argc, char *const argv[]);


public:
   char  const *getIp           () const { return           m_ip; }
   int          getNframes      () const { return      m_nframes; }
   bool         getCopyFlag     () const { return         m_copy; }
   int          getNsdump       () const { return       m_nsdump; }
   int          getNedump       () const { return       m_nedump; }
   int          getDisplay      () const { return      m_display; }
   char const  *getOfilename    () const { return    m_ofilename; }
   int32_t      getLoggingLevel () const { return m_loggingLevel; }
   char const  *getLoggingName  () const { return  m_loggingName; }
   int          getRefresh      () const { return      m_refresh; }
   bool         getQuietFlag    () const { return        m_quiet; }
   void         echo            () const; 

   static uint32_t getLoggingLevel (char const *levelSpec,
                                    char const **loggingName);
   static void reportUsage      ();

public:
   char  const         *m_ip;
   char  const  *m_ofilename;
   int             m_refresh;
   int             m_nframes;
   int              m_nsdump;
   int              m_nedump;
   int             m_display;
   bool               m_copy;
   bool              m_quiet;
   int32_t    m_loggingLevel;
   char const *m_loggingName;
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/* Forward References                                                     */
/* ---------------------------------------------------------------------- */
class RssiConnection;
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Class to receive incoming data
                                                                          */
/* ---------------------------------------------------------------------- */
class Receiver : public rogue::interfaces::stream::Slave 
{
public:
   Receiver (RssiConnection *connection, 
             int             nsdump = 0,
             int             nedump = 0,
             int           ndisplay = 0,
             bool      copyFlag = false, 
             int          outputFd = -1);


public:
   void acceptFrame (boost::shared_ptr<rogue::interfaces::stream::Frame> frame);


   /* ------------------------------------------------------------------- *//*!

      \brief Class to monitor and accumulate statistics about the 
             reception of incoming frames
                                                                          */
   /* ------------------------------------------------------------------- */
   class Statistics
   {
   public:
      Statistics ();
      Statistics (Statistics volatile &stats);


   public:
      static void    gettime (struct timespec          *ts);
      static void    gettime (struct timespec volatile *ts);
      static int64_t subtime (struct timespec const *start,
                              struct timespec const  *stop);

   public:
      struct timespec m_time;  /*!< The timestamp of these statistics     */
      uint32_t     m_rxCount;  /*!< Count of the incoming events          */
      uint64_t     m_rxBytes;  /*!< Total number of bytes received        */
      uint32_t      m_rxLast;  /*!< Number of bytes in last event seen    */
      uint32_t    m_packDrop;  /*!< Number of dropped packets             */
      uint32_t    m_rssiDrop;  /*!< Number of dropped rssi packets        */
   };
   /* ------------------------------------------------------------------- */


public:
   RssiConnection *m_connection;  /*!< The connection                     */
   Statistics volatile  m_stats;  /*!< The shared statistics              */
   int                 m_nsdump;  /*!< # 64-bit words to dump(non-erring) */
   int                 m_nedump;  /*!< # 64-bit words to dump(erring)     */
   int                m_display;  /*!< Current value of display countdown */
   int           m_displayCount;  /*!< Refresh display countdown value    */
   bool                  m_copy;  /*!< Copy flag, set true, if fd >= 0    */
   int                     m_fd;  /*!< Output file descriptor             */
};
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Class to =captures the sequence needed to establish an RSSI 
         connection
                                                                          */
/* ---------------------------------------------------------------------- */
class RssiConnection
{
public:
   RssiConnection (char const *ip, int nframes);

public:
   void connect (boost::shared_ptr<Receiver> receiver);

public:
   rogue::protocols::udp::ClientPtr       m_udp;
   rogue::protocols::rssi::ClientPtr     m_rssi;
   rogue::protocols::packetizer::CorePtr m_pack;
};
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- */
/* Local Prototypes                                                       */
/* ---------------------------------------------------------------------- */
static int  create_file            (char const            *filename);
static void dump                   (uint64_t const               *d,
                                    int                           n);
static void print_statistics_title ();
static void print_statistics       (Receiver::Statistics const *cur,
                                    Receiver::Statistics const *prv,
                                    bool                     opened,
                                    char                        eol);
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

   \brief  Main program to establish and accept data from an RSSI stream

   \param[in] argc The  count of command line arguments
   \param[in] argv The vector of command line arguments
                                                                          */
/* ---------------------------------------------------------------------- */
int main (int argc, char **argv) 
{
   Parameters prms (argc, argv);

   prms.echo ();

   char const      *daqHost = prms.getIp       (); ///"192.168.2.110";
   int              nframes = prms.getNframes  ();
   int               nsdump = prms.getNsdump   ();
   int               nedump = prms.getNedump   ();
   int             ndisplay = prms.getDisplay  ();
   bool            copyFlag = prms.getCopyFlag ();
   char const    *ofilename = prms.getOfilename();
   int32_t     loggingLevel = prms.getLoggingLevel ();
   int                   fd = ofilename ? create_file (ofilename) : -1;

  
   if (loggingLevel > 0)
   {
      rogue::Logging::setLevel (loggingLevel);
   }


   RssiConnection connection (daqHost, nframes);


   // Create a receiver and connect to channel 0 of the packetizer
   boost::shared_ptr<Receiver> 
          receiver = boost::make_shared<Receiver>(&connection, 
                                                  nsdump,
                                                  nedump,
                                                  ndisplay,
                                                  copyFlag, 
                                                  fd);
   connection.connect (receiver);


   print_statistics_title ();

   // Loop forever showing counts
   Receiver::Statistics prv; 
   while(1)
   {
      sleep        (1);

      // Copy and print the statistics
      Receiver::Statistics stats = receiver->m_stats;
      print_statistics (&stats, &prv, connection.m_rssi->getOpen (), '\n');


      // Hold on to the previous copy
      prv = stats;

   }

   return 0;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \brief Parse out the command line parameters
  
  \param[in] argc  The  count of command line parameters
  \param[in] argv  Teh vector of command line parameters

  \par
   This class exits if the command line parameters are ill-specified
                                                                          */
/* ---------------------------------------------------------------------- */
Parameters::Parameters (int argc, char *const argv[]) :
   m_ip           (NULL),
   m_ofilename    (NULL),
   m_refresh         (1),
   m_nframes        (64),
   m_nsdump          (0),
   m_nedump          (0),
   m_display         (0),
   m_copy        (false),
   m_quiet       (false),
   m_loggingLevel   (-1),
   m_loggingName ("-- None --")
{
   int c;

   while ( (c = getopt (argc, argv, "cd:e:n:o:l:s:q")) != EOF)
   {
      if      (c == 's') { m_nsdump       = strtol (optarg, NULL, 0);         }
      else if (c == 'd') { m_display      = strtol (optarg, NULL, 0);         }
      else if (c == 'e') { m_nedump       = strtol (optarg, NULL, 0);         }
      else if (c == 'n') { m_nframes      = strtol (optarg, NULL, 0);         }
      else if (c == 'c') { m_copy         = true;                             }
      else if (c == 'o') { m_ofilename    = optarg;                           }
      else if (c == 'l') { m_loggingLevel = getLoggingLevel (optarg, 
                                                             &m_loggingName); }
      else if (c == 'r') { m_refresh      = strtol (optarg, NULL, 0);         }
      else if (c == 'q') { m_quiet        = true;                             }
   }


   if (optind < argc)
   {
      m_ip = argv[optind];
   }
   else
   {
      std::cerr << "Error: IP of the sender was not specified" << std::endl
                << std::endl;
      reportUsage ();
      exit (-1);
   }

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief     Convenience method to determine the RSSI logging level
  \retval >  0, the logging level
  \retval == 0, no logging requested

  \param[in] l  The first letter of one the logging levels
                  - Critical
                  - Error
                  - Warning
                  - Info
                  - Debug
                                                                          */
/* ---------------------------------------------------------------------- */
uint32_t Parameters::getLoggingLevel (char const *levelSpec, 
                                      char const     **name)
{
   uint32_t level = 0;
   char         l = levelSpec[0];
            

   if      (l == 'C') { level = rogue::Logging::Critical; *name = "Critical"; }
   else if (l == 'E') { level = rogue::Logging::Error;    *name = "Error";    }
   else if (l == 'W') { level = rogue::Logging::Warning;  *name = "Warning";  }
   else if (l == 'I') { level = rogue::Logging::Info;     *name = "Info";     }
   else if (l == 'D') { level = rogue::Logging::Debug;    *name = "Debug";    }
   else
   {
      std::cerr 
<< "Error::Illegal option specified for the logging level: " << levelSpec << std::endl
<< "       Must be one of Critical, Error, Warning Info, Debug" << std::endl;
      reportUsage ();
      exit (-1);
   }

   return level;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Echo the command line parameters
                                                                          */
/* ---------------------------------------------------------------------- */
void Parameters::echo () const
{
   // ----------------------------------
   // Check if should skip the reporting
   // ----------------------------------
   if (m_quiet) return;


   std::cout 
   << "Starting rssi_receiver" << std::endl
   << "  Sender's          IP: "  << m_ip                    << std::endl
   << "  # of receive  frames: "  << m_nframes               << std::endl
   << "  # words to dump (ok): "  << m_nsdump                << std::endl
   << "  # words to dump(err): "  << m_nedump                << std::endl
   << "  Display             : "  << m_display               << std::endl
   << "  Refresh rate        : "  << m_refresh << " secs"    << std::endl
   << "  Copy  incoming  data: "  << (m_copy ? "Yes" : "No") << std::endl
   << "  Logging level       : "  << m_loggingName           << std::endl
   << "  Output file name    : "  << (m_ofilename ? m_ofilename : "-- None --") 
   << std::endl
   << std::endl;


   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Report the command line usage
                                                                          */
/* ---------------------------------------------------------------------- */
void Parameters::reportUsage ()
{
   using namespace std;
   cout 
<< "Usage:" << endl
<< "$ rssi_receiver [cd:e:ln:os:r:] ip" << endl
<< "  where:" << std::endl
<< "      c:  If present, the frame data is copied into a temporary buffer"   << endl
<< "      d:  Display every nth event, default = 0, do not display"           << endl
<< "      e:  Number of 64-bit words to dump for erroring frames"             << endl
<< "      s:  Number of 64-bit words to dump for non-erroring frames"         << endl
<< "      n:  The number of incoming frames to buffer, default = 64"          << endl
<< "      l:  The logging level, one of Critical, Error, Warning Info, Debug" << endl
<< "      o:  If present, then name of an output file"                        << endl
<< "      r:  The display refresh rate in seconds (default = second)"         << endl
<< "     ip:  The ip address of data source"                                  << endl
<<                endl
<< " Example:" << endl
<< " $ rssi_receiver -l Info -n 100 -o/tmp/dump.dat 192.168.2.110"            << endl;
   
   return;
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

   if (n & 0x3) putchar ('\n');

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
    if (fd < 0)
    {
       fprintf (stderr, "Error opening output file: %s err = %d\n",
                filename, errno);
       exit (-1);
    }

    return fd;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Constructor for the RSSI receiver

  \param[in] connection
  \param[in]     nsdump  The number of 64-bit words to dump for 
                         non-erring frames
  \param[in]     nedump  The number of 64-bit words to dump for
                         erring frames
  \param[in]   ndisplay  Display every nth frame (0 = never)
  \param[in]   copyFlag  Flags indicating whether to copy the data or not
  \param[in]   outputFd  If >= 0, a file descriptor to write the output to
                                                                          */
/* ---------------------------------------------------------------------- */
inline Receiver::Receiver (RssiConnection *connection, 
                           int                 nsdump,
                           int                 nedump,
                           int               ndisplay,
                           bool              copyFlag,
                           int               outputFd) :
   m_connection (connection),
   m_stats                (),
   m_nsdump         (nsdump),
   m_nedump         (nedump),
   m_display      (ndisplay),
   m_displayCount (ndisplay),
   m_copy         (copyFlag),
   m_fd           (outputFd)
{
   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Accepts a new incoming frame

  \param[in] frame  The incoming frame
                                                                          */
/* ---------------------------------------------------------------------- */
void Receiver::
     acceptFrame (boost::shared_ptr<rogue::interfaces::stream::Frame> frame ) 
{
   Statistics::gettime (&m_stats.m_time);

   auto    nbytes     = frame->getPayload();
   m_stats.m_rxLast   = nbytes;
   m_stats.m_rxBytes += nbytes;
   m_stats.m_rxCount += 1;


   m_stats.m_rssiDrop = m_connection->m_rssi->getDropCount();
   m_stats.m_packDrop = m_connection->m_pack->getDropCount();

      
   auto err = frame->getError ();
   if (err)
   {
      std::cout << "Frame error: " << err << std::endl;
   }



   m_display -= 1;

   // Copy to buffer
   if (m_copy || m_nsdump || m_nedump || m_fd >= 0 || m_display == 0)
   {
      ///std::cout << "Copying nbytes: " << nbytes << std::endl;


      // Iterator to start of buffer
      rogue::interfaces::stream::Frame::iterator iter = frame->beginRead();
      rogue::interfaces::stream::Frame::iterator  end = frame->endRead();


      uint8_t *buff = (uint8_t *)malloc (nbytes);
      uint8_t  *dst = buff;


      //Iterate through contigous buffers
      while ( iter != end ) {
         rogue::interfaces::stream::Frame::iterator nxt = iter.endBuffer();
         auto size = iter.remBuffer ();
         auto *src = iter.ptr       ();
         memcpy(dst, src, size);
         dst += size;
         iter = nxt;
      }


      uint64_t const *header = (uint64_t const *)buff;
      uint64_t const      *d = (uint64_t const *)buff;
      uint64_t const   *data = header + 1;
      size_t           ndata = nbytes / sizeof (uint64_t) - 1;


      if ((header[0] >> 40) != 0x8b309e)
      {
         printf ("Error : Unsynched frame %8.8" PRIx32 "bytes\n",
                 (unsigned int)nbytes);
         dump (d, m_nedump);
         m_display += 1;
      }
      else
      {
         if (m_nsdump)
         {
            printf ("Success:   Synched frame %8.8" PRIx32 "bytes\n", 
                    (unsigned int)nbytes);
   
            dump (d, m_nsdump);
         }

         if (m_fd >= 0)
         {
            ssize_t nwrote = write (m_fd, buff, nbytes);
            if (nwrote != nbytes)
            {
               fprintf (stderr, "Error %d writing the output file\n", errno);
               exit (errno);
            }
         }

         if (m_display == 0)
         {
            m_display =  m_displayCount;
            print_hdr    (header[0]);
            print_id     (data);
            print_record (data, ndata);
         }
      }

      free(buff);
   }

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief  The constructor to initialize the statistics keep by the
           receiver
                                                                          */
/* ---------------------------------------------------------------------- */
inline Receiver::Statistics::Statistics () :
   m_rxCount  (0),
   m_rxBytes  (0),
   m_rxLast   (0),
   m_packDrop (0),
   m_rssiDrop (0)
{
   gettime (&m_time);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   \brief  The constructor to initialize to a set of volatile statistics

   \param[in] stats  The volatile statistics to copy
                                                                          */
/* ---------------------------------------------------------------------- */
inline Receiver::Statistics::Statistics (Receiver::Statistics volatile &stats) :
   m_rxCount  (stats.m_rxCount),
   m_rxBytes  (stats.m_rxBytes),
   m_rxLast   (stats.m_rxLast),
   m_packDrop (stats.m_packDrop),
   m_rssiDrop (stats.m_rssiDrop)
{
   m_time = const_cast<const struct timespec &>(stats.m_time);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Get the current monotonic increasing time

  \param[out]  ts  The returned struct timespec
                                                                          */
/* ---------------------------------------------------------------------- */
inline void Receiver::Statistics::gettime (struct timespec *ts)
{
   clock_gettime (CLOCK_MONOTONIC_COARSE, ts);
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Get the current monotonic increasing time

  \param[out]  ts  The returned struct timespec

  \note
   The twist here is that the output timespec is volatile.  This just
   hides the ugly cast.
                                                                          */
/* ---------------------------------------------------------------------- */
inline void Receiver::Statistics::gettime (struct timespec volatile *ts)
{
   clock_gettime (CLOCK_MONOTONIC_COARSE, 
                  const_cast<struct timespec *>(ts));
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief  Get the difference of two timespecs
  \return The difference in nanoseconds

  \param[in]  start The starting time
  \param[in]   stop The stopping time
                                                                          */
/* ---------------------------------------------------------------------- */
inline int64_t Receiver::Statistics::subtime (struct timespec const *start,
                                              struct timespec const  *stop)
{
   int32_t diff_ns = stop->tv_nsec - start->tv_nsec;
   int64_t elapsed = (stop->tv_sec - start->tv_sec);
   
   elapsed *= 1e9;
   elapsed += diff_ns;
   
   //printf ("Diff = %" PRIu64 " % "PRId32 "\n", elapsed, diff_ns);
   
   return elapsed;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Puts of the statistics/status title line
                                                                          */
/* ---------------------------------------------------------------------- */
static inline void print_statistics_title ()
{
   puts (
   " Sample   Rate    Gbps Opened DropRssi DropPack  Count Rx  Last   Rx Total\n"
   " ------ ------ ------- ------ -------- -------- ------ -------- ----------");

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
static void print_statistics (Receiver::Statistics const *cur,
                              Receiver::Statistics const *prv,
                              bool                     opened,
                              char                        eol)
{
   static int     Count = 0;
   static bool LastOpen = false;
   //struct timespec  diff;

   // ---------------------------------------------
   // Get the time since the last set of statistics
   // ---------------------------------------------
   //timespec_diff (&prv->m_time, &cur->m_time, &diff);

   double     elapsed = Receiver::Statistics::subtime (&prv->m_time, &cur->m_time);
   uint32_t       cnt = cur->m_rxCount - prv->m_rxCount;
   uint64_t diffBytes = cur->m_rxBytes - prv->m_rxBytes;
   uint32_t  curBytes = cur->m_rxLast;
   uint64_t  totBytes = cur->m_rxBytes;
   double        gbps = (((float)(diffBytes * 8)) / elapsed);
   float         rate =   ((float)cnt / elapsed) * 1e9;

  
   // If no change, only update the count
   if (LastOpen == opened && diffBytes == 0) 
   {
      printf (" %6u\r", Count++);
      fflush (stdout);
   }
   else
   {                  
      //         cnt  rate Gbps open drop rssi    drop pack  count         last         total
      printf (" %6u %6.3f %7.3f %6d  %8" PRIu32 " %8" PRIu32 "%6" PRIu32 " %8" PRIu32 " %10" PRIu64 "%c",
              Count++,
              rate,
              gbps, 
           opened, 
              cur->m_rssiDrop, 
              cur->m_packDrop,
              cur->m_rxCount,
              curBytes, 
              totBytes,
              eol);
   }


   LastOpen = opened;

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief The RssiConnnection constructor

  \param[in]      ip The IP address as the usual dotted string, 
                     \e e.g. 192.168.2.110
  \param[in] nframes The number of jumbo frames to allocate for receiving.
                                                                          */
/* ---------------------------------------------------------------------- */
RssiConnection::RssiConnection (char const *ip, int nframes)
{
   static const uint16_t UdpPort = 8192;


   // Create the UDP client, jumbo = true
   m_udp  = rogue::protocols::udp::Client::create(ip, UdpPort, true);
   

   // Make enough room for 'nframes' outstanding buffers
   m_udp->setRxBufferCount (nframes); 


   // RSSI
   m_rssi = rogue::protocols::rssi::Client::create(m_udp->maxPayload());


   // Packetizer, ibCrc = false, obCrc = true
   ///m_pack = rogue::protocols::packetizer::CoreV2::create(false,true);
   m_pack = rogue::protocols::packetizer::Core::create();


   // Connect the RSSI engine to the UDP client
   m_udp ->setSlave  (m_rssi->transport ());
   m_rssi->transport ()->setSlave   (m_udp);


   // Connect the RSSI engine to the packetizer
   m_rssi->application ()->setSlave (m_pack->transport());
   m_pack->transport   ()->setSlave (m_rssi->application());

   m_rssi->start();

   return;
}
/* ---------------------------------------------------------------------- */
   



/* ---------------------------------------------------------------------- *//*!

  \brief  Connect a receiver to the RssiConnection

  \param[in] receiver The RSSI receiver.  This must be a subclass of
                      rogue::interfaces::stream::Slave 
                                                                          */
/* ---------------------------------------------------------------------- */
void RssiConnection::connect (boost::shared_ptr<Receiver> receiver)
{
   m_pack->application(0)->setSlave (receiver);
   return;
}
/* ---------------------------------------------------------------------- */




