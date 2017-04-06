#ifndef BTD_H
#define BTD_H


/* ---------------------------------------------------------------------- *//*!

   \file  BTD.h
   \brief Binary Tree Dencoding interface file
   \author JJRussell - russell@slac.stanford.edu

    Interface specification for routines to decode bit strings previously
    binary tree encoded data.
                                                                          */
/* ---------------------------------------------------------------------- */



#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int BTD_wordDecode  (unsigned int       w,
                                     unsigned int  scheme,
                                     int           *nbits);

extern unsigned int BTD_shortDecode (unsigned short int s,
                                     unsigned int  scheme,
                                     int           *nbits);

#ifdef __cplusplus
}
#endif


#endif

