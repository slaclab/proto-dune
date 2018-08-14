
#ifndef _TPC_PRINTER_H_
#define _TPC_PRINTER_H_

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

static void print_record     (uint64_t const *data, 
                              size_t         ndata);

static void print_hdr        (uint64_t      header);
static void print_header1    (char const      *dsc,
                              uint64_t         hdr);
static void print_header2    (char const      *dsc,
                              uint32_t         hdr);
static void print_id         (uint64_t const *data);
static void print_origin     (uint64_t const *data);

static void print_tpcRecords (uint64_t const *data, 
                              int             itpc);
static void print_range      (uint64_t const *data);
static void print_toc        (uint64_t const *data);


/* ---------------------------------------------------------------------- */
static void print_record (uint64_t const *data, size_t ndata)
{
   uint64_t const     *origin = (uint64_t const *)(data) + 2; 
   int              n64origin = (origin[0] >> 8) & 0xfff;
   print_origin (origin);
   
   uint64_t const *pTrailer   = data 
                              + ndata 
                              - sizeof (*pTrailer) / sizeof (*data);
   uint64_t const *tpcRecords = (origin + n64origin);

   int ntpc = 0;
   while (tpcRecords < pTrailer)
   {
      if (ntpc > 1) 
      {
         printf ("ERROR: too many tpc records\n");
         break;
      }

      /*
        printf ("Found tpc record @ %p %p \n"
        " %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 "\n", 
        tpcRecords, pTrailer,
        tpcRecords[-1], tpcRecords[0], tpcRecords[1]);
      */
      
      int ntpc64 = (tpcRecords[0] >> 8) & 0xffffff;
      print_tpcRecords (tpcRecords, ntpc);
      tpcRecords += ntpc64;
      ntpc += 1;
   }

   return;
}
/* ---------------------------------------------------------------------- */
             


/* ---------------------------------------------------------------------- *//*!

  \brief Prints the frame header

  \param[in]  data   The header data
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_hdr (uint64_t header)
{
   unsigned format    = (header >>  0) & 0xf;
   unsigned type      = (header >>  4) & 0xf;
   unsigned length    = (header >>  8) & 0xffffff;
   unsigned naux64    = (header >> 32) & 0xf;
   unsigned subtype   = (header >> 36) & 0xf;
   unsigned specific  = (header >> 40) & 0xffffff;


   printf ("Header:     Type.Format = %1.1x.%1.1x length = %6.6x\n"
           "            naux64      = %1.1x      subtype = %1.1x  spec = %6.6x\n",
           type,
           format,
           length,
           naux64,
           subtype,
           specific);

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \brief Prints the Identifier Block

  \param[in]  data The data for the identifier block
                                                                          */
/* ---------------------------------------------------------------------- */
static void  print_id (uint64_t const *data)
{
   uint64_t       w64 = data[0];
   uint64_t timestamp = data[1];
   printf ("Identifier: %16.16" PRIx64 " %16.16" PRIx64 "\n",
           w64, timestamp);

   unsigned format   = (w64 >>  0) & 0xf;
   unsigned type     = (w64 >>  4) & 0xf;
   unsigned src0     = (w64 >>  8) & 0xfff;
   unsigned src1     = (w64 >> 20) & 0xfff;
   unsigned sequence = (w64 >> 32) & 0xffffffff;

   unsigned c0 = (src0 >> 3) & 0x1f;
   unsigned s0 = (src0 >> 8) & 0x7;
   unsigned f0 = (src0 >> 0) & 0x7;

   unsigned c1 = (src1 >> 3) & 0x1f;
   unsigned s1 = (src1 >> 8) & 0x7;
   unsigned f1 = (src1 >> 0) & 0x7;


   printf ("            Format.Type = %1.1x.%1.1x Srcs = %1x.%1x.%1x : %1x.%1x.%1x\n"
           "            Timestamp   = %16.16" PRIx64 " Sequence = %8.8" PRIx32 "\n",
           format, type, c0, s0, f0, c1, s1, f1,
           timestamp, sequence);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Prints the field format = 1 header

  \param[in]  hdr  The format = 1 header
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_header1 (char const *dsc, uint64_t hdr)
{
   unsigned int  format = (hdr >>  0) &        0xf;
   unsigned int    type = (hdr >>  4) &        0xf;
   unsigned int  length = (hdr >>  8) &  0xfffffff;
   uint32_t      bridge = (hdr >> 32) & 0xffffffff;
   
   printf ("%-10.10s: Type.Format = %1.1x.%1.1x Length = %6.6x Bridge = %8.8" PRIx32 "\n",
           dsc,
           type,
           format,
           length,
           bridge);

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief Prints the field format = 2 header

  \param[in]  hdr  The format = 2 header
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_header2 (char const *dsc, uint32_t hdr)
{
   unsigned int   format = (hdr >>  0) &   0xf;
   unsigned int     type = (hdr >>  4) &   0xf;
   unsigned int  version = (hdr >>  8) &   0xf;
   unsigned int specific = (hdr >> 12) &  0xff;
   unsigned int   length = (hdr >> 20) & 0xfff;
   
   printf ("%-10.10s: Type.Format = %1.1x.%1.1x Version = %1.1x Spec = %2.2x"
           " Length = %6.6x\n",
           dsc,
           type,
           format,
           version,
           specific,
           length);

   return;
}
/* ---------------------------------------------------------------------- */



static void print_origin (uint64_t const *data)
{
   uint32_t hdr = data[0];

   print_header2 ("Origin", hdr);

   uint32_t      location = data[0] >> 32;
   uint64_t  serialNumber = data[1];
   uint64_t      versions = data[2];
   uint32_t      software = versions >> 32;
   uint32_t      firmware = versions;
   char const   *rptSwTag = (char const *)(data + 3);
   char const  *groupName = rptSwTag + strlen (rptSwTag) + 1;

   unsigned          slot = (location >> 16) & 0xff;
   unsigned           bay = (location >>  8) & 0xff;
   unsigned       element = (location >>  0) & 0xff;

   uint8_t          major = (software >> 24) & 0xff;
   uint8_t          minor = (software >> 16) & 0xff;
   uint8_t          patch = (software >>  8) & 0xff;
   uint8_t        release = (software >>  0) & 0xff;

   printf ("            Software    ="
           " %2.2" PRIx8 ".%2.2" PRIx8 ".%2.2" PRIx8 ".%2.2" PRIx8 ""
           " Firmware     = %8.8" PRIx32 "\n"
           "            RptTag      = %s\n"
           "            Serial #    = %16.16" PRIx64 "\n"
           "            Location    = %s/%u/%u/%u\n",
           major, minor, patch, release,
           firmware,
           rptSwTag,
           serialNumber,
           groupName, slot, bay, element);

   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

   \brief Prints the packets header

   \param[in] data  The begining of the packets record
   \param[in] itpc  Which tpc record
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_tpcRecords (uint64_t const *data, int itpc)
{
   char title[20];
   int   n  = sprintf (title, "TpcRec [%1d]", itpc);
   title[n] = 0;
   print_header1 (title, data[0]);

   uint64_t const *range = (data + 1);
   uint32_t       bridge = (data[0] >> 32) & 0xffffffff;

   unsigned int  left = (bridge >> 16) & 0xf;
   unsigned int   csf = (bridge >>  4) & 0x7ff;
   unsigned int crate = (csf >> 6) & 0x1f;
   unsigned int  slot = (csf >> 3) & 0x07;
   unsigned int fiber = (csf >> 0) & 0x07;
   printf ("Tcp Stream: Crate.Slot.Fiber %2u.%1u.%1u (%3.3x) Remaining = %u\n",
           crate, slot, fiber, csf, left);


   print_range (range);

   int         rngSize = (range[0] >> 8) & 0xfff;
   uint64_t const *toc = (range + rngSize);
   print_toc    (toc);

   
   uint32_t  tocHdr = *(uint32_t const *)(toc);
   unsigned  n64toc = (tocHdr >>  8) & 0xfff;


   int                npkts = (tocHdr >> 24) & 0xff;
   uint32_t const *ppktDscs = (uint32_t const *)toc + 1;
   uint64_t const *ppktsHdr = (uint64_t const *)toc + n64toc;
   uint64_t const    *ppkts =  ppktsHdr + 1;



   puts ("Pkts      : # Typ.Fmt Offset "
         "Wib[0]           Wib[1]           Wib[2]");

   // -----------------------------------------------------
   // Loop over the packet descriptors for this contributor
   // -----------------------------------------------------
   for (int ipkt = 0; ipkt < npkts;  ipkt++)
   {
      uint32_t pktDsc = *ppktDscs++;
      unsigned format = (pktDsc >> 0) &      0xf;
      unsigned type   = (pktDsc >> 4) &      0xf;
      unsigned opkt   = (pktDsc >> 8) & 0xffffff;
      

      // -----------------------------
      // Dump the first 4 64 bit words
      // -----------------------------
      uint64_t const *ppkt = ppkts + opkt;
      printf ("           %2u   %1x.%1x   %6.6x"
              " %16.16" PRIx64 " %16.16" PRIx64 " %16.16" PRIx64 "\n",
              ipkt, type, format, opkt,
              ppkt[0], ppkt[1], ppkt[2]);
   }

   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
static void print_range (uint64_t const *data)
{
   uint32_t const     *p32 = ((uint32_t const *)data);
   uint32_t            hdr = *p32++;

   unsigned format  = (hdr >>  0) &   0xf;
   unsigned type    = (hdr >>  4) &   0xf;
   unsigned size    = (hdr >>  8) & 0xfff;
   unsigned version = (hdr >> 20) &   0xf;

   printf ("Range     : "
   "Format.Type.Version = %1.1x.%1.1x.%1.1x size %4.4x %8.8"
   "" PRIx32 "\n",
           format,
           type,
           version,
           size,
           hdr);

   // --------------------
   // Untrimmed timestamps
   // --------------------
   uint32_t     idxBeg = *p32++;
   uint32_t     idxEnd = *p32++;
   uint32_t     idxTrg = *p32++;
   printf ("            Idx    Beg: %14.8" PRIx32 " End: %14.8" PRIx32 " "
           "Trg: %8.8" PRIx32 "\n",
           idxBeg, idxEnd, idxTrg);


   // ------------------
   // Trimmed timestamps
   // ------------------
   uint64_t const *p64 = (uint64_t const *)p32;
   uint64_t     pktBeg = *p64++;
   uint64_t     pktEnd = *p64++;
   printf ("            Packet Beg: %14.14" PRIx64 " End: %14.14" PRIx64 "\n",
           pktBeg, pktEnd);

   // ------------
   // Event Window
   // ------------
   uint64_t winBeg = *p64++;
   uint64_t winEnd = *p64++;
   uint64_t winTrg = *p64++;
   printf ("            Window Beg: %14.14" PRIx64 " End: %14.14" PRIx64 " "
           "Trg: %14.14" PRIx64 "\n",
           winBeg, winEnd, winTrg);


   return;
}
/* ---------------------------------------------------------------------- */
   




/* ---------------------------------------------------------------------- *//*!

  \brief Prints the table of contents

  \param[in]  data  The table of contents data
                                                                          */
/* ---------------------------------------------------------------------- */
static void print_toc (uint64_t const *data)
{
   uint32_t            hdr = data[0];
   uint32_t const *ctb_ptr = ((uint32_t const *)data) + 1;

   unsigned format    = (hdr >>  0) &   0xf;
   unsigned type      = (hdr >>  4) &   0xf;
   unsigned size      = (hdr >>  8) & 0xfff;
   unsigned tocFormat = (hdr >> 20) &   0xf;
   unsigned npkts     = (hdr >> 24) &  0xff;


   uint32_t const *pkt_ptr = ctb_ptr;

   printf ("Toc       : "
   "Format.Type.Version = %1.1x.%1.1x.%1.1x npkts = %d size %4.4x %8.8"
   "" PRIx32 "\n",
           format,
           type,
           tocFormat,
           npkts,
           size,
           hdr);


   // -------------------------------------------------------
   // Look ahead to the first packet of the first contributor
   // -------------------------------------------------------
   uint32_t pkt = *pkt_ptr++;




   // -----------------------------
   // Iterate over the contributors
   // -----------------------------
   //for (int ictb = 0; ictb < nctbs; ictb++)
   //{
   //   unsigned   o32 = (ctb >> 20) & 0xfff;
   //   unsigned fiber = (csf >>  0) &   0x7;
   //   unsigned  slot = (csf >>  3) &   0x7;
   //   unsigned crate = (csf >>  6) &  0x1f;

   unsigned int o64 = 0;
   ///printf ("            Ctb] Id: %2d.%2d.%2d npkts: %3d\n",
   //        crate, slot, fiber,
   ///        npkts);


   // --------------------------------------------
   // Iterate over the packets in this contributor
   // --------------------------------------------
   for (unsigned ipkt = 0; ; ipkt++)
   {
      unsigned   format = (pkt >> 0) & 0xf;
      unsigned     type = (pkt >> 4) & 0xf;
      unsigned offset64 = o64;


      // ----------------------------------------------------------
      // Due the lookahead to the next packet to determine the size
      // ----------------------------------------------------------
      pkt  = *pkt_ptr++;
      o64  = (pkt >> 8) & 0xffffff;


      // -----------------------------------------------
      // Check for and format the last packet descriptor
      // -----------------------------------------------
      if (ipkt == npkts)
      {
         printf ("           %2d. %1.x.%1.1x %6.6x\n",
                 npkts,
                 format,
                 type,
                 offset64);
         break;
      }


      // -------------------------
      // Just a regular descriptor
      // -------------------------
      unsigned int n64 = o64 - offset64;
      printf ("           %2d. %1.x.%1.1x %6.6x %6.6x %8.8" PRIx32 "\n",
              ipkt, 
              format,
              type,
              offset64,
              n64,
              pkt);
   }


   return;
}
/* ---------------------------------------------------------------------- */

#endif
