#ifndef LIST_SINGLE_HH
#define LIST_SINGLE_HH

/* ---------------------------------------------------------------------- *//*!

   \file   List-Single.hh
   \brief  Singly linked list
   \author JJRussell - russell@slac.stanford.edu

\verbatim

    CVS $Id: L.ih,v 1.6 2005/10/01 01:00:11 russell Exp $
\endverbatim

   \par SYNOPSIS
    This defines the singly linked list routines. These allow the
    the user to build non-interlocked singly linked lists. In the
    non-interlocked form, they offer the slight advantage of being lower
    overhead, having to maintain only a single link.

                                                                          */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *\
 *
 * HISTORY
 * -------
 *
 * DATE       WHO WHAT
 * ---------- --- ------------------------
 * 2017.05.25 jjr Adopted from EXO routines
 
 *
\* ---------------------------------------------------------------------- */




#ifdef __cplusplus
extern "C"
{
#endif


/* ---------------------------------------------------------------------- *//*!

   \brief Template class to construct an singlarly linked list of a 
          specified type.

                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
class List<Body>
{
public:
   List ();
  ~List ();

  typedef List<Body> Head; 

  struct Node
  {
     Node      *m_flnk;
     Payload m_payload;
  }

  bool empty        ();
  Node insert_head  ();
  Node insert_tail  ();
  Node insert       (Node *node);
  Node remove_head  ();
  Node remove_tail  ();
  Node remove       (Node *node);

  NodeType append  (Head *list);
  NodeType prepend (Head *list);

public:
  Node *m_flnk;
  Node *m_blnk;

};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief       Constructor for an empty singly linked list


   The list is initialized to an empty list. This must be done before
   performing any other operations on the list.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
bool List<Body>::List () :
   m_flnk = static_cast<List<Body> *>(this)
{
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn           void append (L_head *dst, L_head *src)
  \brief        Appends the @a src list members to the @a dst list.
  \param    dst A previously initialized list acting as the destination
  \param    src A previously initialized list acting as the source

   Appends the source list to the destination list. After this operation
   the destination list will have consist of its original members followed
   by the members on the source list. The source list will be empty.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline void List<Body> (List<Body> *src)
{
    Node *sflnk;
    Node *sblnk;
    Node *dblnk;

    /*
               DESTINATION QUE

                     BEFORE
                                            NAMES
       +-------> +--------------+ -------+
       |         | flnk dst blnk|        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  dst.flnk
       |         | flnk  d0     |        |
       |   +---- +--------------+ <--+   |
       |   |                         |   |
       |   +---> +--------------+ ---+   |
       |         | flnk  d0     |        |
       +-------- +--------------+ <------+  dst.blnk


                     AFTER                  NAMES     NEW VALUES

       +-------> +--------------+ -------+            dst.blnk = src.blnk
       |         | flnk dst blnk|        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  dst.flnk
       |         | flnk d0      |        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |
       |         | flnk d1      |        |
       |   +---- +--------------+        |  dst.blnk  dst.blnk.flnk = src.flnk
       |   |                             |
       |   +---> +--------------+        |  src.flnk
       |         | flnk s0      |        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |
       |         | flnk s0      |        |
       +-------- +--------------+ <------+  src.blnk src.blnk.flnk = dst;
    */


    /* Check if source list is empty, if so nothing to do */
    sflnk = src->m_flnk;
    if ((char *)sflnk == static_cast<Node *>src) return;


    /* Non-empty, get the current tail of the source & destination lists */
    dblnk = m_blnk;
    sblnk = src->m_blnk;


    /* Link this to the first member of the source list */
    dblnk->m_flnk = sflnk;


    /* Make the tail of the destination the tail of the source list */
    m_blnk   = sblnk;


    /* Link the current tail of the source list to the destination head */
    sblnk->m_flnk   = static_cast<Node *>dst;


    /* Declare the source list empty */
    src->m_flnk = src->m_blnk = static_cast<Node *>src;

    return;
}
/* ---------------------------------------------------------------------- */





/* ---------------------------------------------------------------------- *//*!

  \fn          int L__empty (L_head *list)
  \brief       Returns non zero if the list is empty list.
  \return      Non zero if the list is empty list, else 0
  \param list  Pointer to the list to initialize.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
bool List<Body>::empty ()
{
   return (m_flnk == (Node *)this);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief       Destroys the data structures associate with the list.
  \param list  Pointer to the list to destroy.

   Currently this routine is effectively a NO-OP, but is provided for
   upward compatibility, just in case at some time in the future this
   operations does something meaningful.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
bool ~List<Body>
{
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn          L_node *insert_tail (L_head *list, L_node *node)
  \brief       Adds a node to the tail of a previously initialized list.
  \param  list A previously initialized list.
  \param  node The node to add at the tail of the mode
  \return      Pointer to the old tail. This can be used to test
               whether this was the first item on the list.

               If return_value == list then empty

   Adds the specified node to the tail of the list. If all nodes are added
   with the L__insert routine, the list behaves as a FIFO.
                                                                          */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn           L_node *L__jam (L_head *list, L_node *node)
  \brief        Adds a node to the head of a previously initialized list.
  \param  list  A previously initialized list.
  \param  node  The node to add.
  \return       Pointer to the old forward link. This can be used to test
                whether this was the first item on the list. \n
                If return_value == list, then empty

   Adds the specified node to the head of the list. If all nodes are added
   with the L__jam routine, the list behaves as a LIFO.

                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Node>
Node *insert_head (Node *node)
{
   Node *blnk;


   /* Current tail */
   blnk = m_blnk;


   /* Set the termination for the new node */
   m_flnk = static_cast<Node *)this;


   /* This is now the last item on the list */
   m_blnk = node;


   /* The old last item's flnk points to the new node */
   blnk->flnk = node;


   /* Return old last link */
   return blnk;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn           void L__prepend (L_head *dst, L_head *src)
  \brief        Prepends the @a src list members to the @a dst list.
  \param    dst A previously initialized list acting as the destination
  \param    src A previously initialized list acting as the source

   Prepends the source list to the destination list. After this operation
   the destination list will have consist of its original members preceded
   by the members on the source list. The source list will be empty.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn           L_node *L__remove (L_head *list)
  \param  list  A previously initialized list.
  \return       A pointer to the removed node or NULL if the list is empty.
  \brief        Removes the node from the head of a previously initialized
                list.. An empty list returns NULL as its node.

   Removes the node at the head of the list. If the list is empty, NULL
   is returned.
                                                                          */
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

  \fn                 L_node *L__unlink (const L_node *node,
                                         L_node *predecessor)
  \param  node        The node to unlink from the list
  \param  predecessor The predecessor node.
  \return             Pointer to the forward link of the removed node.
  \brief              Removes the specified node from the list.

   Unlinks the specified node from a previously initialized list.
                                                                          */
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- */
/*                       FUNCTION PROTOTYPES                              */
/* ---------------------------------------------------------------------- */
L__EXP_PROTO void    L__append (L_head *dst,  L_head *src ) ATTR_UNUSED_OK;
L__EXP_PROTO void    L__destroy(L_head *list)               ATTR_UNUSED_OK;
L__EXP_PROTO int     L__empty  (L_head *list)               ATTR_UNUSED_OK;
L__EXP_PROTO void    L__init   (L_head *list)               ATTR_UNUSED_OK;
L__EXP_PROTO L_node *L__insert (L_head *list, L_node *node) ATTR_UNUSED_OK;
L__EXP_PROTO L_node *L__jam    (L_head *list, L_node *node) ATTR_UNUSED_OK;
L__EXP_PROTO void    L__prepend(L_head *dst,  L_head *src ) ATTR_UNUSED_OK;
L__EXP_PROTO L_node *L__remove (L_head *list)               ATTR_UNUSED_OK;
L__EXP_PROTO L_node *L__unlink (const L_node *node,
                                      L_node *predecessor ) ATTR_UNUSED_OK;
/* ---------------------------------------------------------------------- */



#ifdef __cplusplus
}
#endif


L__EXP_FNC void L__append  (L_head *dst,  L_head *src)
{
    L_node *sflnk;
    L_node *sblnk;
    L_node *dblnk;

    /*
               DESTINATION QUE

                     BEFORE
                                            NAMES
       +-------> +--------------+ -------+
       |         | flnk dst blnk|        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  dst.flnk
       |         | flnk  d0     |        |
       |   +---- +--------------+ <--+   |
       |   |                         |   |
       |   +---> +--------------+ ---+   |
       |         | flnk  d0     |        |
       +-------- +--------------+ <------+  dst.blnk


                     AFTER                  NAMES     NEW VALUES

       +-------> +--------------+ -------+            dst.blnk = src.blnk
       |         | flnk dst blnk|        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  dst.flnk
       |         | flnk d0      |        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |
       |         | flnk d1      |        |
       |   +---- +--------------+        |  dst.blnk  dst.blnk.flnk = src.flnk
       |   |                             |
       |   +---> +--------------+        |  src.flnk
       |         | flnk s0      |        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |
       |         | flnk s0      |        |
       +-------- +--------------+ <------+  src.blnk src.blnk.flnk = dst;
    */


    /* Check if source list is empty, if so nothing to do */
    sflnk = src->flnk;
    if ((char *)sflnk == (char *)src) return;


    /* Non-empty, get the current tail of the source & destination lists */
    dblnk = dst->blnk;
    sblnk = src->blnk;


    /* Link this to the first member of the source list */
    dblnk->flnk = sflnk;


    /* Make the tail of the destination the tail of the source list */
    dst->blnk   = sblnk;


    /* Link the current tail of the source list to the destination head */
    sblnk->flnk   = (L_node *)dst;


    /* Declare the source list empty */
    src->flnk = src->blnk = (L_node *)src;

    return;
}




L__EXP_FNC void L__destroy (L_head *list)
{
   return;
}


L__EXP_FNC int  L__empty (L_head *list)
{
   return (int)((char *)list->flnk == (char *)list);
}




L__EXP_FNC void L__init (L_head *list)
{
   list->flnk = list->blnk = (L_node *)list;
}



L__EXP_FNC L_node *L__insert (L_head *list, L_node *node)
{
   L_node *blnk;


   /* Current tail */
   blnk = list->blnk;


   /* Set the termination for the new node */
   node->flnk = (L_node *)list;


   /* This is now the last item on the list */
   list->blnk = node;


   /* The old last item's flnk points to the new node */
   blnk->flnk = node;


   /* Return old last link */
   return blnk;
}



L__EXP_FNC L_node *L__jam (L_head *list, L_node *node)
{
   L_node *flnk;


   /* Current first item on the list      */
   flnk = list->flnk;


   /* First link the new node to the old first node   */
   node->flnk = flnk;


   /* This is now the first item on the list */
   list->flnk = node;


   return flnk;
}




L__EXP_FNC void L__prepend  (L_head *dst,  L_head *src)
{
    L_node *sflnk;
    L_node *sblnk;
    L_node *dflnk;


    /*
               DESTINATION QUE

                     BEFORE
                                            NAMES
       +-------> +--------------+ -------+
       |         | flnk dst blnk|        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  dst.flnk
       |         | flnk  d0     |        |
       |   +---- +--------------+ <--+   |
       |   |                         |   |
       |   +---> +--------------+ ---+   |
       |         | flnk  d0     |        |
       +-------- +--------------+ <------+  dst.blnk


                     AFTER                  NAMES     NEW VALUES

       +-------> +--------------+ -------+
       |         | flnk dst blnk|        |
       |   +---- +--------------+        |            dst.flnk = src.flnk
       |   |                             |
       |   +---> +--------------+        |  src.flnk
       |         | flnk s0      |        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  src.blnk
       |         | flnk s1      |        |
       |   +---- +--------------+        |            src.blnk.flnk = dst.flnk
       |   |                             |
       |   +---> +--------------+        |  dst.flnk
       |         | flnk d0      |        |
       |   +---- +--------------+        |
       |   |                             |
       |   +---> +--------------+        |  src.blnk
       |         | flnk d0      |        |
       +-------- +--------------+ <------+
    */


    /* Check if source list is empty, if so nothing to do */
    sflnk = src->flnk;
    if ((char *)sflnk == (char *)src) return;


    /* Non-empty, get tcurrent head of the source, tail of the destination */
    dflnk = dst->flnk;
    sblnk = src->blnk;


    /* Make the head of the destination list the first node of the source */
    dst->flnk = sflnk;


    /* Make the tail of the source forward link to the destination 0 */
    sblnk->flnk = dflnk;


    /* Declare the source list empty */
    src->flnk = src->blnk = (L_node *)src;

    return;
}



L__EXP_FNC L_node *L__remove (L_head *list)
{
   L_node *node;

   node = list->flnk;

   /* Check if there is one */
   if (node != (L_node *)list)
   {
       L_node *flnk   = node->flnk;
       list->flnk     = flnk;         /* New forward link is node after     */

       /* If this caused the list to go empty, then modify blnk */
       if (flnk == (L_node *)list) list->blnk = (L_node *)list;
   }
   else
   {
       node       = (L_node *)0;
   }

   return node;
}


L__EXP_FNC L_node *L__unlink (const L_node *node, L_node *predecessor)
{
   /*
    | Just need to replace the predecessor's link with the removed node's link
    | A word of caution. Although this looks like one can produce an
    | interlocked version by using a memory reservation technique, in fact
    | you can't because the predecessor node could be removed out from
    | underneath you during the relink, i.e. the predecessor ceases to
    | be the predecessor. One cannot even jacket this routine with a
    | semaphore or an interlock, because the same problem remains, the
    | predecessor was located outside of the lock. Ergo, no interlocked
    | version of this routine can be produced.
   */
   return predecessor->flnk = node->flnk;
}

#endif
