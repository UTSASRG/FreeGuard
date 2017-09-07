/*
 * FreeGuard: A Faster Secure Heap Allocator
 * Copyright (C) 2017 Sam Silvestro, Hongyu Liu, Corey Crosser, 
 *                    Zhiqiang Lin, and Tongping Liu
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * @file   list.hh: double-linked list implementation.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __LIST_HH__
#define __LIST_HH__

#include <stdlib.h>

typedef struct list {
  struct list* prev;
  struct list* next;
} list_t;

// Initialize a node
inline void nodeInit(list_t* node) { node->next = node->prev = node; }

inline void listInit(list_t* node) { nodeInit(node); }

// Whether a list is empty
inline bool isListEmpty(list_t* head) { return (head->next == head); }

// Next node of current node
inline list_t* nextEntry(list_t* cur) { return cur->next; }

// Previous node of current node
inline list_t* prevEntry(list_t* cur) { return cur->prev; }

// We donot check whetehr the list is empty or not?
inline list_t* tailList(list_t* head) {
  list_t* tail = NULL;
  if(!isListEmpty(head)) {
    tail = head->prev;
  }

  return tail;
}

// Insert one entry to two consequtive entries
inline void __insert_between(list_t* node, list_t* prev, list_t* next) {
  // fprintf(stderr, "line %d: prev %p next %p\n", __LINE__, prev, next);
  // fprintf(stderr, "line %d: prev now %lx next %p\n", __LINE__, *((unsigned long *)prev), next);
  // fprintf(stderr, "line %d: prev->next %lx next %p\n", __LINE__, *((unsigned long *)((unsigned
  // long)prev + 0x8)), next);
  node->next = next;
  node->prev = prev;
  prev->next = node;
  next->prev = node;
}

// Insert one entry to after specified entry prev (prev, prev->next)
inline void listInsertNode(list_t* node, list_t* prev) { __insert_between(node, prev, prev->next); }

// Insert one entry to the tail of specified list.
// Insert between tail and head
inline void listInsertTail(list_t* node, list_t* head) {
  // fprintf(stderr, "node %p head %p head->prev %p\n", node, head, head->prev);
  __insert_between(node, head->prev, head);
}

// Insert one entry to the head of specified list.
// Insert between head and head->next
inline void listInsertHead(list_t* node, list_t* head) { __insert_between(node, head, head->next); }

// Internal usage to link p with n
// Never use directly outside.
inline void __list_link(list_t* p, list_t* n) {
  p->next = n;
  n->prev = p;
}

// We need to verify this
// Insert one entry to the head of specified list.
// Insert the list between where and where->next
inline void listInsertList(list_t* list, list_t* where) {
  // Insert after where.
  __list_link(where, list);

  // Then modify other pointer
  __list_link(list->prev, where->next);
}

// Insert one list between where->prev and where, however,
// we don't need to insert the node "list" itself
inline void listInsertListTail(list_t* list, list_t* where) {
#if 0
  // Link between where->prev and first node of list.
  list_t* origtail = where->prev;
  list_t* orighead = where;
  list_t* newhead = list->next;
  list_t* newtail = list->prev;

  origtail->next = newhead;
  newhead->prev = origtail;

  newtail->next = orighead;
  orighead->prev = newtail;
    
    p->next = n;
    n->prev = p;
#else
  __list_link(where->prev, list->next);

  // Link between last node of list and where.
  __list_link(list->prev, where);
#endif
}

// Delete an entry and re-initialize it.
inline void listRemoveNode(list_t* node) {
  __list_link(node->prev, node->next);
  nodeInit(node);
}

// Check whether current node is the tail of a list
inline bool isListTail(list_t* node, list_t* head) { return (node->next == head); }

// Retrieve the first item form a list
// Then this item will be removed from the list.
inline list_t* listRetrieveItem(list_t* list) {
  list_t* first = NULL;

  // Retrieve item when the list is not empty
  if(!isListEmpty(list)) {
    first = list->next;
    listRemoveNode(first);
  }

  return first;
}

// Retrieve all items from a list and re-initialize all source list.
inline void listRetrieveAllItems(list_t* dest, list_t* src) {
  list_t* first, *last;
  first = src->next;
  last = src->prev;

  first->prev = dest;
  last->next = dest;
  dest->next = first;
  dest->prev = last;

  // reinitialize the source list
  listInit(src);
}

// Print all entries in the link list
inline void listPrintItems(list_t* head, int num) {
  int i = 0;
  list_t* first, *entry;
  entry = head->next;
  first = head;

  while(entry != first && i < num) {
    //    fprintf(stderr, "%d: ENTRY %d: %p (prev %p). HEAD %p\n", getpid(), i++, entry,
    // entry->prev, head);
    entry = entry->next;
  }

  // PRINF("HEAD %p Head->prev %p head->next %p\n", head, head->prev, head->next);
}

/* Get the pointer to the struct this entry is part of
 *
 */
#define listEntry(ptr, type, member) ((type*)((char*)(ptr) - (unsigned long)(&((type*)0)->member)))

#endif
