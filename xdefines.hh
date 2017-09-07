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
 * @file   xdefines.hh: global constants, enums, definitions, and more.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __XDEFINES_HH__
#define __XDEFINES_HH__

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>
#include <assert.h>

#include "slist.h"
#include "dlist.h"

/*
 * @file   xdefines.h
 */

extern char * getThreadBuffer();

extern "C" {
#ifndef CUSTOMIZED_STACK
__thread extern int _threadIndex;
#endif
typedef void * threadFunction(void*);

#ifdef LOGTOFILE
extern int outputfd;
#endif

#ifndef PTHREADEXIT_CODE
#define PTHREADEXIT_CODE 2230
#endif

typedef enum { LEFT, RIGHT } direction;

#ifdef LOGTOFILE
inline int getOutputFD() {
  return outputfd;
}
#endif

#ifndef CUSTOMIZED_STACK
inline int getThreadIndex() {
  return _threadIndex;
}

inline void setThreadIndex(int index) {
  _threadIndex = index;
}
#endif
inline size_t alignup(size_t size, size_t alignto) {
  return (size % alignto == 0) ? size : ((size + (alignto - 1)) & ~(alignto - 1));
}

inline void * alignupPointer(void * ptr, size_t alignto) {
  return ((intptr_t)ptr%alignto == 0) ? ptr : (void *)(((intptr_t)ptr + (alignto - 1)) & ~(alignto - 1));
}

inline size_t aligndown(size_t addr, size_t alignto) { return (addr & ~(alignto - 1)); }

#ifdef LOGTOFILE
#define OUTFD getOutputFD()
#else 
#define OUTFD 2
#endif
#define LOG_SIZE 4096

}; // extern "C"

#define LOG2(x) ((unsigned) (8*sizeof(unsigned long long) - __builtin_clzll((x)) - 1))

#ifdef FIFO_FREELIST
#warning FIFO freelist feature turned on
#define IS_FREELIST_EMPTY isDLLEmpty
#define FREELIST_INIT     initDLL
#define FREELIST_INSERT   insertDLLTail
#define FREELIST_MERGE    insertAllDLLTail
#define FREELIST_REMOVE   removeDLLHead
#define FREELIST_TYPE     dlist_t
#else
#define IS_FREELIST_EMPTY isSLLEmpty
#define FREELIST_INIT     initSLL
#define FREELIST_INSERT   insertSLLHead
#define FREELIST_MERGE    insertAllSLLHead
#define FREELIST_REMOVE   removeSLLHead
#define FREELIST_TYPE     slist_t
#endif
#define ALLOC_SENTINEL (slist_t *)0x1
#ifdef USE_CANARY
	#warning canary value in use
	#define CANARY_SENTINEL 0x7B
	#define NUM_MORE_CANARIES_TO_CHECK 2
	#define IF_CANARY_CONDITION ((size + 1) > LARGE_OBJECT_THRESHOLD)
#else
	#define IF_CANARY_CONDITION (size > LARGE_OBJECT_THRESHOLD)
#endif

#ifdef SSE2RNG
#define RNG_MAX 0x8000
#define SRAND(x) srand_sse(x)
#else
#define RNG_MAX RAND_MAX
#define SRAND(x) srand(x)
#endif
#define MAX_ALIVE_THREADS 128
#define BIBOP_NUM_HEAPS 1024
//#warning reduced BIBOP_BAG_SET_SIZE from 4 to 1
//#define BIBOP_BAG_SET_SIZE 1
#define BIBOP_BAG_SET_SIZE 4
#define BIBOP_BAG_SET_WEIGHT 8 
#define BIBOP_BAG_SET_RANDOMIZER (BIBOP_BAG_SET_SIZE * BIBOP_BAG_SET_WEIGHT)
#define BIBOP_BAG_SET_RANDOMIZER_MASK (BIBOP_BAG_SET_RANDOMIZER - 1)
#define BIBOP_BAG_SET_MASK (BIBOP_BAG_SET_SIZE - 1)

#define CACHEDFREELIST_THRESHOLD_RATIO_BYBAG 10
#define PAGESIZE 0x1000
#define CACHE_LINE_SIZE 64

#define TWO_KILOBYTES 2048
#ifdef DESTROY_ON_FREE
#warning destroy-on-free feature in use
#endif

/*
 * Important:
 * All BiBOP-related parameters must be specified as powers of 2
*/
#ifdef MANYBAGS
	#warning MANYBAGS flag in use
	#define BIBOP_NUM_BAGS 32
	//#warning increased MIN_RANDOM_BAG_SIZE to 4MB for use with Parsec
	#define MIN_RANDOM_BAG_SIZE 0x400000			// 4MB
	//#define MIN_RANDOM_BAG_SIZE 0x100000			// 1MB
	#define MAX_RANDOM_BAG_SIZE 0x1000000			// 16MB
	//#define BIBOP_BAG_SIZE 0x100000						// 1MB
	#define BIBOP_MIN_BLOCK_SIZE 16
	// When the use of trailing guard pages is turned on (controlled via the
	// -DENABLE_GUARDPAGE flag), the last usable bag will be the one whose
	// class size = BAG_SIZE/2. Otherwise, the last usable bag has class
	// size equal to the bag size.
	#define LARGE_OBJECT_THRESHOLD MIN_RANDOM_BAG_SIZE
#else
	#define BIBOP_NUM_BAGS 16
	#warning increased MIN_RANDOM_BAG_SIZE to 4MB for use with Parsec
	#define MIN_RANDOM_BAG_SIZE 0x400000			// 4MB
	//#define MIN_RANDOM_BAG_SIZE 0x100000			// 1MB
	#define MAX_RANDOM_BAG_SIZE 0x1000000			// 16MB
	//#define BIBOP_BAG_SIZE 0x400000					// 4MB
	#define BIBOP_MIN_BLOCK_SIZE 16
	#define LARGE_OBJECT_THRESHOLD 0x80000	// 512KB
#endif

#define BIBOP_NUM_SUBHEAPS MAX_ALIVE_THREADS
#define BIBOP_SUBHEAP_SIZE (long long)(BIBOP_NUM_BAGS * _bibopBagSize)
#define BIBOP_HEAP_SIZE (long long)(BIBOP_SUBHEAP_SIZE * BIBOP_NUM_SUBHEAPS)
#define PageSize 4096UL
#define PageMask (PageSize - 1)
#define PageSizeShiftBits 12
#define RANDOM_GUARD_PROP 0.1		// 10% random guard pages per bag
#define RANDOM_GUARD_RAND_CUTOFF (RANDOM_GUARD_PROP * RNG_MAX)
#define BIBOP_GUARD_PAGE_MAP_SIZE 16
#define BIBOP_GUARD_PAGE_MAP_SIZE_MASK (BIBOP_GUARD_PAGE_MAP_SIZE - 1)
#define THREAD_MAP_SIZE	1280

#ifdef CUSTOMIZED_STACK
#define STACK_SIZE  		0x800000	// 8M, PageSize * N
#define STACK_SIZE_BIT  23	// 8M
#define MAX_THREADS 		MAX_ALIVE_THREADS
#define INLINE      		inline __attribute__((always_inline))

#define GUARD_PAGE_SIZE PageSize // PageSize * N
#include <sys/mman.h>
extern intptr_t globalStackAddr;
// Get the thread index by its stack address
INLINE int getThreadIndex(void* stackVar) {
	//int index = ((intptr_t)stackVar - globalStackAddr) / STACK_SIZE;
	int index = ((intptr_t)stackVar - globalStackAddr) >> STACK_SIZE_BIT;
#if 0 // test
	if(index >= MAX_THREADS || index < 0) {
		fprintf(stderr, "var %p stackaddr %lx index %d\n", stackVar, globalStackAddr, index);
	}

	if(index == 1 ) {
		//char * tmp = (char*)(globalStackAddr + (index+1) * STACK_SIZE - 512);
		char * tmp = (char*)(globalStackAddr + (index) * STACK_SIZE + 512);
		fprintf(stderr, "touch %p thread %d %p - %p\n", tmp, index, globalStackAddr + (index) * STACK_SIZE, globalStackAddr + (index+1) * STACK_SIZE);
		*tmp = 'a';
	}
#endif
#ifdef CUSTOMIZED_MAIN_STACK
	assert(index >= 0 && index < MAX_THREADS);
	return index;
#endif
	if (index >= MAX_THREADS || index <= 0) return 0;
	else return index;
}
#endif

#define WORD_SIZE sizeof(size_t)
#define POINTER_SIZE sizeof(void *)
#define CALLSTACK_DEPTH 3
#define AUX_STORAGE_AREA_SIZE (POINTER_SIZE * CALLSTACK_DEPTH)
#define BIBOP_SHADOW_VALUE_SIZE sizeof(shadowMemData)

struct shadowMemAuxData {
		void * callsite[CALLSTACK_DEPTH];
};

class shadowObjectInfo {
public:
	slist_t listentry; // Always put the next pointer to the first one. Thus, it is easy 
								// to operate for pointer operations
#if defined(DETECT_UAF) || defined(DETECT_BO)  
	struct shadowMemAuxData aux;
#endif
};

#endif
