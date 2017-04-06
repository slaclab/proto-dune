/* ---------------------------------------------------------------------- *//*!

   \file  BTD.c
   \brief Binary Tree Encoding routines
   \author JJRussell - russell@slac.stanford.edu

    Routines to encode bit strings using a binary tree

\verbatim
    CVS $Id
\endverbatim
                                                                         */
/* --------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\
 *
 * HISTORY
 * -------
 *
 * DATE     WHO WHAT
 * -------- --- ---------------------------------------------------------
 * 08.17.10 jjr Eliminated local copy of FFS.ih in favor of PBI version
 *
\* ---------------------------------------------------------------------- */


#include "BTD.h"


#ifndef CMX_DOXYGEN
#define LKUP(_0, _10, _11)  (_10 << 0) | (_11 << 2) | (_0 << 4)
#endif


/* --------------------------------------------------------------------- *//*!

  \fn     unsigned int BTD_wordDecode  (unsigned int      w,
                                        unsigned int scheme,
                                        int          *nbits)
 \brief   Decodes the bits in \a w according to \a scheme
 \return  The decoded word

 \param      w   The encoded bits
 \param scheme   The encoding scheme
 \param  nbits   Returns the number of bits decoded.
                                                                         */
/* --------------------------------------------------------------------- */
extern unsigned int BTD_wordDecode  (unsigned int w,
                                     unsigned int scheme,
                                     int         *nbits)
{
   int             n;
   int             b;
   int          node;
   int         node0;
   int          bits;
   unsigned int   m1;
   unsigned int    o;
   unsigned int lkup;


   /*  If the encoding scheme is 0, just return the 32 bits */
   if (scheme == 0) { *nbits = 32; return w; }



   /*
    |
    |  Algorithm is to first expand the bits in w according to the
    |  indicated encoding scheme.
    |
    |     Scheme     0    10   11
    |          1    01    10   11
    |          2    10    01   11
    |          3    11    01   10
    |
    |  This is done by indexing an 'array' stored in an integer mask
    |  Each element is 2 bits wide
    |
    |  Bits 0,1 = pattern for 10 code
    |  Bits 2,3 = pattern for 11 code
    |  Bits 4,5 = pattern for  0 code.
    |
    |  The first symbol extracted gives the binary tree code for the halfwords
    |
    |  One should set the output word to
    |
    |         01 => 00000000 00000000 11111111 11111111
    |         10 => 11111111 11111111 00000000 00000000
    |         11 => 11111111 11111111 11111111 11111111
    |
    |  The next symbol gives the binary tree code for the next 2 bytes.
    |  One should scan the word, looking for the first set bit, this
    |  is the top bit of the byte to be inserted. Lets take the 01
    |  case as an example
    |
    |  t = o;
    |  b = FFSL (t);   // b = 16;
    |
    |  If the symbol is
    |
    |        01 => 00000000 00000000 00000000 11111111
    |        10 => 00000000 00000000 11111111 00000000
    |        11 => 00000000 00000000 11111111 11111111
    |
    |  One should now eliminated the next 16  bits in the pattern.
    |
    |  t &= ~0xffff0000 >> b;  // t = 0;
    |  if (t != 0) extract next symbol
    |
    |
    |
   */
   if      (scheme == 1)  lkup = LKUP (1, 2, 3);
   else if (scheme == 2)  lkup = LKUP (2, 1, 3);
   else                   lkup = LKUP (3, 1, 2);


   /* Get the node value when the extract symbol = 0 */
   node0 = lkup >> 4;


   /*
    | Initialize
    |   The number of bits consumed from the input word  to 0
    |   The output word to all bits set.
    |
    | As the values a each node are discovered, the bit fields corresponding
    | to the width at the level being processed are cleared. After all the
    | levels are processed, the bits left standing represent the value of
    | decoded word;
   */
   n = 0;
   o = 0xffffffff;


   m1   = 0xffffffff; /* Bits controlled by the next node                  */
   bits = 16;         /* Next level controls fate of 16 bits at each node  */

   do
   {
       unsigned int   m;
       unsigned int tmp;


       /* Mask representing the number of bits at each node */
       m  = m1;
       m1 = m & (m << bits);


       tmp = o;
       do
       {

           /* Find the next set of bits to be dealt with */
           b  = __builtin_clz (tmp);


           /* Get the value at this node. */
           if ((signed int)w >= 0)
           {
               /* Next bit is a 0, value is the node0 value */
               node = node0;
               n   += 1;
               w  <<= 1;

           }
           else
           {
               /*
                | Next bit is a 1, use the value of the next bit to
                | find the value at this node. The index bit is in
                | bit position (little endian counting) 30. So shift
                | it down 30 to get it in the LSb, then shift it
                | up by 1, (multiple by 2) to get an index. This looks
                | like at lot of code, but it should reduce to two
                | rwlimn instructions.
                |
               */
               node = (lkup >> ((w >> (30 - 1)) & 2) & 3);
               n   += 2;
               w  <<=2;
           }

           /*
            | Eliminate the bits in the output mask associated with
            | a controlling bit of 0.
           */
           if ( (node & 1) == 0) o &= ~(m1 >> (b + bits));
           if ( (node & 2) == 0) o &= ~(m1 >>  b);


           /* Eliminate these bits from consideration */
           tmp &= ~m >> b;

           /* If no more bits at this level, continue to the next level */
       }
       while (tmp);

   }
   while (bits >>= 1);


   *nbits = n;
   return o;
}
/* --------------------------------------------------------------------- */




/* --------------------------------------------------------------------- *//*!

  \fn     unsigned int BTD_shortDecode  (unsigned short int s,
                                         unsigned int  scheme,
                                         int           *nbits)
 \brief   Decodes the bits in \a w according to \a scheme
 \return  The decoded short word

 \param      s   The encoded bits
 \param scheme   The encoding scheme
 \param  nbits   Returns the number of bits decoded.
                                                                         */
/* --------------------------------------------------------------------- */
extern unsigned int BTD_shortDecode  (unsigned short int s,
                                      unsigned int  scheme,
                                      int           *nbits)
{
    
   int             n;
   int             b;
   int          node;
   int         node0;
   int          bits;
   unsigned int    w;
   unsigned int   m1;
   unsigned int    o;
   unsigned int lkup;


   /*  If the encoding scheme is 0, just return the 16 bits */
   if (scheme == 0) { *nbits = 16; return s; }

   /* Left justify the pattern */
   w = s << 16;

   /*
    |
    |  Algorithm is to first expand the bits in w according to the
    |  indicated encoding scheme.
    |
    |     Scheme     0    10   11
    |          1    01    10   11
    |          2    10    01   11
    |          3    11    01   10
    |
    |  This is done by indexing an 'array' stored in an integer mask
    |  Each element is 2 bits wide
    |
    |  Bits 0,1 = pattern for 10 code
    |  Bits 2,3 = pattern for 11 code
    |  Bits 4,5 = pattern for  0 code.
    |
    |  The first symbol extracted gives the binary tree code for the bytes
    |
    |  One should set the output word to
    |
    |         01 => 00000000 11111111 
    |         10 => 11111111 00000000
    |         11 => 11111111 11111111
    |
    |  The next symbol gives the binary tree code for the next 2 nibbles
    |  One should scan the word, looking for the first set bit, this
    |  is the top bit of the byte to be inserted. Lets take the 01
    |  case as an example
    |
    |  t = o;
    |  b = FFSL (t);   // b = 16;
    |
    |  If the symbol is
    |
    |        01 => 00000000 11111111
    |        10 => 11111111 00000000
    |        11 => 11111111 11111111
    |
    |  One should now eliminated the next 8 bits in the pattern.
    |
    |  t &= ~0xff000000 >> b;  // t = 0;
    |  if (t != 0) extract next symbol
    |
    |
    |
   */
   if      (scheme == 1)  lkup = LKUP (1, 2, 3);
   else if (scheme == 2)  lkup = LKUP (2, 1, 3);
   else                   lkup = LKUP (3, 1, 2);


   /* Get the node value when the extract symbol = 0 */
   node0 = lkup >> 4;


   /*
    | Initialize
    |   The number of bits consumed from the input word  to 0
    |   The output word to all bits set.
    |
    | As the values a each node are discovered, the bit fields corresponding
    | to the width at the level being processed are cleared. After all the
    | levels are processed, the bits left standing represent the value of
    | decoded word;
   */
   n = 0;
   o = 0xffff0000;


   m1   = 0xffff0000; /* Bits controlled by the next node                  */
   bits = 8;          /* Next level controls fate of 8 bits at each node  */

   do
   {
       unsigned int   m;
       unsigned int tmp;


       /* Mask representing the number of bits at each node */
       m  = m1;
       m1 = m & (m << bits);


       tmp = o;
       do
       {

           /* Find the next set of bits to be dealt with */
           b  = __builtin_clz (tmp);


           /* Get the value at this node. */
           if ((signed int)w >= 0)
           {
               /* Next bit is a 0, value is the node0 value */
               node = node0;
               n   += 1;
               w  <<= 1;

           }
           else
           {
               /*
                | Next bit is a 1, use the value of the next bit to
                | find the value at this node. The index bit is in
                | bit position (little endian counting) 30. So shift
                | it down 30 to get it in the LSb, then shift it
                | up by 1, (multiple by 2) to get an index. This looks
                | like at lot of code, but it should reduce to two
                | rwlimn instructions.
                |
               */
               node = (lkup >> ((w >> (30 - 1)) & 2) & 3);
               n   += 2;
               w  <<=2;
           }

           /*
            | Eliminate the bits in the output mask associated with
            | a controlling bit of 0.
           */
           if ( (node & 1) == 0) o &= ~(m1 >> (b + bits));
           if ( (node & 2) == 0) o &= ~(m1 >>  b);


           /* Eliminate these bits from consideration */
           tmp &= ~m >> b;

           /* If no more bits at this level, continue to the next level */
       }
       while (tmp);

   }
   while (bits >>= 1);


   *nbits = n;
   return o >> 16;
}
/* --------------------------------------------------------------------- */
