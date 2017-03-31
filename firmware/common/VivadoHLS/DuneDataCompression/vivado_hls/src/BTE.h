#ifndef BTE_H
#define BTE_H


/* ---------------------------------------------------------------------- *//*!
   
   \file  BTE.h
   \brief Binary Tree Encoding interface file
   \author JJRussell - russell@slac.stanford.edu

    Interface specification for routines to encode bit strings using a
    binary tree.
                                                                          */
/* ---------------------------------------------------------------------- */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  extern unsigned int BTE_wordEncode (unsigned int w,
				      unsigned int p,
				      unsigned int scheme_size);
    
  extern unsigned int BTE_wordPrepare (unsigned int w);

  
  extern unsigned int BTE_wordSize (unsigned int w, unsigned int p);

  static __inline int BTE_size  (int _schemeSize) { return (_schemeSize) & 0xffff;}
  static __inline int BTE_scheme(int _schemeSize) { return (_schemeSize) >> 16;   }


  uint32_t bte_prepare (uint32_t w);
  uint8_t  bte_size    (uint32_t w, uint32_t p);
  uint32_t bte_encode  (uint32_t w, uint32_t p, uint8_t scheme_size);

  static __inline uint8_t bte_nbits  (uint8_t schemeSize) { return schemeSize & 0x1f; }
  static __inline uint8_t bte_scheme (uint8_t schemeSize) { return schemeSize >>   5; }


#ifdef __cplusplus
}
#endif


#endif

    
    
