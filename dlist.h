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
 * @file   dlist.h: double-linked list implementation.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __DLIST_H__
#define __DLIST_H__

#include <stdio.h>

typedef struct dlist {
  struct slist* next;
  struct slist* prev;
} dlist_t;

inline void initDLL(dlist_t* dlist) { dlist->next = dlist->prev = NULL; }

// Whether a dlist is empty
inline bool isDLLEmpty(dlist_t* dlist) { return (dlist->next == NULL); }

// Next node of current node
inline slist_t* nextEntry(dlist_t* cur) { return cur->next; }

// Previous node of current node
inline slist_t* prevEntry(dlist_t* cur) { return cur->prev; }

// Insert one entry to the head of specified dlist.
// Insert between head and head->next
inline void insertDLLHead(slist_t* node, dlist_t* dlist) { 
	slist_t * old_head = dlist->next;
	slist_t * old_tail = dlist->prev;

	node->next = old_head;

	if(!old_tail) {
		dlist->prev = node;
	}

	dlist->next = node;
}

inline void insertDLLTail(slist_t* node, dlist_t* dlist) { 
	slist_t * old_tail = dlist->prev;
	node->next = NULL;

	if(old_tail) {
		old_tail->next = node;
	} else {
		dlist->next = node;
	}

	dlist->prev = node;
}

inline void printDLLList(dlist_t * dlist) {
		slist_t * next = dlist->next;
		printf("contents of dlist %p\n", dlist);
		printf("%p", next);

		while(next) {
				next = nextEntry(next);
				printf(", %p", next);
		}
		printf("\n");
}

// Delete an entry and re-initialize it.
inline void* removeDLLHead(dlist_t* dlist) {
	slist_t * old_head, * replacement;

	old_head = dlist->next;
	replacement = old_head->next;

	if(!replacement) {
		dlist->prev = NULL;
	}

	dlist->next = replacement;
	
	return old_head;
}

inline void insertAllDLLTail(slist_t* slist, dlist_t * dlist) {
  slist_t * tail_segment1 = slist->next;
  slist_t * tail_segment2 = NULL;
	slist_t * old_tail = dlist->prev;

	if(!tail_segment1) {
			return;
	}

	// Reset the dlist's prev pointer to point to the new tail.
	dlist->prev = tail_segment1;

	do {
			slist_t * next = nextEntry(tail_segment1);
			// The new tail segment should point to what is in front of it.
			tail_segment1->next = tail_segment2;
			tail_segment2 = tail_segment1;
			tail_segment1 = next;
	} while(tail_segment1);

	if(old_tail) {
			old_tail->next = tail_segment2;
	} else {
			dlist->next = tail_segment2;
	}
}
#endif
