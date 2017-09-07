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
 * @file   slist.h: single-linked list implementation.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __SLIST_H__
#define __SLIST_H__

#include <stdlib.h>

typedef struct slist {
  struct slist* next;
} slist_t;

inline void initSLL(slist_t* slist) { slist->next = NULL; }

// Whether a slist is empty
inline bool isSLLEmpty(slist_t* slist) { return (slist->next == NULL); }

// Next node of current node
inline slist_t* nextEntry(slist_t* cur) { return cur->next; }

// Insert one entry to the head of specified slist.
// Insert between head and head->next
inline void insertSLLHead(slist_t* node, slist_t* slist) { 

	node->next = slist->next;
	slist->next = node;
}

inline slist_t * getTailSLL(slist_t *slist);
// Insert all entries to the head of specified slist.
inline void insertAllSLLHead(slist_t* node, slist_t* slist) { 
  assert(node!=NULL);
  slist_t* tmp = slist->next;
  slist->next = node->next;
  slist_t* tail = getTailSLL(node->next);
  tail->next = tmp;
}

// Delete an entry and re-initialize it.
inline void* removeSLLHead(slist_t* slist) {
	slist_t * temp;

	temp = slist->next;
//	fprintf(stderr, "slistnext %p\n", slist->next);
//	fprintf(stderr, "slist->next->next %p\n", slist->next->next);
	slist->next = slist->next->next;

	return temp;
}

inline slist_t * getTailSLL(slist_t *slist) {
  if(slist == NULL) {
    return NULL;
  }

  slist_t * current = slist;
  while(current->next != NULL) {
    current = nextEntry(current);
  }
  return current;
}

inline int countSLL(slist_t *slist) {
	if(slist == NULL) {
		return -1;
	}

	int length = 0;
	slist_t * prev = slist;
	slist_t * current = slist->next;

	while(current != NULL) {
		length++;
		prev = current;
		current = nextEntry(current);
	}

	return length;
}

#endif
