/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


As a derivative work from code placed in the public domain by its
original author, this file is also in the public domain.
*/

/* File: dll.c
*  Purpose: Doubly-linked list routines.
*  Author:  Philip Thompson ($Author: phils $)
*  $Date: 94/04/07 19:31:47 $      $State: Exp $
*   The original doubly-linked list library was place in the public
*   domain by its author Paul Sander, paul@sander.uucp or
*   sander!paul@Atherton.COM. Heavily modified since.
*/
/* Adapted by Georg Mischler
 * - Unsigned long counters for list size to stretch limits.
 * - Replacement of custom fprintf routine (why do people do this?).
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef UNIX
	#include <unistd.h>
#endif

#include "dlltypes.h"
#include "dllproto.h"



/* DllNewNode -- This function creates and initializes a new node
 *  for a linked list.  The function's parameters are a pointer
 *  to a client-defined key and a pointer to arbitrary data.  The
 *  return value is a pointer to the new node, or NULL in case of
 *  error. 
 */
static DllNode *DllNewNode(void *key, void *data)

{
    DllNode *retval;

    if ((retval = (DllNode*)malloc(sizeof(DllNode))) == NULL) {
        fprintf(stderr, "DllNewNode: can't alloc node");
        return NULL;
    }
    retval->key = key;
    retval->data = data;
    retval->next = retval->prev = NULL;
    return retval;
}


/* DllStore -- This function inserts node2 into the linked list
 *  after node1.  Node2 is assumed not to be linked into the list.
 */
static void DllStore(DllNode *node1, DllNode *node2)

{
    node2->prev = node1;
    node2->next = node1->next;
    node1->next = node2;
    node2->next->prev = node2;
}


/* DllUnlink -- This function unlinks a node from a list.
*/
static void DllUnlink(DllNode *node)

{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}


/* DllLocate -- This function performs a sequential search of a
 *  doubly-linked list for a specified key.  This function
 *  assumes the list is sorted.
 */
static int DllLocate(DllList *list, void *key, DllNode **node)

{
    int res;
    DllNode *cur;

    cur = DLL_FIRST(list);
    res = (*list->comp)(cur->key, key);
    while ((res < 0) && (cur != list->last)) {
        cur = cur->next;
        res = (*list->comp)(cur->key, key);
    }
    *node = cur;
    return res;
}


/* DllTouch -- This function marks a list as having been modified.
 *              This affects the behavior of DllNext and DllPrev.
 */
void DllTouch(DllList *list)

{
    list->current = NULL;
    list->nextOk = 1;
    list->changed = 1;
}


/* DllNew -- Given a DllSetup structure, this function creates an empty
 *            doubly-linked list.
 */
DLL_LIST DllNewList(DLL_SETUP psetup)

{
    DllList *retval;
    DllSetup *setup;

    if ((setup = (DllSetup*)psetup) == NULL)
        return NULL;
    if ((retval = (DllList*)malloc(sizeof(DllList))) == NULL) {
        fprintf(stderr, "DllNewList: can't alloc list");
        return NULL;
    }
    retval->last = retval->current = NULL;
    retval->data = setup->data;
    retval->comp = setup->comp;
    retval->freeData = setup->freeData;
    retval->freeKey = setup->freeKey;
    retval->nextOk = 1;
    retval->size = retval->changed = retval->ordered = 0;
    return (DLL_LIST)retval;
}


/* DllSetup -- This function creates a doubly-linked list setup
 *  structure, and returns it to the caller.  The parameter is an
 *  optional comparison function (used in case the contents of
 *  the list are sorted).  The setup structure is then passed to
 *  DllNewList when a new list is created. 
 */
DLL_SETUP DllSetupList(CompProc comp,
					   FreeProc freeKey, FreeProc freeData,
					   void *data)

{
    DllSetup *retval;

    if ((retval = (DllSetup*)malloc(sizeof(DllSetup))) == NULL) { /* local heap */
        fprintf(stderr, "DllSetupList: can't alloc list");
        return NULL;
    }
    retval->comp = comp;
    retval->data = data;
    retval->freeKey = freeKey;
    retval->freeData = freeData;
    return (DLL_SETUP)retval;
}


/* DllFreeSetup -- This function frees a doubly-linked list setup
 *  structure.
 */
void DllFreeSetup(DLL_SETUP setup)

{
    if (setup != NULL)
        free((char*)setup); /* local heap */
}


/* DllDelete -- This function deletes a node by key from a sorted list.
 */
void *DllDelete(DLL_LIST plist, void *key, void **data)

{
    DllList *list;
    DllNode *cur;
    void *retval;

    /* Validate list parameter */
    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    if (list->comp == NULL)
        return NULL;
    /* Perform sequential search for node */
    if (DllLocate(list, key, &cur) != 0)
        return NULL;
    /* unlink and free node if found */
    retval = cur->key;
    if (data != NULL)
        *data = cur->data;
    DllUnlink(cur);
    /* Special consideration if this is the tail of the list */
    if (cur == list->last) {
        if (cur->prev != cur)
            list->last = cur->prev;
        else
            list->last = NULL;
    }
    free((char*)cur);
    list->size -= 1;
    DllTouch(list);
    return retval;
}


/* DllLocRank -- This function locates a node in a list by rank.
 */
static DllNode *DllLocRank(DllList *list, long rank)
{
    DllNode *node;

    node = DLL_FIRST(list);
    while (rank > 0) {
        node = node->next;
        rank--;
    }
    return node;
}


/* DllDelRank -- This function deletes a node by rank from a list.
 */
void *DllDelRank(DLL_LIST plist, unsigned long rank, void **data)
{
    DllList *list;
    DllNode *cur;
    void *retval;

    /* Validate list parameter */
    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    if (rank >= list->size)
        return NULL;
    /* Find the node with the given rank */
    cur = DllLocRank(list, rank);
    /* unlink and free node if found */
    if (cur == NULL)
        return NULL;
    retval = cur->key;
    if (data != NULL)
        *data = cur->data;
    DllUnlink(cur);
    /* Special consideration if cur is the tail of the list */
    if (cur == list->last) {
        if (cur->prev != cur)
            list->last = cur->prev;
        else
            list->last = NULL;
    }
    free((char*)cur);
    list->size -= 1;
    DllTouch(list);
    return retval;
}


/*  DllCopyList -- Copy a list structure and its data.
 */
DLL_LIST DllCopyList(DLL_LIST plist, DLL_SETUP psetup,
					 CopyProc copyKey, CopyProc copyData, void *info)

{
    DllList *list, *newn;
    DllNode *cur, *node;
    void *key = NULL, *data = NULL;
    DllSetup *setup;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if ((newn = (DllList*)malloc(sizeof(DllList))) == NULL) {
        fprintf(stderr, "DllCopyList: can't alloc list");
        return NULL;
    }
    memcpy(newn, list, sizeof(DllList));
    if ((setup = (DllSetup*)psetup) != NULL) {
        newn->data = setup->data;
        newn->comp = setup->comp;
        newn->freeData = setup->freeData;
        newn->freeKey = setup->freeKey;
    }
    newn->last = newn->current = NULL;
    newn->size = 0;

    if (list->last != NULL) {
        for (cur = DLL_FIRST(list); cur != list->last;
                cur = cur->next) {
            if (copyData != NULL)
                data = (*copyData)(cur->data, info);
            if (copyKey != NULL)
                key = (*copyKey)(cur->key, info);
            if ((node = DllNewNode(key, data)) == NULL) {
                fprintf(stderr, "DllCopyList: can't alloc node");
                free(newn);
                return NULL;
            }
            if (newn->last != NULL)
                DllStore(newn->last, node);
            else {
                node->next = node;
                node->prev = node;
            }
            newn->last = node;
            newn->size += 1;
        }
        /* do the last node in the list */
        if (copyData != NULL)
            data = (*copyData)(cur->data, info);
        if (copyKey != NULL)
            key = (*copyKey)(cur->key, info);
        if ((node = DllNewNode(key, data)) == NULL) {
            fprintf(stderr, "DllCopyList: can't alloc node");
            free(newn);
            return NULL;
        }
        if (newn->last != NULL)
            DllStore(newn->last, node);
        else {
            node->next = node;
            node->prev = node;
        }
        newn->last = node;
        newn->size += 1;
    }
    DllTouch(newn);
    if (list->size != newn->size) {
        fprintf(stderr, "DllCopyList: lists not same size");
        return NULL;
    }
    return (DLL_LIST)newn;
}


/* DllDestroyList -- This function destroys a doubly-linked list.  It is
 *  passed the doomed list, along with pointers to functions
 *  that destroy the keys and client-defined data stored in each
 *  node in the list.  An additional parameter is passed along to
 *  the callback functions without modification.  The freeData
 *  function is always called before the freeKey function.  Either
 *  or both of these may be NULL, in which no action is taken for
 *  that item.  The interfaces for freeKey and freeData are the
 *  same, which is the following: 
 *
 *  void freeMe(keyOrData,info)
 *  void *keyOrData;
 *  void *info;
 */
void DllDestroyList(DLL_LIST plist,
					FreeProc freeKey, FreeProc freeData, void *info)

{
    DllList *list;
    DllNode *cur, *next;

    if ((list = (DllList*)plist) == NULL)
        return;
    if (list->last != NULL) {
        cur = DLL_FIRST(list);
        while (cur != list->last) {
            next = cur->next;
            if (freeData != NULL)
                (*freeData)(cur->data, info);
            if (freeKey != NULL)
                (*freeKey)(cur->key, info);
            free((char*)cur);
            cur = next;
        }
        if (freeData != NULL)
            (*freeData)(cur->data, info);
        if (freeKey != NULL)
            (*freeKey)(cur->key, info);
        free((char*)cur);
    }
    free((char*)list);
}


/* DllTraverseAndDelete -- This function deletes a node by key from a
 *  sorted list.
 */
void DllTraverseAndDelete(DLL_LIST plist,
						  CompProc findKey, CompProc findData, void *info)

{
    DllList *list;
    DllNode *node, *next;

    /* Validate list parameter */
    if ((list = (DllList*)plist) == NULL)
        return;
    if (list->last == NULL)
        return;
    if (findData == NULL)
        return;
    for (node = DLL_FIRST(list); node != list->last; node = next) {
        next = node->next;
        if (findData != NULL)
            if ((*findData)((void*)node->data, (void*)info) != 0)
                continue;
        if (findKey != NULL)
            if ((*findKey)((void*)node->key, (void*)info) != 0)
                continue;
        /* unlink and free node if found */
        DllUnlink(node);
        if (list->freeData != NULL)
            (*list->freeData)(node->data, NULL);
        if (list->freeKey != NULL)
            (*list->freeKey)(node->key, NULL);
        free((char*)node);
        list->size--;
    }
    /* Special consideration for the tail of the list */
    if ((*findData)((void*)node->data, (void*)info) == 0) {
        DllUnlink(node);
        if (list->freeData != NULL)
            (*list->freeData)(node->data, info);
        if (list->freeKey != NULL)
            (*list->freeKey)(node->key, info);
        if (node->prev == node)
            list->last = NULL;
        else
            list->last = node->prev;
        free((char*)node);
        list->size--;
    }
    DllTouch(list);
}


void DllDump(DLL_LIST plist, void (*key_dump)(FILE*,void*,void*,void*), void *info, FILE *fp)

{
    DllList *list;
    DllNode *cur;

    if ((list = (DllList*)plist) == NULL) {
        fprintf(stderr, "DllDump: trying to dump null list");
        return;
    }
    if (fp == NULL)
        fp = stderr;
    fprintf(fp, "List handle located at %p\n", (void*)list);
    fprintf(fp, "last = %p\n", (void*)(list->last));
    fprintf(fp, "current = %p\n", (void*)(list->current));
    fprintf(fp, "data = %p\n", list->data);
    fprintf(fp, "nextOk = %d\n", list->nextOk);
    fprintf(fp, "size = %ld\n", list->size);
    fprintf(fp, "changed = %d\n", list->changed);
    fprintf(fp, "ordered = %d\n", list->ordered);
    fprintf(fp, "-----------\n");
    if (list->last != NULL) {
        cur = DLL_FIRST(list);
        do {
            fprintf(fp, "Node at %p:\n",  (void*)cur);
            fprintf(fp, "  next = %p\n",  (void*)cur->next);
            fprintf(fp, "  prev = %p\n",  (void*)cur->prev);
            if (key_dump != NULL)
                (*key_dump)(fp, cur->key, cur->data, info);
            fprintf(fp, "\n");
            cur = cur->next;
        } while (cur != DLL_FIRST(list));
    }
}


/* DllInsert -- This function takes as its parameters a doubly-
 *  linked list, a key, and a pointer to arbitrary data.  It
 *  performs a linear search of the list and inserts a new node in
 *  the appropriate place if the key is not found.  Returns values
 *  are:  1 for success, 0 for failure, -1 for duplicate key. 
 */
int DllInsert(DLL_LIST plist, void *key, void *data)

{
    DllList *list;
    DllNode *cur, *next;
    int res;

    /* Validate list parameter */
    if ((list = (DllList*)plist) == NULL)
        return 0;
    if (list->comp == NULL)
        return 0;
    /* Validate key parameter */
    if (key == NULL)
        return 0;
    /* Perform insertion */
    if (list->last == NULL) {
        /* Insert first node into empty list */
        if ((cur = DllNewNode(key, data)) == NULL)
            return 0;
        DllStore(cur, cur);
        list->last = cur;
    } else {
        /* Perform sequential search of list for given key */
        /* Test for duplicate */
        if ((res = DllLocate(list, key, &cur)) == 0)
            return -1;
        /* Create new node for new key */
        if ((next = DllNewNode(key, data)) == NULL)
            return 0;
        /* Insert before "cur" node if new key is less than "cur",
         * else insert after end of list and update list structure. */
        if (res > 0)
            DllStore(cur->prev, next);
        else {
            DllStore(list->last, next);
            list->last = next;
        }
    }
    list->size += 1;
    list->ordered = 1;
    DllTouch(list);
    return 1;
}


/* DllFirst -- This function returns the first key stored in the list.
 *   If the list is sorted, the key returned is the lowest key in
 *   the lexical order of those keys stored in the list.
 */
void *DllFirst(DLL_LIST plist, void **data)

{
    DllList *list;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    list->current = DLL_FIRST(list);
    list->nextOk = 1;
    list->changed = 0;
    if (data != NULL)
        *data = list->current->data;
    return list->current->key;
}


/* DllLast-- This function returns the last key stored in the list.
 *  If the list is sorted, the key returned is the highest key in
 *  the lexical order of those keys stored in the list.
 */
void *DllLast(DLL_LIST plist, void **data)

{
    DllList *list;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    list->current = list->last;
    list->nextOk = 1;
    list->changed = 0;
    if (data != NULL)
        *data = list->current->data;
    return list->current->key;
}


/* DllPrev -- This function returns the previous key stored in the list.
 *             If the list is sorted, this key is the next lowest in
 *             the lexical order of keys stored in the tree.
 */
void *DllPrev(DLL_LIST plist, void **data)

{
    DllList *list;

    /* Validate parameters */
    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    /* Require a search, first, or next */
    if (list->changed)
        return NULL;
    /* Last search was for a key that was beyond the end of the list */
    if (list->current == NULL)
        return DllLast(plist, data);
    /* Indicate error if list is overrun */
    if (list->current == DLL_FIRST(list)) {
        list->nextOk = 0;
        return NULL;
    }
    /* Return next item in list */
    list->current = list->current->prev;
    list->nextOk = 1;
    if (data != NULL)
        *data = list->current->data;
    return list->current->key;
}


/* DllNext -- This function returns the next key stored in the list.
 *  If the list is sorted, this key is the next highest in the
 *  lexical order of keys stored in the tree.
 */
void *DllNext(DLL_LIST plist, void **data)

{
    DllList *list;

    /* Validate parameters */
    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    /* Require a search, first, or next */
    if (list->changed)
        return NULL;
    /* If no search was done, return first item in list */
    if ((list->current == NULL) && list->nextOk) {
        /* This is never reached.  This condition is met only after the
         * linked list has been changed, in which case the test above
         * causes this function to fail. */
        return DllFirst(plist, data);
    }
    /* Indicate error if list is overrun */
    if (((list->current == list->last) && list->nextOk) ||
            (list->current == NULL)) {
        list->current = NULL;
        list->nextOk = 0;
        return NULL;
    }
    /* Return next item in list */
    if (list->nextOk)
        list->current = list->current->next;
    list->nextOk = 1;
    if (data != NULL)
        *data = list->current->data;
    return list->current->key;
}


/* DllPeekF -- This function returns the key and data of the item
 *  at the head of a doubly linked list.  It behaves similarly to
 *  DllFirst, except that subsequent calls to DllNext and DllPrev
 *  are not affected.
 */
void *DllPeekF(DLL_LIST plist, void **data)

{
    DllList *list;
    DllNode *node;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    node = DLL_FIRST(list);
    if (data != NULL)
        *data = node->data;
    return node->key;
}


/*  DllPeekR -- This function returns the key and data of the item
 *              at the end of a doubly linked list.  It behaves
 *              similarly to DllLast, except that subsequent calls
 *              to DllNext and DllPrev are not affected.
 */
void *DllPeekR(DLL_LIST plist, void **data)

{
    DllList *list;
    DllNode *node;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    node = list->last;
    if (data != NULL)
        *data = node->data;
    return node->key;
}


/* DllPeek -- This function returns the key and data of the item
 *  at the specified end of a doubly-linked list.  Subsequent
 *  calls to DllNext and DllPrev are not affected.
 */
void *DllPeek(DLL_LIST plist, int where, void **data)

{
    switch (where) {
    case DLL_FRONT:
        return DllPeekF(plist, data);
    case DLL_BACK:
        return DllPeekR(plist, data);
    default:
        fprintf(stderr, "DllPeek: invalid location (%d)", where);
        return NULL;
    }
}


/* DllPushF -- This function pushes an item onto the front of an
 *  unordered doubly-linked list.
 */
void *DllPushF(DLL_LIST plist, void *key, void *data)

{
    DllList *list;
    DllNode *node;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if ((node = DllNewNode(key, data)) == NULL)
        return NULL;
    if (list->last != NULL)
        DllStore(list->last, node);
    else {
        list->last = node;
        node->next = node;
        node->prev = node;
    }
    list->size += 1;
    list->ordered = 0;
    DllTouch(list);
    return key;
}


/* DllPushR -- This function pushes an item onto the rear of an
 *              unordered doubly-linked list.
 */
void *DllPushR(DLL_LIST plist, void *key, void *data)

{
    DllList *list;
    DllNode *node;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if ((node = DllNewNode(key, data)) == NULL)
        return NULL;
    if (list->last != NULL)
        DllStore(list->last, node);
    else {
        node->next = node;
        node->prev = node;
    }
    list->last = node;
    list->size += 1;
    list->ordered = 0;
    DllTouch(list);
    return key;
}


/* DllPush -- This function pushes an item onto a doubly-linked
 *             list at a specified end.
 */
void *DllPush(DLL_LIST plist, int where, void *key, void *data)

{
    switch (where) {
    case DLL_FRONT:
        return DllPushF(plist, key, data);
    case DLL_BACK:
        return DllPushR(plist, key, data);
    default:
        fprintf(stderr, "DllPush: invalid location (%d)", where);
        return NULL;
    }
}


/* DllPopF -- This function deletes an item from the front of a
 *             doubly-linked list.
 */
void *DllPopF(DLL_LIST plist, void **data)

{
    DllList *list;
    DllNode *node;
    void *retval;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    node = DLL_FIRST(list);
    retval = node->key;
    if (data != NULL)
        *data = node->data;
    if (node == list->last)
        list->last = NULL;
    else
        DllUnlink(node);
    free((char*)node);
    list->size -= 1;
    DllTouch(list);
    return retval;
}


/* DllPopR -- This function deletes an item from the rear of a
 *   doubly-linked list.
 */
void *DllPopR(DLL_LIST plist, void **data)

{
    DllList *list;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    list->last = list->last->prev;
    return DllPopF(plist, data);
}


/* DllPop -- This function removes an item from a doubly-linked
 *            list at a specified end.
 */
void *DllPop(DLL_LIST plist, int where, void **data)

{
    switch (where) {
    case DLL_FRONT:
        return DllPopF(plist, data);
    case DLL_BACK:
        return DllPopF(plist, data);
    default:
        fprintf(stderr, "DllPop: invalid location (%d)", where);
        return NULL;
    }
}


/* DllRank -- This function returns the specified item stored in the
 *  list, as specified by rank.
 */
void *DllRank(DLL_LIST plist, unsigned long rank, void **data)

{
    DllList *list;
    DllNode *node;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (rank >= list->size)
        return NULL;
    node = DllLocRank(list, rank);
    list->nextOk = 1;
    list->current = node;
    list->changed = 0;
    if (node != NULL) {
        if (data != NULL)
            *data = node->data;
        return node->key;
    }
    /* This code is never reached because earlier tests on rank
    * insure that a node is found. */
    return NULL;
}


/* DllSearch -- This function searches a sorted list for a specified
 *   key.
 */
void *DllSearch(DLL_LIST plist, void *target, void **data)

{
    int res;
    DllList *list;
    DllNode *node;
    void *retval;

    /* Validate parameters */
    if ((list = (DllList*)plist) == NULL)
        return NULL;
    if (list->last == NULL)
        return NULL;
    if (list->comp == NULL)
        return NULL;
    /* Perform sequential search for target */
    if ((res = DllLocate(list, target, &node)) < 0) {
        /* Target is higher than highest key, overran list */
        list->current = NULL;
        list->nextOk = 0;
        retval = NULL;
    } else if (res > 0) {
        /* Target not found, but has a successor */
        list->current = node;
        list->nextOk = 0;
        retval = NULL;
    } else {
        /* Target was found */
        list->current = node;
        list->nextOk = 1;
        if (data != NULL)
            *data = node->data;
        retval = target;
    }
    list->changed = 0;
    return retval;
}


/*  DllTraverse -- This function traverses a linked list, calling
 *                 a client-provided visitation function once for
 *                 each node stored there.  If the list is sorted,
 *                 the nodes are visited in the lexical order of
 *                 the keys.  If the list is unsorted, the nodes
 *                 are visited in the order in which DllFirst and
 *                 DllNext visits them.
 *                 The interface for the visitation function follows:
 *                      void fn(key,data,parms)
 *                      void *key, *parms, *data;
 *                 where "key" is the key stored in the node, "parms"
 *                 is an arbitrary client-specified pointer passed
 *                 to DllTraverse, and "data" is the data pointer
 *                 stored in the node.
 */


 void DllTraverse(DLL_LIST plist, TraversalProc fn, void *parms)

{
    DllList *list;
    DllNode *node;

    if ((list = (DllList*)plist) == NULL)
        return;
    if (list->last == NULL)
        return;
    if (fn == NULL)
        return;
    node = DLL_FIRST(list);
    do {
        (*fn)(node->key, node->data, parms);
        node = node->next;
    } while (node != DLL_FIRST(list));
}


/* DllGetListData -- This function retrieves the list's global
 *  client-defined data pointer.
 */
void *DllGetListData(DLL_LIST plist)

{
    DllList *list;

    if ((list = (DllList*)plist) == NULL)
        return NULL;
    return list->data;
}


/* DllSetData -- This function sets the list's global client-defined
 *   data pointer.
 */
void DllSetListData(DLL_LIST plist, void *data)

{
    DllList *list;

    if ((list = (DllList*)plist) != NULL)
        list->data = data;
}


/* DllGetSize -- This function retrieves the list's size, nunber of
 *  node in the list;
 */
long DllGetListSize(DLL_LIST plist)

{
    DllList *list;

    if ((list = (DllList*)plist) == NULL)
        return -1;
    return list->size;
}

/*** end dll.c ***/
