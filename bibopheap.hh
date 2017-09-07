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
 * @file   bibopheap.hh: main BIBOP heap implementation.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __BIBOPHEAP_H__
#define __BIBOPHEAP_H__

#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "xdefines.hh"
#include "mm.hh"
#include "log.hh"
#include "errmsg.hh"

#ifdef SSE2RNG
#include "sse2rng.h"
#elif ARC4RNG
extern "C" uint32_t arc4random_uniform(uint32_t upper_bound);
#endif

class BibopHeap {
private:
	// The start of the heap area.
	char * _heapBegin;
	// The end of the heap area.
	char * _heapEnd;
	unsigned long _heapMask;
	unsigned _heapSizeShiftBits;

	// The boundaries of the shadow memory region.
	char * _shadowMemBegin;
	char * _shadowMemEnd;
	size_t _shadowMemSizePerHeap;
	size_t _shadowMemSizePerHeapCeil;
	unsigned _shadowMemSizePerHeapCeilShiftBits;
	unsigned long _shadowMemSizePerHeapMask;
	size_t _shadowObjectInfoSize;
	unsigned _shadowObjectInfoSizeShiftBits;
	unsigned _numUsableBags;
	unsigned _lastUsableBag;

	size_t _bibopBagSize;
	size_t _firstBagPower;
	size_t _threadSize;
	unsigned _threadShiftBits;

	unsigned long _bagShiftBits;
	unsigned long _bagMask;
	/*****************************************************
		Information about each heap
	******************************************************/
	unsigned long _numObjectsPerHeap;
	unsigned long _numObjectsPerSubHeap;
	unsigned long _numBagsPerHeap;
	unsigned long _numBagsPerSubHeapMask;
	unsigned _numBagsPerHeapShiftBits;
	
	class PerThreadBag {
		public:
			// Pointing to the freelist objects 
			FREELIST_TYPE freelist[BIBOP_BAG_SET_SIZE];

			// The lock to protect the operations on freelist
			pthread_spinlock_t listlock[BIBOP_BAG_SET_SIZE];

			// Pointing to the memory that is not allocated.
			char * position[BIBOP_BAG_SET_SIZE];

			// The address of last object in the current heap
			char * lastofCurBag[BIBOP_BAG_SET_SIZE];
			
			unsigned numObjects;
			unsigned lastObjectIndex;
			unsigned bagNum;
			unsigned threadIndex; 
			unsigned long classMask;
			size_t classSize;	
			size_t shiftBits;
	
			// Starting offset of the current bag in the current heap
			size_t startOffset;
			size_t startShadowMemOffset;
		
			// offset of the first object in the next heap from the stop offset
			size_t nextHeapObjectOffset;
			size_t nextShadowHeapObjectOffset;

      // cached free list  
      slist_t cfreelist;
      int ncfree;
      int cflthreshold;
			
#ifdef ENABLE_GUARDPAGE
      size_t guardsize;
      size_t guardoffset;
			char padding[2];
#else
			char padding[56];
#endif
	};

	PerThreadBag _threadBag[MAX_ALIVE_THREADS][BIBOP_NUM_BAGS];

public:
	static BibopHeap & getInstance() {
      static char buf[sizeof(BibopHeap)];
      static BibopHeap* theOneTrueObject = new (buf) BibopHeap();
      return *theOneTrueObject;
  }

	void * initialize() {
		unsigned threadNum, bagNum;
		size_t lastUsableBagSize;

		#ifdef BIBOP_BAG_SIZE
		_bibopBagSize = BIBOP_BAG_SIZE;
		#else		// randomized bag size
		unsigned randPower = getRandomNumber() % (LOG2(MAX_RANDOM_BAG_SIZE / MIN_RANDOM_BAG_SIZE) + 1);
		_bibopBagSize = MIN_RANDOM_BAG_SIZE << randPower;
		#endif

		if(_bibopBagSize > LARGE_OBJECT_THRESHOLD) {
				lastUsableBagSize = LARGE_OBJECT_THRESHOLD;
		} else {
				lastUsableBagSize = _bibopBagSize;
		}
		_numUsableBags = LOG2(lastUsableBagSize) - LOG2(BIBOP_MIN_BLOCK_SIZE) + 1;
		if(_numUsableBags > BIBOP_NUM_BAGS) {
				_numUsableBags = BIBOP_NUM_BAGS;
		}
		_lastUsableBag = _numUsableBags - 1;

		assert(BIBOP_HEAP_SIZE > 0);

		_shadowObjectInfoSize = sizeof(shadowObjectInfo);
		_shadowObjectInfoSizeShiftBits = LOG2(sizeof(shadowObjectInfo));

		_firstBagPower = LOG2(BIBOP_MIN_BLOCK_SIZE);
		_bagShiftBits = LOG2(_bibopBagSize);
		_threadSize = (_bibopBagSize * BIBOP_NUM_BAGS);
		_threadShiftBits = LOG2(_threadSize);
		_bagMask = _bibopBagSize - 1;

		PRINF("_bibopBagSize=0x%lx, _bagShiftBits=%ld, sizeof(PerThreadBag)=%zu, _shadowObjectInfoSize=%zu",
			_bibopBagSize, _bagShiftBits, sizeof(PerThreadBag), _shadowObjectInfoSize);
		PRINF("BIBOP_NUM_BAGS=%u, _numUsableBags=%u", BIBOP_NUM_BAGS, _numUsableBags);

		// _shadowObjectInfoSize must be a power of 2,
		// otherwise _shadowObjectShiftBits logic won't work.
		assert(__builtin_popcount(_shadowObjectInfoSize) == 1);

		// BIBOP_BAG_SET_SIZE must be a power of 2, otherwise the
		// bitmask logic associated with BIBOP_BAG_SET_MASK won't work.
		// To accommodate an arbitrary integer set size we must change
		// this particular bitwise AND operation to a modulo operation.
		assert(__builtin_popcount(BIBOP_BAG_SET_SIZE) == 1);

		// To prevent false sharing of the PerThreadBag global array
		// we must ensure its size is cache line-aligned.
		assert((sizeof(PerThreadBag) % CACHE_LINE_SIZE) == 0);

		// Bag size cannot be smaller than the large object threshold.
		assert(_bibopBagSize >= LARGE_OBJECT_THRESHOLD);

		// Allocate the heap all at once.
		size_t totalHeapSize = BIBOP_NUM_HEAPS * BIBOP_HEAP_SIZE;
		_heapMask = BIBOP_HEAP_SIZE - 1;
		_heapSizeShiftBits = LOG2(BIBOP_HEAP_SIZE);
		allocHeaps(totalHeapSize);

		unsigned long numBagObjects;
		unsigned long numCumObjects = 0;
		unsigned long offsetBag = 0;
		unsigned long offsetShadowMem = 0;
		unsigned long shiftBits;
		size_t unusableHeapSpace = (BIBOP_NUM_BAGS - _numUsableBags) * _bibopBagSize;

		// Initialize each thread bag's free list, and other information
		for(threadNum = 0; threadNum < MAX_ALIVE_THREADS; threadNum++) {
			size_t classSize = BIBOP_MIN_BLOCK_SIZE;
			shiftBits = LOG2(classSize);

			for(bagNum = 0; bagNum < _numUsableBags; bagNum++) {
				PerThreadBag * curBag = &_threadBag[threadNum][bagNum];
				
				curBag->classSize = classSize;
				curBag->classMask = classSize - 1;
				curBag->shiftBits = shiftBits;
				for(int curBagSetItem = 0; curBagSetItem < BIBOP_BAG_SET_SIZE; curBagSetItem++) {
						FREELIST_INIT(&curBag->freelist[curBagSetItem]);
						pthread_spin_init(&curBag->listlock[curBagSetItem], 0);
				}
				initSLL(&curBag->cfreelist);
				curBag->ncfree = 0;
				curBag->bagNum = bagNum;
				curBag->threadIndex = threadNum; 
				curBag->startShadowMemOffset = offsetShadowMem;

				numBagObjects = _bibopBagSize / classSize;

				#ifdef ENABLE_GUARDPAGE
						size_t guardsize = classSize > PAGESIZE ? classSize : PAGESIZE;
						size_t guardoffset = guardsize;
						if(bagNum == _lastUsableBag) {
								// If this bag can only fit one object, forego the use of a guard object.
								if(_bibopBagSize == lastUsableBagSize) {
										guardoffset = 0;
										guardsize = 0;
								}
								guardsize += (BIBOP_NUM_BAGS - LOG2(lastUsableBagSize) + LOG2(BIBOP_MIN_BLOCK_SIZE) - 1) * _bibopBagSize;

								//PRDBG("last usable bag: lastUsableBagSize=%zu, _bibopBagSize=%zu, guardsize=%zu, guardoffset=%zu",
								//		lastUsableBagSize, _bibopBagSize, guardsize, guardoffset);
						}
						curBag->guardsize = guardsize;
						curBag->guardoffset = guardoffset;
				#else
						size_t guardoffset = 0;
				#endif

						curBag->nextHeapObjectOffset = BIBOP_HEAP_SIZE * (BIBOP_BAG_SET_SIZE - 1) +
								(BIBOP_HEAP_SIZE - _bibopBagSize + classSize + guardoffset);
						for(int curBagSetItem = 0; curBagSetItem < BIBOP_BAG_SET_SIZE; curBagSetItem++) {
								// Initialize bump pointer to the first object
								curBag->position[curBagSetItem] = _heapBegin + offsetBag + (curBagSetItem * BIBOP_HEAP_SIZE);
								curBag->lastofCurBag[curBagSetItem] = getLastOfBag(curBag->position[curBagSetItem], guardoffset, classSize);
								//ptrdiff_t diff = curBag->lastofCurBag[curBagSetItem] - curBag->position[curBagSetItem];
								//PRINF("thread %u bag %u set %d: classSize=%zu, guardsize=%zu, guardoffset=%zu, lastofCurBag=%p, position=%p, diff=%lu",
								//				threadNum, bagNum, curBagSetItem, classSize, curBag->guardsize, guardoffset, curBag->lastofCurBag[curBagSetItem], curBag->position[curBagSetItem], diff);
								#ifdef ENABLE_GUARDPAGE
								setGuardPage(curBag->position[curBagSetItem], curBag->guardsize, guardoffset);
								#endif
						}

						#ifdef ENABLE_GUARDPAGE
						// We must perform this >1 test or else we will underflow this value
						// for bags which have 0 objects in them. Also, the last usable bag
						// will only contain a single object, but it will not be reduced to
						// make room for a guard page, but rather the next bag (which is
						// unusable) will be converted entirely into a guard page. 
						if(numBagObjects > 1) {
								if(classSize < PageSize) {
										numBagObjects -= (PageSize / classSize);
								} else {
										numBagObjects--;
								}
						}
						#endif


				curBag->cflthreshold = numBagObjects / CACHEDFREELIST_THRESHOLD_RATIO_BYBAG;
				curBag->startOffset = offsetBag;
				curBag->numObjects = numBagObjects;
				curBag->lastObjectIndex = numBagObjects - 1;

				// Update the following values; 
				numCumObjects += numBagObjects;
				offsetBag += _bibopBagSize;
				offsetShadowMem += numBagObjects * _shadowObjectInfoSize;
				shiftBits++;
				classSize *= 2;
			}
			offsetBag += unusableHeapSpace;
		}

		_numBagsPerSubHeapMask = BIBOP_NUM_BAGS - 1;
		_numBagsPerHeap = BIBOP_NUM_BAGS * BIBOP_NUM_SUBHEAPS;
		_numBagsPerHeapShiftBits = LOG2(_numBagsPerHeap);
		_numObjectsPerHeap = numCumObjects;
		_numObjectsPerSubHeap = numCumObjects / BIBOP_NUM_SUBHEAPS;
		_shadowMemSizePerHeap = _numObjectsPerHeap * _shadowObjectInfoSize;
		_shadowMemSizePerHeapCeilShiftBits = (sizeof(size_t) * 8) - __builtin_clzl(_shadowMemSizePerHeap - 1);
		_shadowMemSizePerHeapCeil = (1ULL << _shadowMemSizePerHeapCeilShiftBits); 
		_shadowMemSizePerHeapMask = _shadowMemSizePerHeapCeil - 1; 

		// Iterate through the PerThreadBag array once more, setting the 
		for(threadNum = 0; threadNum < MAX_ALIVE_THREADS; threadNum++) {
				for(bagNum = 0; bagNum < _numUsableBags; bagNum++) {
						PerThreadBag * curBag = &_threadBag[threadNum][bagNum];
						curBag->nextShadowHeapObjectOffset = _shadowMemSizePerHeapCeil * BIBOP_BAG_SET_SIZE -
								((unsigned long)(curBag->numObjects - 1) << _shadowObjectInfoSizeShiftBits);
				}
		}

		allocShadowMem();
		PRINF("_shadowMemBegin=%p, _shadowMemEnd=%p, _shadowMemSizePerHeap=%zu, _smSPHeapCeilShiftBits=%u",
						_shadowMemBegin, _shadowMemEnd, _shadowMemSizePerHeap, _shadowMemSizePerHeapCeilShiftBits);

		return _heapBegin;
	}

	void allocHeaps(size_t heapSize) {
			_heapBegin = (char *)MM::mmapAllocatePrivate(heapSize, NULL);
			_heapEnd = _heapBegin + heapSize;
			madvise(_heapBegin, heapSize, MADV_NOHUGEPAGE);
			PRINF("_heapBegin=%p, _heapEnd=%p", _heapBegin, _heapEnd);
	}

	void allocShadowMem() {
      size_t totalShadowMemSize = BIBOP_NUM_HEAPS * _shadowMemSizePerHeap;
      _shadowMemBegin = (char *)MM::mmapAllocatePrivate(totalShadowMemSize, NULL);
      _shadowMemEnd = _shadowMemBegin + totalShadowMemSize;
			madvise(_shadowMemBegin, totalShadowMemSize, MADV_NOHUGEPAGE);
	}

  size_t getUsableSize(void * ptr) {
    unsigned numBagSetItem;
    PerThreadBag *bag;
    (void)getShadowObjectInfo(ptr, &bag, &numBagSetItem);

    //void * addrEnd = (void *)((uintptr_t)addr + bag->classSize);
    //PRDBG("thread %u bag %u set %u freeing object %p ~ %p",
    //  bag->threadIndex, bag->bagNum, numBagSetItem, addr, addrEnd);
    #ifdef USE_CANARY
    return (bag->classSize - 1);
    #else
    return bag->classSize;
    #endif
  }

	// The major routine of allocate a small object 
	void * allocateSmallObject(size_t sz) {
		#ifdef CUSTOMIZED_STACK
		int threadIndex = getThreadIndex(&sz);
		#else
		int threadIndex = getThreadIndex();
		#endif
		size_t classSize;
		void * ptr;		

		#ifdef USE_CANARY
		sz++;		// make room for the buffer overflow canary
		#endif

    if(sz <= BIBOP_MIN_BLOCK_SIZE) {
      classSize = BIBOP_MIN_BLOCK_SIZE;
    } else {
      classSize = (1ULL << 32) >> __builtin_clz(sz - 1);
    }

		// compute the bag number
		unsigned bagNum = getBagNum(classSize);

		PerThreadBag * curBag = &_threadBag[threadIndex][bagNum]; 
		shadowObjectInfo * shadowinfo = NULL;

    #if (BIBOP_BAG_SET_SIZE <= 1)
    // If there's only one posible bag we do not need to perform
    // the AND operation to select it.
    unsigned numBagSetItem = 0;
    bool useBumpPointer = false;
    #else
    unsigned randNum = getRandomNumber();
    unsigned numBagSetItem = randNum & BIBOP_BAG_SET_MASK;
    // There are 1-in-BIBOP_BAG_SET_RANDOMIZER odds that we will use the
    // bump pointer, despite possibly having free objects to choose from.
    bool useBumpPointer = ((randNum & BIBOP_BAG_SET_RANDOMIZER_MASK) == 0);
    #endif

		lock(curBag, numBagSetItem);
		// If yes, then alloate an object from the freelist.
		if(!IS_FREELIST_EMPTY(&curBag->freelist[numBagSetItem]) && !useBumpPointer) {
			// LTP: it is good to add this to the paper since we are using the 
			// per-bag lock, instead of using the per-thread lock.
			// Also, only the allocation from the freelist will require a lock
			shadowinfo = (shadowObjectInfo *)FREELIST_REMOVE(&curBag->freelist[numBagSetItem]);
			unlock(curBag, numBagSetItem);
			ptr = getAddrFromShadowInfo(shadowinfo, curBag);
		} else {
			unlock(curBag, numBagSetItem);

			char ** position = &curBag->position[numBagSetItem];

			// Save the current value of the position pointer, as this will be used to allocate
			// the object requested by the caller. The position pointer will then be modified to
			// point to the next available object.
			ptr = *position;

			incrementBumpPointer(curBag, numBagSetItem);

			#ifdef RANDOM_GUARD
			if(((uintptr_t)*position & PageMask) == 0) {
					tryRandomGuardPage(curBag, numBagSetItem);
			}
			#endif
		}

		shadowinfo = getShadowObjectInfo(ptr, curBag);

		//void * ptrEnd = (void *)((uintptr_t)ptr + curBag->classSize);
		//char * canary_dbg = (char *)ptr + curBag->classSize - 1;
		//PRDBG("thread %u bag %u set %u malloc size %zu(%zu) @ %p ~ %p (canary @ %p, sm @ %p)",
		//	curBag->threadIndex, curBag->bagNum, numBagSetItem, sz, curBag->classSize,
		//	ptr, ptrEnd, canary_dbg, shadowinfo);

		#ifdef USE_CANARY
		char * canary = (char *)ptr + curBag->classSize - 1;
		*canary = CANARY_SENTINEL;
		#endif

		shadowinfo->listentry.next = ALLOC_SENTINEL;

		return ptr;
	}

	inline void incrementBumpPointer(PerThreadBag * curBag, unsigned numBagSetItem) {
			char ** position = &curBag->position[numBagSetItem];
			char ** lastofCurBag = &curBag->lastofCurBag[numBagSetItem];

			//unsigned heapNum = getHeapNumber(*position);
			//PRDBG("thread %u bag %u set %u: heap=%u, position=%p, lastofCurBag=%p",
			//				curBag->threadIndex, curBag->bagNum, numBagSetItem, heapNum, *position,
			//				*lastofCurBag);

			// Check whether this object is the last one on the current bag...
			if(*position < *lastofCurBag) {
					// We're still inside the current bag.
					*position += curBag->classSize;
			} else {
					// We will now point to the next heap.
					*position += curBag->nextHeapObjectOffset;

					#ifdef ENABLE_GUARDPAGE
					// check whether only one object is in bag
					size_t guardoffset = curBag->guardoffset;
					// set a guard page at the end of this new bag
					setGuardPage(*position, curBag->guardsize, guardoffset);
					#else
					size_t guardoffset = 0;
					#endif

					//void * oldValue = *lastofCurBag;
					*lastofCurBag = getLastOfBag(*position, guardoffset, curBag->classSize);
					//unsigned heapNum = getHeapNumber(*position);
					//PRDBG("thread %u bag %u set %u: moved to heap number %u: bag start=%p, lastofCurBag=%p",
					//				curBag->threadIndex, curBag->bagNum, numBagSetItem, heapNum, oldValue, *lastofCurBag);
			}
	}

	// Tries to place a random guard page at the current location
	// of the specified bag.
	bool tryRandomGuardPage(PerThreadBag * curBag, unsigned numBagSetItem) {
			char ** position = &curBag->position[numBagSetItem];
			void * savedPosition = (void *)curBag->position[numBagSetItem];
			size_t classSize = curBag->classSize;

			if(getRandomNumber() < RANDOM_GUARD_RAND_CUTOFF) {
					size_t guardSize;
					if(classSize < PageSize) {
							guardSize = PageSize;
							unsigned numObjectsPerPageLessOne = PageSize / classSize - 1;
							// For the purposes of the caller (allocateSmallObject()), we want
							// it to assume we are operating on the last object of this page;
							// thus, we should increment the bump pointer by the number of
							// objects that make up a page, minus one.
							*position += numObjectsPerPageLessOne * classSize; 
					} else {
							guardSize = classSize;
					}
					mprotect(savedPosition, guardSize, PROT_NONE);
					/*
					if(mprotect(savedPosition, guardSize, PROT_NONE) == -1) {
							PRERR("mprotect(%p, %zu, PROT_NONE) failed: %s", savedPosition, guardSize, strerror(errno));
					}
					*/

					incrementBumpPointer(curBag, numBagSetItem);
					return true;
			}
			return false;
	}

  size_t getObjectSize(void * addr) {
    if(isInvalidAddr(addr)) {
      return -1;
    }

    PerThreadBag *bag;
    getShadowObjectInfo(addr, &bag);

    // Return the bag's class size in place of the object's actual size.
		#ifdef USE_CANARY
    return (bag->classSize - 1);
		#else
    return bag->classSize;
		#endif
    //return (bag->classSize - 1);
  }

	inline bool isObjectFree(shadowObjectInfo * shadowinfo) {
		return(shadowinfo->listentry.next != ALLOC_SENTINEL);
	}

	void freeSmallObject(void * addr) {
		unsigned numBagSetItem;
		PerThreadBag *bag;
		shadowObjectInfo * shadowinfo = getShadowObjectInfo(addr, &bag, &numBagSetItem);

		//void * addrEnd = (void *)((uintptr_t)addr + bag->classSize);
		//PRDBG("thread %u bag %u set %u freeing object %p ~ %p",
		//	bag->threadIndex, bag->bagNum, numBagSetItem, addr, addrEnd);

		if(isObjectFree(shadowinfo)) {
			PRERR("Double free or invalid free problem found on object %p (sm %p)", addr, shadowinfo);
      printCallStack();
      exit(EXIT_FAILURE);
		} else {
				#ifdef DESTROY_ON_FREE
				#ifdef USE_CANARY
				size_t objectSizeWoCanary = bag->classSize - 1;
				#else
				size_t objectSizeWoCanary = bag->classSize;
				#endif
				#endif

				#ifdef USE_CANARY
				char * canary = (char *)addr + bag->classSize - 1;
				if(*canary != CANARY_SENTINEL) {
						FATAL("canary value for object %p not intact; canary @ %p, value=0x%x",
										addr, canary, *canary);
				}
				#if (NUM_MORE_CANARIES_TO_CHECK > 0)
				for(int move = LEFT; move <= RIGHT; move++) {
						shadowObjectInfo * canaryShadow = shadowinfo;
						for(int pos = 0; pos < NUM_MORE_CANARIES_TO_CHECK; pos++) {
								if((canaryShadow = getNextCanaryNeighbor(canaryShadow, bag, (direction)move))) {
										char * neighborAddr = (char *)getAddrFromShadowInfo(canaryShadow, bag);
										char * canary = neighborAddr + bag->classSize - 1;
										// We will only inspect the canary of objects currently in-use; if the
										// object is free, then it has already been checked previously.
										if(!isObjectFree(canaryShadow) && (*canary != CANARY_SENTINEL)) {
												FATAL("canary value for object %p (neighbor of %p) not intact; canary @ %p, value=0x%x",
																neighborAddr, addr, canary, *canary);
										}
								} else {
										// getNextCanaryNeighbor will only return null when we attempt to move
										// left from the first object in a bag within a heap that is less
										// than BIBOP_BAG_SET_SIZE. This indicates there are no previous heaps
										// we can move to. In this case, simply stop trying to move left.
										break;
								}
						}
				}
			#endif
			#endif
			#ifdef DESTROY_ON_FREE
			destroyObject(addr, objectSizeWoCanary);
			#endif

			#ifdef CFREELIST
			#ifdef CUSTOMIZED_STACK
			int threadIndex = getThreadIndex(&bag);
			#else
			int threadIndex = getThreadIndex();
			#endif

			if(bag->threadIndex == threadIndex) {
				// Add the current object directly into my own freelist.
				FREELIST_INSERT(&shadowinfo->listentry, &bag->freelist[numBagSetItem]);
			} else {
				lock(bag, numBagSetItem);
				// Add the current object into the thread's cached free list.
				#if (BIBOP_BAG_SET_SIZE > 1)
				#warning there is a known bug with using the cached freelist (CFREELIST) \
					feature; each bag set must have its own cached freelist, rather than \
					sharing a single list between multiple bag sets. If CFREELIST is combined \
					with a value of BIBOP_BAG_SET_SIZE > 1, a race condition will exist that \
					causes different threads freeing objects to this bag to each attempt to \
					manipulate the cached freelist concurrently, causing corruption of the list \
					and resulting in segfault. We must either never use these features together, \
					or add support for multiple cached freelists per bag.  -- SAS
				#endif
				insertSLLHead(&shadowinfo->listentry, &bag->cfreelist);
				bag->ncfree++;
				if(bag->ncfree > bag->cflthreshold) {
						realFreeCurrentList(bag, numBagSetItem);
				}
				unlock(bag, numBagSetItem); 
			}
			#else
			lock(bag, numBagSetItem);
			FREELIST_INSERT(&shadowinfo->listentry, &bag->freelist[numBagSetItem]);
			unlock(bag, numBagSetItem);
			#endif
		}

		return;
	}

	bool isSmallObject(void * addr) {
		return ((char *)addr >= _heapBegin && (char *)addr <= _heapEnd);
	}


private:
	inline void lock(PerThreadBag *bag, unsigned numBagSetItem) { pthread_spin_lock(&bag->listlock[numBagSetItem]); }
	inline void unlock(PerThreadBag *bag, unsigned numBagSetItem) { pthread_spin_unlock(&bag->listlock[numBagSetItem]); }

	#ifdef DESTROY_ON_FREE
	inline void destroyObject(void * addr, size_t classSize) {
			#warning destroy-on-free only applies to objects <= 2KB in size
				if(classSize <= TWO_KILOBYTES) {
						memset(addr, 0, classSize);
				}
	}
	#endif

	inline int getRandomNumber() {
		int retVal;

    #ifdef SSE2RNG
    #warning using sse2rng routine rather than libc rand
    unsigned randNum[4];
    rand_sse(randNum);
		retVal = randNum[0];
    #elif ARC4RNG
    #warning using arc4rng routine rather than libc rand
    retVal = arc4random_uniform(RAND_MAX);
    #else
    #warning using libc random number generator
    retVal = rand();
    #endif

		return retVal;
	}

	inline shadowObjectInfo * getNextCanaryNeighbor(shadowObjectInfo * shadowinfo, PerThreadBag * bag, direction move) {
			unsigned heapNum = getHeapNumber(shadowinfo);
			ptrdiff_t shadowOffset = (char *)shadowinfo - _shadowMemBegin;
			unsigned long objectindex = ((shadowOffset & _shadowMemSizePerHeapMask) - bag->startShadowMemOffset) >>
					_shadowObjectInfoSizeShiftBits;

			//PRDBG("getneighbor: t%3u/b%2u, sm %p, objectindex %lu, dir=%u",
			//	bag->threadIndex, bag->bagNum, shadowinfo, objectindex, move);

			if(move == LEFT) {
					if(objectindex == 0) {
							// Check whether we are physically capable of moving to the left; if not, return null
							if(heapNum < BIBOP_BAG_SET_SIZE) {
									//PRDBG("thread %u bag %u: no more canary neighbors to check; sm %p in heap %u",
									//				bag->threadIndex, bag->bagNum, shadowinfo, heapNum);
									return NULL;
							}

							// We must move to the last object in the previous heap
							//shadowObjectInfo * shadowinfoOld = shadowinfo;
							shadowinfo = (shadowObjectInfo *)((char *)shadowinfo - bag->nextShadowHeapObjectOffset);
							//PRDBG("thread %u bag %u: canary neighbor moving to prev heap sm %p -> sm %p, offset=%zu",
							//				bag->threadIndex, bag->bagNum, shadowinfoOld, shadowinfo, bag->nextShadowHeapObjectOffset);
					} else {
							shadowinfo--;
					}
			} else {
					// Check to see if we reached the index of the last object in this bag
					if(objectindex == bag->lastObjectIndex) {
							// We must move to the first object in the next heap
							//shadowObjectInfo * shadowinfoOld = shadowinfo;
							shadowinfo = (shadowObjectInfo *)((char *)shadowinfo + bag->nextShadowHeapObjectOffset);
							//PRDBG("thread %u bag %u: canary neighbor moving to next heap sm %p -> sm %p, offset=%zu",
							//				bag->threadIndex, bag->bagNum, shadowinfoOld, shadowinfo, bag->nextShadowHeapObjectOffset);
					} else {
							shadowinfo++;
					}
			}

			return shadowinfo;
	}

	inline char * getLastOfBag(char * start, size_t guardoffset, size_t classSize) {
			return start + _bibopBagSize - guardoffset - classSize;
	}

	inline unsigned int getBagNum(size_t classSize) {
		return LOG2(classSize) - _firstBagPower;
	}	

	inline unsigned getHeapNumber(shadowObjectInfo * shadowinfo) {
		ptrdiff_t shadowOffset = (char *)shadowinfo - _shadowMemBegin;
		unsigned heapIndex = shadowOffset >> _shadowMemSizePerHeapCeilShiftBits;

		return heapIndex;
	}

	inline unsigned getHeapNumber(char * addr) {
		unsigned long offset = addr - _heapBegin;
		unsigned globalBagNum = offset >> _bagShiftBits;
		unsigned heapIndex = globalBagNum >> _numBagsPerHeapShiftBits;

		return heapIndex;
	}

	inline shadowObjectInfo * getShadowObjectInfo(void * addr, PerThreadBag ** bag, unsigned * numBagSetItem = NULL) {
		unsigned long offset = (char *)addr - _heapBegin;
    unsigned long localHeapOffset = offset & _heapMask;
    unsigned long localBagOffset = offset & _bagMask;
		unsigned long globalBagNum = offset >> _bagShiftBits;
		unsigned long heapIndex = globalBagNum >> _numBagsPerHeapShiftBits;

		if(numBagSetItem) {
			#if (BIBOP_BAG_SET_SIZE <= 1)
			*numBagSetItem = 0;
			#else
			*numBagSetItem = heapIndex & BIBOP_BAG_SET_MASK;
			#endif
		}

		//PRDBG("addr=%p, _heapBegin=%p, offset=0x%lx, localHeapOffset=0x%lx, localBagOffset=0x%lx, globalBagNum=%lu, heapIndex=%lu, _heapMask=0x%lx, _bagMask=0x%lx, _numBagsPerHeapShiftBits=%u, _numBagsPerSubHeapMask=0x%lx, bag num result=%lu",
		//	addr, _heapBegin, offset, localHeapOffset, localBagOffset, globalBagNum, heapIndex, _heapMask, _bagMask, _numBagsPerHeapShiftBits, _numBagsPerSubHeapMask, (globalBagNum & _numBagsPerSubHeapMask));

		// Now we will locate the PerThreadBag based on the bag number and heap offset.
		*bag = &_threadBag[localHeapOffset >> _threadShiftBits][globalBagNum & _numBagsPerSubHeapMask];
	
		// Check whether this is a valid address.
		// It should be aligned to the specific sizeClass at least.
		if((localBagOffset & (*bag)->classMask) != 0) {
				PRERR("Invalid object: addr %p, classSize 0x%lx, classMask 0x%lx, offset 0x%lx",
							addr, (*bag)->classSize, (*bag)->classMask, localBagOffset);
      printCallStack();
      exit(EXIT_FAILURE);
		}

		// Check whether this object is already freed or not.
		shadowObjectInfo * shadowinfo = (shadowObjectInfo *)(_shadowMemBegin + (heapIndex << _shadowMemSizePerHeapCeilShiftBits) + (*bag)->startShadowMemOffset);

		return &shadowinfo[localBagOffset >> (*bag)->shiftBits];
	}

	inline shadowObjectInfo * getShadowObjectInfo(void * addr, PerThreadBag * bag, bool debug = false) {
		unsigned long offset = (char *)addr - _heapBegin;
    unsigned long localBagOffset = offset & _bagMask;
		unsigned long globalBagNum = offset >> _bagShiftBits;
		unsigned long heapIndex = globalBagNum >> _numBagsPerHeapShiftBits;

		shadowObjectInfo * bagShadowInfo = (shadowObjectInfo *)(_shadowMemBegin + (heapIndex << _shadowMemSizePerHeapCeilShiftBits) + bag->startShadowMemOffset);

		#ifdef DEBUG
		shadowObjectInfo * retval = &bagShadowInfo[localBagOffset >> bag->shiftBits];
		if(!debug) {
				void * checkAddr = getAddrFromShadowInfo(retval, bag, true);
				if(checkAddr != addr) {
						PRERR("ERROR: getShadowObjectInfo: self-check failed");
						PRERR("addr=%p, offset into heap=%lu, localBagOffset=%lu, globalBagNum=%lu, heapIndex=%lu, bagShadowInfo=%p, retval=%p",
							addr, offset, localBagOffset, globalBagNum, heapIndex, bagShadowInfo, retval);
						PRERR("checkAddr=%p, threadIndex=%u, bagNum=%u, PerThreadBag@%p",
										checkAddr, bag->threadIndex, bag->bagNum, bag);
						for(int i = 0; i < BIBOP_BAG_SET_SIZE; i++) {
						PRERR("\t lastofCurBag[%d]=%p",
										i, bag->lastofCurBag[i]);
						}
						raise(SIGUSR2);
				}
		}
		return retval;
		#else
		return &bagShadowInfo[localBagOffset >> bag->shiftBits];
		#endif
	}

	inline void * getAddrFromShadowInfo(shadowObjectInfo * shadowaddr, PerThreadBag * bag, bool debug = false) {
		ptrdiff_t shadowOffset = (char *)shadowaddr - _shadowMemBegin;
		unsigned long objectindex;
		unsigned long heapOffset = 0;
		unsigned long heapIndex;

		// If the bag is not in the first heap.
		heapIndex = shadowOffset >> _shadowMemSizePerHeapCeilShiftBits;
		objectindex = ((shadowOffset & _shadowMemSizePerHeapMask) - bag->startShadowMemOffset) >> _shadowObjectInfoSizeShiftBits;
		heapOffset = heapIndex << _heapSizeShiftBits; 

		#ifdef DEBUG
		if(!debug) {
				void * retval = (void *)(_heapBegin + heapOffset + bag->startOffset + (objectindex << bag->shiftBits));
				shadowObjectInfo * checkAddr = getShadowObjectInfo(retval, bag, true);
				if(checkAddr != shadowaddr) {
						PRERR("ERROR: getAddrFromShadowInfo: self-check failed");
						PRERR("shadowaddr=%p, shadowOffset=%lu, heapIndex=%lu, objectindex=%lu, heapOffset=%lu, retval=%p",
							shadowaddr, shadowOffset, heapIndex, objectindex, heapOffset, retval);
						PRERR("checkAddr=%p", checkAddr);
						raise(SIGUSR2);
				}
		}
		return retval;
		#else
		return (void *)(_heapBegin + heapOffset + bag->startOffset + (objectindex << bag->shiftBits));
		#endif
	}	

	#ifdef CFREELIST
  inline void realFreeCurrentList(PerThreadBag * bag, unsigned numBagSetItem) {
		FREELIST_MERGE(&bag->cfreelist, &bag->freelist[numBagSetItem]);
    bag->ncfree = 0;
    initSLL(&bag->cfreelist);
  }
	#endif

	#ifdef ENABLE_GUARDPAGE
  inline int setGuardPage(void * bagStartAddr, size_t guardsize, size_t guardoffset) {
    if(guardsize == 0) { return 0; }

    uintptr_t guardaddr = (uintptr_t)bagStartAddr + _bibopBagSize;
		guardaddr -= guardoffset;

		//uintptr_t endAddr = guardaddr + guardsize;
		//ptrdiff_t diff = endAddr - guardaddr;
		//PRDBG("setGuardPage: mprotect region 0x%lx ~ 0x%lx, size=%lu", guardaddr, endAddr, diff);

		/*
    int mresult = mprotect((void *)guardaddr, guardsize, PROT_NONE); 
		if(mresult == -1) {
				PRERR("mprotect failed: %s", strerror(errno));
		}
		return mresult;
		*/

    return mprotect((void *)guardaddr, guardsize, PROT_NONE); 
  }
	#endif

	inline bool isInvalidAddr(void * addr) {
		return !((char *)addr >= _heapBegin && (char *)addr <= _heapEnd);
	}
};
#endif
