#ifndef BFU_H
#define BFU_H


/* ---------------------------------------------------------------------- *//*!

   \file  BFU.h
   \brief Bit Field Unpack Routines
   \author JJRussell - russell@slac.stanford.edu

    Inline functions to unpack bit fields. The bit fields are packed
    big-endian style, with bit 0 being the most significant bit 
                                                                          */
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *\
 * 
 * HISTORY
 * -------
 *
 * DATE     WHO WHAT
 * -------- --- ---------------------------------------------------------
 * 09.30.06 jjr Corrected _bfu_construct (it didn't used the newly 
 *              calculated bit offset to extract the buffered word into
 *              cur). Added _bfu_constructW for constructing a BFU from
 *              an aligned source buffer.
 * 09.30.06 jjr Added History log
\* ---------------------------------------------------------------------- */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef uint64_t  BfuWord_t;
#define BFUWORD_K_NBITS   64
#define BFUWORD_M_ALLBITS 0xfffffffffffffffLL
#define BFUWORD_M_TOPBIT  (1LL << 63)

typedef uint32_t BfuPosition_t;
#define BFUPOSITION_V_INDEX    6 /*!< Shift to get a BfuWord index        */
#define BFUPOSITION_M_BIT   0x3f /*!< Mask  to get a BfuWord bit positiion*/

/* ====================================================================== */
/* Structures, Public                                                     */
/* ---------------------------------------------------------------------- *//*!

  \struct _BFU
  \brief   The return value of the BFU routines, capturing the current
           word that is being used as a source and the extracted
           value
                                                                          *//*!
  \typedef BFU
  \brief   Typedef for struct _BFU
                                                                          */
/* ---------------------------------------------------------------------- */
typedef struct _BFU
{
  BfuWord_t         cur;  /*!< The current word                           */
  BfuWord_t         val;  /*!< The extracted word or updated position     */
}
BFU;
/* ====================================================================== */




/* ====================================================================== */
/* Inlines, Prototypes                                                    */
/* ---------------------------------------------------------------------- */

  static __inline BFU BFU__bitL    (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position);

  static __inline BFU BFU__bitR    (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position);

  static __inline BFU BFU__boolean (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position);

  static __inline BFU BFU__wordR   (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position,
                                          unsigned int  width);

  static __inline BFU BFU__wordL   (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position,
                                          unsigned int  width);

  static __inline BFU BFU__word    (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position);

  static __inline BFU BFU__ffc     (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position);
  
  static __inline BFU BFU__ffs     (const BfuWord_t    *wrds,
                                          BfuWord_t     cur,
                                          BfuPosition_t position);

/* ====================================================================== */




/* ====================================================================== */
/* Inlines, Implementation                                                */
/* ---------------------------------------------------------------------- */
//#include <FFS.ih>
/* ---------------------------------------------------------------------- *//*!

  \def   _bfu_get_pos(_bfu)
  \brief  Gets the current buffer bit position
  \retval The value of the current buffer bit position
  \param  _bfu  The context to fetch the buffer bit position from
                                                                          *//*!
  \def   _bfu_get_tmp(_bfu)
  \brief  Gets the current temporary buffer
  \retval The value of the temporary buffer
  \param  _bfu  The context to fetch the temporary buffer from
                                                                          *//*!

  \def   _bfu_get(_bfu, _tmp, _pos)
  \brief  Gets the current temporary buffer and buffer bit position
  \param  _bfu  The context to fetch the temporary buffer from
  \param  _tmp  Returned as the value of the temporary holding buffer
  \param  _pos  Returned as the value of the buffer bit position
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_get_pos(_bfu) _bfu.val
#define _bfu_get_tmp(_bfu) _bfu.cur
#define _bfu_get(_bfu, _tmp, _pos)               \
                      _tmp = _bfu.cur;           \
                      _pos = _bfu.val;





/* ---------------------------------------------------------------------- *//*!

  \def   _bfu_put_tmp(_bfu, _tmp)
  \brief  Updates the current temporary buffer

  \param _bfu  The context to fetch the temporary buffer from
  \param _tmp  Returned as the updated value of the temporary holding buffer
                                                                          *//*!
  \def   _bfu_put_pos(_bfu, _pos)
  \brief  Gets the current buffer bit position
  \param _bfu  The context to fetch the buffer bit position from
  \param _pos  The updated value of the buffer bit position
                                                                          *//*!
  \def   _bfu_put(_bfu, _tmp, _pos)
  \brief  Updates the current temporary buffer and buffer bit position
  \param _bfu  The context to update
  \param _tmp  The updated value of the temporary holding buffer
  \param _pos  The updated value of the buffer bit position
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_put_tmp(_bfu, _tmp)       _bfu.cur = _tmp;
#define _bfu_put_pos(_bfu, _pos)       _bfu.val = _pos;
#define _bfu_put(_bfu, _tmp, _pos)     _bfu.cur = _tmp;           \
                                       _bfu.val = _pos;
/* ---------------------------------------------------------------------- */



static __inline int bfu_bit (BfuPosition_t position)
{
   int bit = position & BFUPOSITION_M_BIT;
   return bit;
}

static __inline int bfu_lshift (BfuPosition_t position)
{
   int lshift = position & BFUPOSITION_M_BIT;
   return lshift;
}

static __inline int bfu_rshift (BfuPosition_t position)
{
   int rshift =  BFUWORD_K_NBITS - 1 - (position & BFUPOSITION_M_BIT);
   return rshift;
}

static inline int bfu_index (BfuPosition_t position)
{
   int index = position >> BFUPOSITION_V_INDEX;
   return index;
}

/* ---------------------------------------------------------------------- *//*!

  \fn   static __inline BFU BFU__boolean (const BfuWord_t    *wrds,
                                                BfuWord_t    cur,
                                                BfuPosition  position)

  \brief  Unpacks the next bit. This is equivalent to a bit extract, but
          the justification (left or right) is unspecified. Rather the
          must efficient extraction is used, returning only 0 or non-zero.
          Use BFU__bitL or BFU__bitR if a specific justification is needed.
  \retval 0, if bit was 0
  \retval !=0, if bit was 1 (i.e. non-zero

  \param wrds     The input word array
  \param cur      The current set of bits one is working on
  \param position The current bit position in the output bit field
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__boolean (const BfuWord_t    *wrds,
                                        BfuWord_t     cur,
                                        BfuPosition_t position)
{
   BFU bfu;

   /* Compute where in the current word this field is to be extracted from*/
   int lshift = bfu_lshift (position);


   /* Get the bit, always guaranteed to be at least 1 bit in the buffer   */
   bfu.val = (cur << lshift) & BFUWORD_M_TOPBIT;;


   /* Has the buffered word field been exhausted ? */
   if (lshift == BFUWORD_K_NBITS -1)  cur = wrds[bfu_index (position) + 1];
   bfu.cur = cur;

   return bfu;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn   static __inline BFU BFU__bitR (const BfuWord_t    *wrds,
                                             BfuWord_t     cur,
                                             BfuPosition   position)

  \brief  Unpacks the right justified bit from the current position.
  \return The extracted bit, right justified

  \param wrds     The input word array
  \param cur      The current set of bits one is working on
  \param position The current bit position in the output bit field
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__bitR (const BfuWord_t     *wrds,
                                     BfuWord_t      cur,
                                     BfuPosition_t  position)
{
   BFU bfu;

   /* Compute where in the current word this field is to be extracted from */
   int rshift = bfu_rshift (position);


   /* Get the bit, always guaranteed to be at least 1 bit in the buffer */
   bfu.val = (cur >> rshift) & 1;


   /* Has the buffered word field been exhausted ? */
   if (rshift == 0)  cur = wrds[(bfu_index (position)) + 1];

   bfu.cur = cur;

   return bfu;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \fn   static __inline BFU BFU__bitL (const BfuWord_t   *wrds,
                                             BfuWord_t    cur,
                                             BfuPosition  position)

  \brief  Unpacks the left justified bit from the current position.
  \return The extracted bit, right justified

  \param wrds     The input word array
  \param cur      The current set of bits one is working on
  \param position The current bit position in the output bit field
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__bitL (const BfuWord_t    *wrds,
                                     BfuWord_t     cur,
                                     BfuPosition_t position)
{
   BFU bfu;

   /* Compute where in the current word this field is to be extracted from */
   int lshift = bfu_lshift (position);


   /* Get the bit, always guaranteed to be at least 1 bit in the buffer */
   bfu.val = (cur << lshift) & BFUWORD_M_TOPBIT;


   /* Has the buffered word field been exhausted ? */
   if (lshift == BFUWORD_K_NBITS -1)  cur = wrds[(bfu_index (position)) + 1];
   bfu.cur = cur;


   return bfu;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn   static __inline BFU BFU__wordR (const BfuWord_t    *wrds,
                                              BfuWord_t     cur,
                                              BfuPositin_t  position,
                                              unsigned int  width)

  \brief  Unpacks a right justified bit field from the current position.
          The width of the bit field must be less than 32 bits.
  \return The extracted field, right justified

  \param wrds     The input word array
  \param cur      The current set of bits one is working on
  \param position The current bit position in the output bit field
  \param width    The width of the field to be extracted
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__wordR (const BfuWord_t    *wrds,
                                      BfuWord_t    cur,
                                      unsigned int  position,
                                      unsigned int  width)
{
   BFU bfu;

   /* Compute where in the current word this field is to be extracted from */
   int    rposition = bfu_bit (position);
   int       rshift = BFUWORD_K_NBITS - (rposition + width);
   unsigned int msk = ((1 << (width-1)) << 1) - 1;

   if      (rshift  > 0)  bfu.val = (cur >> rshift) & msk;
   else if (rshift == 0) 
   {
       bfu.val = (cur >> rshift) & msk;
       cur = wrds[(bfu_index (position)) + 1];
   }
   else
   {
       /*
        | Crosses over to the next word
        | In this case -shift is the number of bits in the next word
       */
       unsigned int  tmp;
       unsigned int nwrd;


       rshift    = -rshift;
       nwrd      = bfu_index (position)  + 1;
       tmp       = (cur & (msk >> rshift)) << rshift;
       cur       = wrds[nwrd];
       tmp      |= cur >> (BFUWORD_K_NBITS - rshift);
       bfu.val   = tmp;
   }
   bfu.cur   = cur;

   return bfu;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \fn   static __inline BFU BFU__wordL (const BfuWord_t    *wrds,
                                              BfuWord_t     cur,
                                              BfuPosition_t position,
                                              unsigned int  width)

  \brief  Unpacks a left justified bit field from the current position.
          The width of the bit field must be less than 32 bits.
  \return The extracted field, left justified + the current buffered word

  \param wrds     The input word array
  \param cur      The current set of bits one is working on
  \param position The current bit position in the output bit field
  \param width    The width of the field to be extracted
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__wordL (const BfuWord_t    *wrds,
                                      BfuWord_t     cur,
                                      BfuPosition_t position,
                                      unsigned int  width)
{
   BFU bfu;

   bfu       = BFU__wordR (wrds, cur, position, width);
   bfu.val <<= BFUWORD_K_NBITS - width;
   return bfu;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn   static __inline BFU BFU__word (const unsigned int *wrds,
                                              unsigned int  cur,
                                              unsigned int  position)

  \brief  Unpacks exactly 32 bits from the current position.
  \return The extracted field, right justified

  \param wrds     The input word array
  \param cur      The current set of bits one is working on
  \param position The current bit position in the output bit field
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__word (const BfuWord_t    *wrds,
                                     BfuWord_t     cur,
                                     unsigned int  position)
{
   BFU bfu;

   bfu = BFU__wordR (wrds, cur, position, 32);
   return bfu;

#ifdef OPTIMIZED

   /* Compute where in the current word this field is to be extracted from */
   int       rshift = bfu_rshift (position);


   /* If exactly on a word boundary */
   if (lshift == 0)
   {
       bfu.val = cur;
       cur     = wrds[bfu_index (position) + 1];
   }
   else
   {
       /*
        | Crosses over to the next word
        | In this case shift is the number of bits in the next word
       */
       BfuWord_t     tmp;
       unsigned int nwrd;

       nwrd      = bfu_index (position)  + 1;
       tmp       = (cur & (BFUWORD_M_ALLBITS >> lshift)) << lshift;
       cur       = wrds[nwrd];
       tmp      |= cur >> (BFUWORD_K_NBITS - lshift);

       bfu.val   = tmp;
   }
   bfu.cur   = cur;
#endif

   return bfu;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \fn     static __inline BFU BFU__ffc (const Bfuword_t    *wrds,
                                              BfuWord_t     cur,
                                              BfuPosition_t position)

  \brief  Scans the input bit stream for the first clear bit, this
          effectively counts the number of set bits
  \return The updated BFU past the first clear bit.

  \param wrds     The input word array
  \param cur      The current 32-bit word
  \param position The current bit position in the output bit field
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__ffc (const BfuWord_t     *wrds,
                                    BfuWord_t      cur,
                                    BfuPosition_t  position)
{
    BFU           bfu;
    BfuPosition_t pos = position;
    int          left = bfu_lshift (pos);
    unsigned int tmp = ~(cur << left);

    left = BFUWORD_K_NBITS - left;


    /* 
     | Check for special case of 32 bits left and all bits formerly set
     | This is the only case where the inverse of tmp will be all 0s.
    */
    while (tmp == 0)
    {
        //printf ("Found tmp == 0 advancing pos %8.8x + %2.2x ->\n", pos,left);
        pos += left;
        //printf (" %8.8x\n", pos);
        cur  = wrds[bfu_index (pos)];
        tmp  = ~cur;
        left = BFUWORD_K_NBITS;
    }


    while (1)
    {
        int n;

        
        /* The above check ensures that there must be a 1 bit somewhere */
        n    = __builtin_clzl (tmp);
        pos += n;
        //printf ("N = %2.2x Left = %2.2x\n", n, left);

        if (n >= left)
        {
            /* The bit that was found was past the valid bits, reduce by 1 */
            cur  = wrds[bfu_index (pos)];
            //printf ("New value of cur = %8.8x pos %8.8x: %8.8x\n",
            //cur, ~cur, pos);
            if (n == left) break;
            pos -= 1;

            while (1)
            {
               tmp = ~cur;
               //printf ("New value of cur = %8.8x pos %8.8x: %8.8x\n",
               //cur, tmp, pos);
               if (tmp) break;
               pos += BFUWORD_K_NBITS;
               cur  = wrds[bfu_index (pos)];
               //printf ("Advancing pos by 64 -> %8.8x\n", pos);
            }
            left = BFUWORD_K_NBITS;
        }
        else 
        {
            break;
        }
    }

    
   _bfu_put_pos (bfu, pos);
   _bfu_put_tmp (bfu, cur);
    return bfu;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn     static __inline BFU BFU__ffs (const Bfuword_t    *wrds,
                                              BfuWord_t     cur,
                                              BfuPosition_t position)

  \brief  Scans the input bit stream for the first set bit, this
          effectively counts the number of clear bits
  \return The updated BFU past the first set bit.

  \param wrds     The input word array
  \param cur      The current 32-bit word
  \param position The current bit position in the output bit field
                                                                          */
/* ---------------------------------------------------------------------- */
static __inline BFU BFU__ffs (const BfuWord_t     *wrds,
                                    BfuWord_t      cur,
                                    BfuPosition_t  position)
{
    BFU           bfu;
    BfuPosition_t pos = position;
    int          left = bfu_lshift (pos);
    BfuWord_t     tmp = cur << left;

    left = BFUWORD_K_NBITS - left;


    /* 
     | Check for special case of 64 bits left and all bits will be 0s.
    */
    while (tmp == 0)
    {
        //printf ("Found tmp == 0 advancing pos %8.8x + %2.2x ->\n", pos,left);
        pos += left;
        //printf (" %8.8x\n", pos);
        cur  = wrds[bfu_index (pos)];
        tmp  = cur;
        left = BFUWORD_K_NBITS;
    }


    while (1)
    {
        int n;

        
        /* The above check ensures that there must be a 0 bit somewhere */
        n    = __builtin_clzl (tmp);
        pos += n;
        //printf ("N = %2.2x Left = %2.2x\n", n, left);

        if (n >= left)
        {
            /* The bit that was found was past the valid bits, reduce by 1 */
            tmp  = wrds[bfu_index (pos)];
            //printf ("New value of cur = %8.8x pos %8.8x: %8.8x\n",
            //cur, ~cur, pos);
            if (n == left) break;
            pos -= 1;

            while (1)
            {
               //printf ("New value of cur = %8.8x pos %8.8x: %8.8x\n",
               //cur, tmp, pos);
               if (cur) break;
               pos += BFUWORD_K_NBITS;
               cur  = wrds[bfu_index (pos)];
               tmp  = cur;
               //printf ("Advancing pos by 64 -> %8.8x\n", pos);
            }
            left = BFUWORD_K_NBITS;
        }
        else 
        {
            break;
        }
    }
    
   _bfu_put_pos (bfu, pos);
   _bfu_put_tmp (bfu, cur);
    return bfu;
}
/* ---------------------------------------------------------------------- */




/* ====================================================================== */
/* Convenience macros                                                     */
/* ---------------------------------------------------------------------- *//*!

  \def   _bfu_construct(_bfu, _s, _alignment, _src, _boff)
  \brief  Constructs a BFU from an arbitrarily aligned source buffer
          pointer, and a bit offset \a _boff. Also returns a 32-bit
          aligned pointer to be used with _bfu_extract

  \param _bfu       The BFU structure to populate
  \param _s         Returned as a 32-bit aligned version of \a _src
  \param _alignment Returned as the alignment adjustment. This value
                    should be subtracted off when resetting the bit 
                    offset back to a value that corresponds to the
                    alignment associated with \a _src.
  \param _src       The source buffer pointer
  \param _boff      The original bit offset, returned as the adjusted offset
                    to compensate for any possible alignment to \a _s
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_construct(_bfu, _s, _alignment, _src, _boff)                  \
do                                                                         \
{                                                                          \
    /* Move to the nearest 32-bit boundary */                              \
                                                                           \
    /* Convert to a 32-bit pointer */                                      \
    _s = (const unsigned int *)_src;                                       \
                                                                           \
    /* Calculate the number of bits that are not a multiple of 32 */       \
    alignment = ((uintptr_t)_s & 0x3) << 3;                                \
                                                                           \
    /* Add this to the bit offset */                                       \
    _boff += _alignment;                                                   \
                                                                           \
    /* Reset the pointer back to the nearest 32-bit boundary */            \
    _s = (const unsigned int *)((uintptr_t)_s                              \
                             & ~(uintptr_t)0x3);                           \
                                                                           \
    /* Extract the first word */                                           \
    _bfu.cur = _s[bfu_index (_boff)];                                      \
    _bfu.val = boff;                                                       \
                                                                           \
} while (0)
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \def   _bfu_constructW(_bfu, _src, _boff)
  \brief  Constructs a BFU from an 32-bit (word) aligned source buffer
          pointer and a bit offset \a _boff. 

  \param _bfu  The BFU structure to populate
  \param _src  The source buffer pointer
  \param _boff The original bit offset, returned as the adjusted offset
               to compensate for any possible alignment to \a _s
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_constructW(_bfu, _src, _boff)                                 \
    _bfu.cur = _src[bfu_index (_boff)];                                           \
    _bfu.val = _boff                                                       \

/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_extractBoolean (_bfu, _wrds, _position)
  \brief   Extracts the next bit, returning it as a boolean value. The
           justification is unspecfied. The implementation is the most
           efficient of _bfu_extractBL or _bfu_extractBR. Use one of those
           routine if the justification needs to be specified.
  \retval  0, if the bit was 0
  \retval  !=0, if the bit was 1

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the bit to be extracted from the
                    source array
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_extractBoolean(_bfu, _wrds, _position)                        \
       (_bfu       = BFU__boolean (_wrds, _bfu.cur, _position),            \
        _position += 1,                                                    \
        _bfu.val)
/* ---------------------------------------------------------------------- */  




/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_extractBR(_bfu, _wrds, _position)
  \brief   Extracts the right justified bit from the current position
  \return  The right justified bit

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the bit to be extracted from the
                    source array
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_extractBR(_bfu, _wrds, _position)                             \
       (_bfu       = BFU__bitR (_wrds, _bfu.cur, _position),               \
        _position += 1,                                                    \
        _bfu.val)
/* ---------------------------------------------------------------------- */  




/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_extractBL(_bfu, _wrds, _position)
  \brief   Extracts the left justified bit from the current position
  \return  The left justified bit

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the bit to be extracted from the
                    source array
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_extractBL(_bfu, _wrds, _position)                             \
       (_bfu       = BFU__bitL (_wrds, _bfu.cur, _position),               \
        _position += 1,                                                    \
        _bfu.val)
/* ---------------------------------------------------------------------- */  



/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_extractL(_bfu, _wrds, _position, _width)
  \brief   Extracts a left justified value from the current position
  \return  The left justified value

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the left most bit of the field
                    to be extracted in the source array
  \param  _width    The width of the field to be extracted; must be < 32
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_extractL(_bfu, _wrds, _position, _width)                     \
       (_bfu       = BFU__wordL (_wrds, _bfu.cur, _position, _width),     \
        _position += _width,                                              \
        _bfu.val)
/* ---------------------------------------------------------------------- */  



/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_extractR(_bfu, _wrds, _position, _width)
  \brief   Extracts a right justified value from the current position
  \return  The right justified value

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the left most bit of the field
                    to be extracted in the source array
  \param  _width    The width of the field to be extracted; must be < 32
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_extractR(_bfu, _wrds, _position, _width)                     \
       (_bfu       = BFU__wordR (_wrds, _bfu.cur, _position, _width),     \
        _position += _width,                                              \
        _bfu.val)
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_extract32(_bfu, _wrds, _position)
  \brief   Extracts 32 bits from the current position
  \return  The extract value

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the left most bit of the field
                    to be extracted in the source array
                                                                          */
/* ---------------------------------------------------------------------- */
#define _bfu_extract32(_bfu, _wrds, _position)                             \
       (_bfu       = BFU__word (_wrds, _bfu.cur, _position),               \
        _position += 32,                                                   \
        _bfu.val)
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_count1s(_bfu, _wrds, _position)
  \brief   Counts the number of 1s till the first clear bit
  \return  The number of bits to the first clear bit

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the left most bit of the field
                    to be extracted in the source array
                                                                          */
/* ---------------------------------------------------------------------- */
#define  _bfu_count1s(_bfu, _wrds, _position)                              \
        (_bfu      = BFU__ffc (_wrds, _bfu_get_tmp(bfu), _position),       \
         _position += (_bfu.val -= _position),                             \
         _bfu.val - 1)
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \def    _bfu_revert(_bfu, _wrds, _position, _nwidth, _nexp)
  \brief   Counts the number of 1s till the first clear bit
  \return  The number of bits to the first clear bit

  \param  _bfu      The bit field unpacking context
  \param  _wrds     The source word array
  \param  _position The bit position of the left most bit of the field
                    to be extracted in the source array
  \param  _nwidth   The bit width of the word to be reverted
  \param  _nexp     The number bits used to store the exponent

  \warning
   This macro uses a GCC enhancement to the preprocessor and is likely
   not portable.
                                                                          */
/* ---------------------------------------------------------------------- */
#define  _bfu_revert(_bfu, _wrds, _position, _nwidth, _nexp)               \
({                                                                         \
   int  val;                                                               \
   int  exp        = _bfu_extractR (_bfu, _wrds, _position, _nexp) - 1;    \
   int  nleading   = (1 << _nexp) - 1;                                    \
   int  ntrailing  = (_nwidth - nleading > 0) ? _nwidth - nleading : 0;    \
   int  nbits      = (exp >= 0)                                            \
                   ? (exp += ntrailing, val = (1 << exp), exp)             \
                   : (val = 0, exp = ntrailing);                           \
                                                                           \
   /* printf ("Exp = %2u, Nbits = %2u\n", exp, nbits); */                  \
                                                                           \
   /* If any more bits, get them */                                        \
   if (nbits > 0)                                                          \
   {                                                                       \
      /* printf ("Val = %8.8x -> ", val); */                               \
      val |= _bfu_extractR (_bfu, _wrds, _position, nbits);                \
      /* printf ("%8.8x\n", val); */                                       \
   }                                                                       \
   val;                                                                    \
})
/* ---------------------------------------------------------------------- */



#ifdef __cplusplus
}
#endif



#endif
