// -*-Mode: C;-*-

/* ---------------------------------------------------------------------- *//*!
 *
 *  @file     rssi_sink.cpp
 *  @brief    Primitive RSSI receiver for data from the RCEs.  This is
 *            a very small modification of Ryan's original implementation.
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

#include <cinttypes>

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


/* ---------------------------------------------------------------------- *//*!

   \brief Class to parse and capture the command line parameters
                                                                          */
/* ---------------------------------------------------------------------- */
class Parameters
{
public:
   Parameters (int argc, char *const argv[]);


public:
   char       const *getIp   () const { return        m_ip; }
   int          getNframes   () const { return   m_nframes; }
   int          getCopyFlag  () const { return      m_copy; }
   int          getNdump     () const { return     m_ndump; }
   char const  *getOfilename () const { return m_ofilename; }
   int          getRefresh   () const { return   m_refresh; }
   void         echo         () const;
   static void reportUsage   ();

public:
   char  const        *m_ip; /*!< The RSSI source IP -mandatory           */
   char  const *m_ofilename; /*!< Output file -optional                   */
   int            m_nframes; /*!< Number of input frames-optional         */
   int              m_ndump; /*!< Number of 64-bit words to dump -optional*/
   bool              m_copy; /*!< Copy the input data -optional           */
   int            m_refresh; /*!< Refresh rate -default = 1 second        */
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Parse out the command line parameters
  
  \param[in] argc  The  count of command line parameters
  \param[in] argv  Teh vector of command line parameters

  \par
   This class exits if the command line parameters are ill-specified
                                                                          */
/* ---------------------------------------------------------------------- */
Parameters::Parameters (int argc, char *const argv[])
{
   int c;

   // Default the number of frames
   m_nframes   = 64;
   m_copy      = false;
   m_ofilename = NULL;
   m_ndump     = 0;
   m_refresh   = 1;

   while ( (c = getopt (argc, argv, "ca:n:o:r:")) != EOF)
   {
      if      (c == 'n') { m_nframes = strtol (optarg, NULL, 0);   }
      else if (c == 'a') { m_copy    = true;
                           m_ndump   = strtol (optarg, NULL, 0);   }
      else if (c == 'c') { m_copy    = true;                       }
      else if (c == 'o') { m_copy    = true; m_ofilename = optarg; }
      else if (c == 'r') { m_refresh = strtol (optarg, NULL, 0);   }
   }


   if (optind < argc)
   {
      m_ip = argv[optind];
   }
   else
   {
      reportUsage ();
      exit (-1);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

   brief Echo the input parameters
                                                                          */
/* ---------------------------------------------------------------------- */
void Parameters::echo () const
{
   std::cout << "Starting  on IP: " << m_ip      << std::endl
             << " refresh   rate: " << m_refresh << std::endl;

   if (m_nframes)
   {
      std::cout << " receive frames: " << m_nframes  << std::endl;
   }


   if (m_copy)
   {
      std::cout << " copy          : " << (m_copy ? "yes" : "no") <<  std::endl;
   }


   if (m_ndump)
   {
      std::cout << " dump          : " << m_ndump << std::endl;
   }


   if (m_ofilename)
   {
      std::cout << " output file   : " << m_ofilename << std::endl;
   }


   std::cout << std::endl;

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
<< "Usage:" << std::endl
<< "$ rssi_sink [a:cn:o:r:] ip" << endl
<< "  where:"           << endl
<< "     ip:  The ip address of data source"                                << endl
<< "      a:  Number of hex words to dump (debugging aid)"                  << endl
<< "      n:  The number of incoming frames to buffer, default = 64"        << endl
<< "      c:  If present, the frame data is copied into a temporary buffer" << endl
<< "      o:  If present, then name of an output file"                      << endl
<< "      r:  The display refresh rate in seconds (default = 1 second)"     << endl
<< endl
<< " Example:" << endl
<< " $ rssi_sink -r 2 -n 100 -a 32 -o/tmp/dump.dat 192.168.2.110" << endl
<< endl
<< " Sets display refresh rate to 2 seconds"    << endl
<< " Allocates  100 frames"                     << endl
<< " Dumps the first 32 64-bit words"           << endl
<< " Writes the binary output to /tmp/dump.dat" << endl
<< " Sets the source IP to 192.168.2.110"       << endl;

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


static void dump (uint64_t const  *d,
                  int              n);

//! Receive slave data, count frames and total bytes for example purposes.
class TestSink : public rogue::interfaces::stream::Slave {


   public:
      uint32_t rxCount;
      uint64_t rxBytes;
      uint32_t  rxLast;
      int        ndump;
      bool        copy;
      int           fd;

   TestSink(bool copyFlag = false, int ndump = 0, int outputFd = -1) {
         rxCount = 0;
         rxBytes = 0;
         rxLast  = 0;
         ndump   = 0;
         copy    = copyFlag;
         fd      = outputFd;
   }

   void acceptFrame ( boost::shared_ptr<rogue::interfaces::stream::Frame> frame ) {
      rxLast   = frame->getPayload();
      rxBytes += rxLast;
      rxCount++;
      
      auto err = frame->getError ();
      if (err)
      {
         std::cout << "Frame error: " << err << std::endl;
      }
      
      //std::cout << "Got:" << rxLast << " bytes" << std::endl;

      
      // Copy to buffer
      if (copy)
      {
         // std::cout << "Copying nbytes: " << rxLast << std::endl;

         // iterator to start of buffer
         rogue::interfaces::stream::Frame::iterator iter = frame->beginRead();
         rogue::interfaces::stream::Frame::iterator  end = frame->endRead  ();


         uint8_t  *buff = (uint8_t *)malloc(frame->getPayload());
         uint8_t   *dst = buff;


         //Iterate through contigous buffers
         while ( iter != end ) {
            rogue::interfaces::stream::Frame::iterator nxt = iter.endBuffer();
            auto size = iter.remBuffer ();
            auto *src = iter.ptr       ();
            memcpy(dst, src, size);
            dst += size;
            iter = nxt;
         }


         if (fd >= 0)
         {
            ssize_t nwrote = write (fd, buff, rxLast);
            if (nwrote != rxLast)
            {
               fprintf (stderr, "Error %d writing the output file\n", errno);
               exit (errno);
            }
         }


         if (ndump)
         {
            dump ((uint64_t const *)buff, ndump);
         }
         
         free(buff);
      }
   }
};


int main (int argc, char **argv) {

   Parameters prms (argc, argv);

   struct timeval last;
   struct timeval curr;
   struct timeval diff;
   double   timeDiff;
   uint64_t lastBytes;
   uint64_t diffBytes;

   double bw;

   char const   *daqHost = prms.getIp       (); ///"192.168.2.110";
   int           nframes = prms.getNframes  ();
   bool         copyFlag = prms.getCopyFlag ();
   char const *ofilename = prms.getOfilename();
   int             ndump = prms.getNdump    ();
   int           refresh = prms.getRefresh  ();


   prms.echo ();
   
   int fd = ofilename ? create_file (ofilename) : -1;
    

   rogue::Logging::setLevel(rogue::Logging::Info);

   // Create the UDP client, jumbo = true
   rogue::protocols::udp::ClientPtr 
          udp  = rogue::protocols::udp::Client::create(daqHost, 8192, true);
   
   // Make enough room for 'nframes' outstanding buffers
   udp->setRxBufferCount (nframes); 

   // RSSI
   rogue::protocols::rssi::ClientPtr 
          rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());

   // Packetizer, ibCrc = false, obCrc = true
   ////rogue::protocols::packetizer::CoreV2Ptr 
   ////       pack = rogue::protocols::packetizer::CoreV2::create(false,true);
   rogue::protocols::packetizer::CorePtr 
          pack = rogue::protocols::packetizer::Core::create();

   // Connect the RSSI engine to the UDP client
   udp->setSlave(rssi->transport());
   rssi->transport()->setSlave(udp);

   // Connect the RSSI engine to the packetizer
   rssi->application()->setSlave(pack->transport());
   pack->transport()->setSlave(rssi->application());

   // Create a test sink and connect to channel 1 of the packetizer
   boost::shared_ptr<TestSink> 
      sink = boost::make_shared<TestSink>(copyFlag, ndump, fd);
   pack->application(0)->setSlave(sink);

   // Loop forever showing counts
   lastBytes = 0;
   gettimeofday(&last,NULL);

   int count = 0;
   while(1) 
   {
      sleep (refresh);
      gettimeofday (&curr, NULL);

      timersub (&curr,&last,&diff);

      diffBytes = sink->rxBytes - lastBytes;
      lastBytes = sink->rxBytes;

      timeDiff = (double)diff.tv_sec + ((double)diff.tv_usec / 1e6);
      bw = (((float)diffBytes * 8.0) / timeDiff) / 1e9;

      gettimeofday(&last,NULL);

      char eol = diffBytes ? '\n' : '\r';

      printf("%6u RSSI = %i. RxLast=%i, RxCount=%i, RxTotal=%li, Bw=%f, DropRssi=%i, DropPack=%i%c",
             count++,
             rssi->getOpen(),
             sink->rxLast,
             sink->rxCount,
             sink->rxBytes,
             bw,
             rssi->getDropCount(),
             pack->getDropCount(),
             eol);

      fflush (stdout);
   }
}


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
