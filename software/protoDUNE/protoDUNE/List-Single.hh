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
 * ---------- --- -------------------------
 * 2017.05.25 jjr Adapted from EXO routines
 *
\* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!

   \brief Template class to construct an singlarly linked list of a 
          specified type.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
class List
{
public:
   List ();
  ~List ();

   struct Node
   {
      Node      *m_flnk;
      Body       m_body;
   };

   void        init         ();
   bool        is_empty     ()                 const;
   bool        no_more      (Node const *node) const;
   Node const *terminal     ()                 const;
   Node        *insert_head  (Node       *node);
   Node        *insert_tail  (Node       *node);
   Node        *remove_head  ();
   Node        *remove_tail  ();

   void append           (List<Body> *list);
   void prepend          (List<Body> *list);

   void split            (List<Body>       *list, 
                          List<Body>::Node *node);

   void transfer        (List<Body>       *list);

public:
  Node *m_flnk;
  Node *m_blnk;
};
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief       Initialize an empty singly linked list
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline void List<Body>::init ()
{
   m_flnk = reinterpret_cast<Node *>(this);
   m_blnk = m_flnk;
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief       Constructor for an empty singly linked list

   The list is initialized to an empty list.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline List<Body>::List () :
   m_flnk (reinterpret_cast<Node *>(this)),
   m_blnk (reinterpret_cast<Node *>(this))
{
   return;
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
inline List<Body>::~List()
{
   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \fn           void append (L_head *dst, L_head *src)
  \brief        Appends the @a src list members to the @a dst list.

  \param    src A previously initialized list acting as the source

   Appends the source list to this list. After this operation this list
   will consist of its original members followed by the members on the
   \a src list. The \a src list will be empty.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline void List<Body>::append (List<Body> *src)
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
    if (reinterpret_cast<Node *>(sflnk) == reinterpret_cast<Node *>(src)) return;

    /* Non-empty, get the current tail of the source & destination lists */
    dblnk = m_blnk;
    sblnk = src->m_blnk;

    /* Link this to the first member of the source list */
    dblnk->m_flnk = sflnk;

    /* Make the tail of the destination the tail of the source list */
    m_blnk   = sblnk;

    /* Link the current tail of the source list to the destination head */
    sblnk->m_flnk   = reinterpret_cast<Node *>(this);

    /* Declare the source list empty */
    src->m_flnk = src->m_blnk = reinterpret_cast<Node *>(src);

    return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief        Prepends the \a src list members to this list.

  \param    src A previously initialized list acting as the source

   Prepends the source list to this list. After this operation the destination
   list will have consist of its original members preceded by the members of
   the \a src list. The \a src list will be empty.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
void List<Body>::prepend (List<Body> *src)
{
    Node *sflnk;
    Node *sblnk;
    Node *dflnk;


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
    sflnk = src->m_flnk;
    if (reinterpret_cast<Node *>(sflnk) == reinterpret_cast<Node *>(src)) return;

    /* Non-empty, get tcurrent head of the source, tail of the destination */
    dflnk = m_flnk;
    sblnk = src->m_blnk;

    /* Make the head of the destination list the first node of the source */
    m_flnk = sflnk;

    /* Make the tail of the source forward link to the destination 0 */
    sblnk->m_flnk = dflnk;

    /* Declare the source list empty */
    src->m_flnk = src->m_blnk = reinterpret_cast<Node *>(src);

    return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief   Returns the terminal node
  \return  The terminal node.

   The terminal node is not a real node, so should not be accessed as
   such.  It is used to test whether a node in a scan has reached the
   end of the list.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
typename List<Body>::Node const *List<Body>::terminal () const
{
   return reinterpret_cast<Node const *>(this);
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief       Test for an empty list
  \retval      true, list is empty
  \retval      false, list in non-empty
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
bool List<Body>::is_empty () const
{
   return (m_flnk == reinterpret_cast<decltype (m_flnk)>(this));
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief       Test that this node is the terminal node
  \retval      true,  is the terminal node
  \retval      false, is not the terminal node

   The terminal node is not a real node, so should not be accessed as
   such.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
bool List<Body>::no_more (Node const *node) const
{
   return (node == reinterpret_cast<decltype (node)>(this));
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief       Adds a node to the tail of the list
  \return      Pointer to the old tail. This can be used to test
               whether this was the first item on the list.

               If return_value == list then list was empty

  \param  node The node to add at the tail of the mode


   Adds the specified node to the tail of the list. If all nodes are added
   with the insert_tail routine and removed with remove_head, the list 
   behaves as a FIFO.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline   typename List<Body>::Node *List<Body>::insert_tail (Node *node)
{
   Node *blnk;

   /* Current tail */
   blnk = m_blnk;

   /* This is now the last item on the list */
   m_blnk = node;

   /* Forward link on new node to top of the list */
   node->m_flnk = reinterpret_cast<Node *>(this);

   /* The old last item's flnk points to the new node */
   blnk->m_flnk = node;

   /* Return old last link */
   return blnk;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!


  \brief        Adds a node to the head of the list.
  \return       Pointer to the old forward link. This can be used to test
                whether this was the first item on the list. \n
                If return_value == list, then empty

  \param  node  The node to add.

   Adds the specified node to the head of the list. If all nodes are added
   with the insert_head routine and removed with remove_head, the ist
   behaves as a LIFO.
                                                                          */
/* ---------------------------------------------------------------------- */
template <typename Body>
inline    typename List<Body>::Node *List<Body>::insert_head (Node *node)
{
   Node *flnk;

   /* Current first item on the list      */
   flnk = m_flnk;

   /* First link the new node to the old first node   */
   node->m_flnk = flnk;

   /* This is now the first item on the list */
   m_flnk = node;

   return flnk;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief        Removes the node from the head of a previously initialized
                list.. An empty list returns NULL as its node.

  \return       A pointer to the removed node or NULL if the list is empty.

   Removes the node at the head of the list. If the list is empty, NULL
   is returned.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline   typename List<Body>::Node *List<Body>::remove_head ()
{
   Node *node;

   node = m_flnk;

   /* Check if there is one */
   if (node != reinterpret_cast<Node *>(this))
   {
      Node *flnk = node->m_flnk;
      m_flnk     = flnk;         /* New forward link is node after     */

      /* If this caused the list to go empty, then modify blnk */
      if (flnk == reinterpret_cast<Node *>(this)) 
      {
         m_blnk = reinterpret_cast<Node *>(this);
      }
   }
   else
   {
      node = reinterpret_cast<Node *>(0);
   }

   return node;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!

  \brief        Splits the input list starting into two pieces. 

  \param[in]    list  The input list
  \param[in]    node  The node last node in the \list

   All nodes after \a node are transferred to this list.
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline  void List<Body>::split (List<Body>       *list, 
                                List<Body>::Node *node)
{
   // This list begins at next node and ends at the last node of the list
   m_flnk = node->m_flnk;
   m_blnk = list->m_blnk;

   // Input list still begins at the same point, but ends at node
   list->m_blnk  = node;

   return;
}
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- *//*!

  \brief   Transfer the input list 

  \param[in] list The list to transfer

   The input list will be empty after this method
                                                                          */
/* ---------------------------------------------------------------------- */
template<typename Body>
inline  void List<Body>::transfer (List<Body> *list)
{
   *this = *list;
   this->m_blnk->m_flnk = reinterpret_cast<List<Body>::Node *>(this);
   list->init ();
   return;
}
/* ---------------------------------------------------------------------- */
#endif
