/* ---------------------------------------------------------------------- *//*!
   
   \file  BTE.c
   \brief Binary Tree Encoding routines
   \author JJRussell - russell@slac.stanford.edu

    Routines to encode bit strings using a binary tree

\verbatim
    CVS $Id

\endverbatim

    EXAMPLE
    The following shard of code shows how one ties these routines together
    to encode a 32 bit word, \e word.

   \code

     
     unsigned int pattern     = BTE_wordPrepare (word);
     unsigned int scheme_size = BTE_wordSize    (word, pattern);
     unsigned int scheme      = scheme_size >> 16;
     unsigned int encoded     = BTE_wordEncode  (word, pattern, scheme);

  \endcode

                                                                         */
/* --------------------------------------------------------------------- */


#include <stdio.h>
#include "BTE.h"

#ifndef CMX_DOXYGEN

#ifdef BTE_DEBUG
#define PRINTF(args)  printf args
#else
#define PRINTF(args)
#endif

#endif


static __inline unsigned int encode (unsigned int e,
				     unsigned int w,
				     unsigned int p1,
				     unsigned int p3);

/* --------------------------------------------------------------------- *//*!

  \fn    unsigned int encode (unsigned int e,
                              unsigned int w,
                              unsigned int p1,
                              unsigned int p3)
  \brief Encodes the input word \a w into the output word \a e.

  \param e   The current encoded word
  \param w   The new set of 32 bits to add
  \param p1  The pattern to associate with the encoding pattern 0
  \param p3  The pattern to associate with the encoding pattern 3

  The routine encodes 2 bits at a time, extracting the 2 bits from the
  2 most significant bits of the input word \e w. These 2 bits are
  compared to \a p1. If they are equal, a single bit, 0, is loaded into
  the LSB of the output word and the output word is shifted left 1 bit.
  If these 2 are equal to \e p3, an encoding pattern of 2 bits is shifted
  into the LSB of the output word. If the pattern is 0, no bits are
  inserted. Finally if the pattern is not 0, p1, or p3, the pattern
  10 is inserted.
                                                                         */
/* --------------------------------------------------------------------- */
static unsigned int encode (unsigned int e,
                            unsigned int w,
                            unsigned int p1,
                            unsigned int p3)
{
   int idx;

   PRINTF (("e = %8.8x w = %8.8x P1 = %8.8x  P3 = %8.8x\n", e, w,  p1, p3));
   
   for (idx = 0; idx < 16; idx++, w <<= 2)
   {
       int x = w & 0xc0000000;
       PRINTF (("w = %8.8x X = %8.8x adding ", w, x));
       
       if (x)
       {
           if (x == p1)  { e <<= 1;  PRINTF ((" a 0 ", e)); }
           else 
           {
               /* Make some room, and shift in either 0b10 or 0b01 */
               e <<= 2;
               e  |= 2;
               if (x == p3) { e |= 1; PRINTF ((" a 3 ")); }
               else                   PRINTF ((" a 2 "));
                 
           }
       }
       else PRINTF ((" none"));
       
       PRINTF ((" e = %8.8x\n", e));
   }

   return e;
}
/* --------------------------------------------------------------------- */



/* --------------------------------------------------------------------- *//*!

  \fn  unsigned int BTE_wordEncode (unsigned int w,
                                    unsigned int p,
                                    unsigned int scheme_size)
  \brief Encodes the original word and its higher level binary tree
         pattern returning a 32 bit output word. The algorithm is such
         that at most 32 bits are used.
         
  \param w      The original 32-bit word that is to be encoded
  \param p      The higher level binary tree pattern word. This is
                generally computed by calling BTE_wordPrepare().
  \param scheme_size  The encoding scheme to be used and the encoded size.
                This is generally computed by BTE_wordSize (). The
                scheme is one of

             - 0, one encoding, just return w,
             - 1, encode using 01 =  0, 10 = 10, 11 = 11
             - 2, encode using 01 = 10, 10 =  0, 11 = 11
             - 3, encode using 01 = 10, 10 = 11, 11 = 0
                                                                         */
/* --------------------------------------------------------------------- */
unsigned int BTE_wordEncode (unsigned int w,
                             unsigned int p,
                             unsigned int scheme_size)
{
   unsigned int e;
   int     scheme = scheme_size >> 16;
 
   if (scheme == 0) return w;

   /* Encode process */

   /*
    |
    |  If SCHEME  1            2              3
    |     01 = 1, 0    01 = 2,10      01 = 2,10    
    |     10 = 2,10    10 = 1,0       10 = 2,11
    |     11 = 3,11    11 = 2,11      11 = 1,0
   */
   {
       unsigned int p3 = 0xc0000000;
       if (scheme == 3) p3 = 0x80000000;

       scheme <<= 30;
       e = encode (0, p, scheme, p3);
       e = encode (e, w, scheme, p3);       
   }

   
   /* Left justify the encoded word */
   return e << (32 - (scheme_size & 0xffff));
}
/* --------------------------------------------------------------------- */




/* --------------------------------------------------------------------- *//*!

  \fn    unsigned int BTE_wordPrepare (unsigned int w)
  \brief Prepares a pattern word which includes all but the lowest level
         of the binary tree
 
  \param  w  The word to encode
  \return    The pattern word

   A 32 bit word express as a binary tree takes a maximum of 63 bits to
   encode. This tree looks like

  \verbatim
                          
                          
              +---------- x ----------+               word       =  1 bit
              |                       |
        +---- x ----+           +---- x ----+         half       =  2 bits
        |           |           |           |
     +- x -+     +- x -+     +- x -+     +- x -+      byte       =  4 bits
     |     |     |     |     |     |     |     |
     x     x     x     x     x     x     x     x      nibble     =  8 bits
    / \   / \   / \   / \   / \   / \   / \   / \
   x   x x   x x   x x   x x   x x   x x   x x   x    2 bit      = 16 bits
   ab cd ef gh ij kl mn op qr st uv wx yz AB CD EF    1 bit      = 32 bits
                                                                   --
                                                                   63 bits
  \endverbatim

   This pattern word is arranged by levels from the highest levels in
   the most significant bits to the lowest levels. Note that the lowest
   level of the tree is original 32 bit word.

   Here is an example of tree for w = 0x00011000

  \verbatim

                                1                                   L0
                                |
                 1--------------+----------------1                  L1
                 |                               |
         0-------+-------1               1-------+-------0          L2
         |               |               |               |
     0---+---0       0---+---1       1---+---0       0---+---0      L3
     |       |       |       |       |       |       |       |
   0-+-0   0-+-0   0-+-0   0-+-1   0-+-1   0-+-0   0-+-0   0-+-0    L4
   00 00   00 00   00 00   00 01   00 01   00 00   00 00   00 00    L5

  \endverbatim 
   The levels are

  \verbatim 

       l0                     1
       L1                    1 1
       L2                   01 10
       L3                0001   0100
       L4           00000001     01000000
       L5  0000000000000001       0001000000000000

  \endverbatim

   Which, ignoring the lowest level, l5, is the pattern.

  \verbatim
         0 1 11 0110 00010100 0000000101000000
       = 0x76180140

  \endverbatim

   Note, by convention, the highest level of the tree is ignored, so
   that only the upper 30 bits are used. Since the upper bit only
   indicates whether the word is 0 or non-zero, something the user
   can easily determine.
                                                                         */
/* --------------------------------------------------------------------- */
unsigned int BTE_wordPrepare (unsigned int w)
{
   unsigned int l1;
   unsigned int l2;
   unsigned int l3;
   unsigned int l4;
   unsigned int l5;
   unsigned int  p;

   /*
    |  FEDCBA9876543210fedcba9876543210
    |   FEDCBA9876543210fedcba987654321
    |                   
    |  L4
    |   E C A 8 6 4 2 0 e c a 8 6 4 2 0
    |   F D B 9 7 5 3 1 f d b 9 7 5 3 1
    |
    |
    |  L3
    |     C   8   4   0   c   8   4   0
    |     D   9   5   1   d   9   5   1
    |     E   A   6   2   e   a   6   2
    |     F   B   7   3   f   b   7   3
    |
    |  L2
    |         8       0       8       0
    |         9       1       9       1
    |         A       2       a       2
    |         B       3       b       3
    |         C       4       c       4
    |         D       5       d       5
    |         E       6       e       6
    |         F       7       f       7
    |
    |  L1
    |                 0               0
    |                 1               1
    |                 2               2
    |                 3               3
    |                 4               4
    |                 5               5
    |                 6               6
    |                 7               7
    |                 8               8
    |                 9               9
    |                 A               a
    |                 B               b
    |                 C               c
    |                 D               d
    |                 E               e
    |                 F               f
    |
    |  L0
    |   Well, you get the idea, its the OR of all the bits
    |   Its not really necessary since this original value
    |   is checked for a non-zero value.
    |
   */

   l5 = w;
   l4 = l5 | (l5 >>  1);   /* 2 bit    OR, every other bit */
   l3 = l4 | (l4 >>  2);   /* Nibble   OR, every 4rth  bit */
   l2 = l3 | (l3 >>  4);   /* Byte     OR, every 8th   bit */
   l1 = l2 | (l2 >>  8);   /* Halfword OR, every 16th  bit */

   PRINTF ((" Bit      OR = %8.8x\n"
            " 2 Bit    OR = %8.8x\n"
            " Nibble   OR = %8.8x\n"
            " Byte     OR = %8.8x\n"
            " Halfword OR = %8.8x\n",
            l5,
            l4 & 0x55555555,
            l3 & 0x11111111,
            l2 & 0x01010101,
            l1 & 0x00010001));



   /* L1 - Halfword level   */
   p  = (((l1 >> 15) & 2) | (l1 & 1)) << 30;

   /* L2 - Byte level   */
   p |= ((l2 >> (24-3) & 0x8)
     |   (l2 >> (16-2) & 0x4)
     |   (l2 >> ( 8-1) & 0x2)
     |   (l2 >> (   0) & 0x1)) << 26;

   /* l3 - Nibble level */
   p |= ((l3 >> (28-7) & 0x80)
     |   (l3 >> (24-6) & 0x40)
     |   (l3 >> (20-5) & 0x20)
     |   (l3 >> (16-4) & 0x10)
     |   (l3 >> (12-3) & 0x08)
     |   (l3 >> ( 8-2) & 0x04)
     |   (l3 >> ( 4-1) & 0x02)
     |   (l3 >> (   0) & 0x01)) << 18;

   /* L4 - 2-bit  level */
   p |= ((l4 >> (30-15) & 0x8000)
     |   (l4 >> (28-14) & 0x4000)
     |   (l4 >> (26-13) & 0x2000)
     |   (l4 >> (24-12) & 0x1000)
     |   (l4 >> (22-11) & 0x0800)
     |   (l4 >> (20-10) & 0x0400)
     |   (l4 >> (18- 9) & 0x0200)
     |   (l4 >> (16- 8) & 0x0100)
     |   (l4 >> (14- 7) & 0x0080)
     |   (l4 >> (12- 6) & 0x0040)
     |   (l4 >> (10- 5) & 0x0020)
     |   (l4 >> ( 8- 4) & 0x0010)
     |   (l4 >> ( 6- 3) & 0x0008)
     |   (l4 >> ( 4- 2) & 0x0004)
     |   (l4 >> ( 2- 1) & 0x0002)
     |   (l4 >> (    0) & 0x0001)) << 2;

   PRINTF (("BTE pattern = %8.8x\n", p));
   
      
   return p; 
}
/* --------------------------------------------------------------------- */





/* --------------------------------------------------------------------- *//*!

  \fn      unsigned int BTE_wordSize (unsigned int w, unsigned int p)
  \brief   Computes the optimal encoding size using of the 4 possible
           encoding schemes.

  \param   w   The original word (the lowest level  of the tree
  \param   p   The pattern  word (the higher levels of the tree
  \return      A packed word containing the optimal encoding scheme
               and its size in bits. The upper 16 bits are the scheme
               number and the lower 16 bits is the size in bits.

   The encoding size of using each of the 4 possible encoding schemes is
   found. The smallest size and its corresponding scheme identifier are
   returned. The scheme identifiers are

           0: Just use the original 32 bit number
           1: Use the scheme that assignes the 01 pattern the value 0
           2: Use the scheme that assignes the 10 pattern the value 0
           3: Use the scheme that assignes the 11 pattern the value 0
                                                                         */
/* --------------------------------------------------------------------- */
unsigned int BTE_wordSize (unsigned int w, unsigned int p)
{
   int cnts   = 0;
   int min    = 0;
   int scheme = 0;

   if (w == 0) return 32;
   else
   {
       int idx;
       
       for (idx = 0; idx < 16; idx++)
       {
           unsigned int x =   p & 3;
           unsigned int y =  (w & 3);

           cnts += (1 << 8*x);
           cnts += (1 << 8*y);
           
           p >>= 2;
           w >>= 2;
       }
       
       {
           int tot_00;
           int tot_11;
           int tot_01;
           int tot_10;

           /* Extract the counts from the 4 bytes */
           int cnt_00 = (cnts >>  0) & 0xff;
           int cnt_01 = (cnts >>  8) & 0xff;
           int cnt_10 = (cnts >> 16) & 0xff;
           int cnt_11 = (cnts >> 24) & 0xff;

           /* Compute the sums for the 3 different encoding schemes */
           tot_00 = cnt_00 + cnt_01 + cnt_10 + cnt_11;
           tot_01 = cnt_01 + 2 * (cnt_10 + cnt_11);           
           tot_10 = cnt_10 + 2 * (cnt_11 + cnt_01);           
           tot_11 = cnt_11 + 2 * (cnt_01 + cnt_10);

           /* Find the minimum */
           min    = 32     < tot_01 ? (scheme=0, 32)     : (scheme=1, tot_01);
           min    = tot_10 < min    ? (scheme=2, tot_10) : min;
           min    = tot_11 < min    ? (scheme=3, tot_11) : min;
           

              
           PRINTF (("Encode count = %d\n"
                   "Raw Counts = %2d %2d %2d %2d = %2d\n"
                   "W  0,1,2,2 = %2d %2d %2d %2d = %2d\n"
                   "W  0,2,1,2 = %2d %2d %2d %2d = %2d\n"
                   "W  0,2,2,1 = %2d %2d %2d %2d = %2d\n",
                   min,
                     cnt_00,   cnt_01,   cnt_10,   cnt_11, tot_00,
                   0*cnt_00, 1*cnt_01, 2*cnt_10, 2*cnt_11, tot_01,
                   0*cnt_00, 2*cnt_01, 1*cnt_10, 2*cnt_11, tot_10,
                   0*cnt_00, 2*cnt_01, 2*cnt_10, 1*cnt_11, tot_11));
       }
       
   }

   return (scheme << 16) | min;
}
/* --------------------------------------------------------------------- */


#include <ap_int.h>
typedef ap_uint<4> Nibble_t;
typedef ap_uint<2> BteLookup;
uint8_t bte_size (uint32_t w)
{
   static const BteLookup Lookup[16][3] =
   {
         /* Pattern               01   10, 11 */
         /*    0000          */   { 0,  0,  0},
         /*    0001 01 01    */   { 2,  0,  0},
         /*    0010 01 10    */   { 1,  1,  1},
         /*    0011 01 11    */   { 1,  0,  1},
         /*    0100 10 01    */   { 1,  1,  0},
         /*    0101 11 01 01 */   { 2,  0,  1},
         /*    0110 11 01 10 */   { 1,  1,  1},
         /*    0111 11 01 11 */   { 1,  0,  2},
         /*    1000 10 10    */   { 0,  2,  0},
         /*    1001 11 10 01 */   { 1,  1,  1},
         /*    1010 11 10 10 */   { 0,  2,  1},
         /*    1011 11 10 11 */   { 0,  1,  2},
         /*    1100 10 11    */   { 0,  1,  1},
         /*    1101 11 11 01 */   { 1,  0,  2},
         /*    1110 11 11 10 */   { 0,  1,  2},
         /*    1111 11 11 11 */   { 0,  0,  3}
   };

   ap_uint<6> n01 = 0;
   ap_uint<6> n10 = 0;
   ap_uint<6> n11 = 0;

   for (int i = 0; i < 2; i++)
   {
      Nibble_t nm = 0;

      for (ap_uint<4> m = 1; m != 0;  m <<= 1)
      {
         Nibble_t nibble = (w & 0xf);
         n01 += Lookup[nibble][0];
         n10 += Lookup[nibble][1];
         n11 += Lookup[nibble][2];

         if (nibble) nm |= m;
         w  >>= 4;
      }

      n01 += Lookup[nm][0];
      n10 += Lookup[nm][1];
      n11 += Lookup[nm][2];
   }

   // NEED TO CALCLULATE THE SIZE and SCHEME
   return 0;
}

#undef PRINTF
