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
 * @file   bigheap.hh: manages the mapping/unmapping of large objects
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __BIGHEAP_HH__
#define __BIGHEAP_HH__

#include "real.hh"
#include "xthread.hh"
#include "mm.hh"
#include "xdefines.hh"
#include "errmsg.hh"

class BigHeap {

private:
	class bigObjectStatus {
		public: 
			void * start;
			size_t size; 
			size_t pageUpSize; 

			#ifdef DETECT_UAF
			unsigned long long freedtime;
			#endif 
	};

public:
  static BigHeap &getInstance() {
      static char buf[sizeof(BigHeap)];
      static BigHeap* theOneTrueObject = new (buf) BigHeap();
      return *theOneTrueObject;
  }
	
	// Initialization of the Big Heap
	void initBigHeap(void) {
    // Initialize the spin_lock
    pthread_spin_init(&_spin_lock, PTHREAD_PROCESS_PRIVATE);

		// Initialize the hash map
    _xmap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, THREAD_MAP_SIZE);
	} 

	// For big objects, we don't have the quarantine list. 
	// Actually, the size information will be kept until new allocation is 
	void * allocateAtBigHeap(size_t size) {
		assert(IF_CANARY_CONDITION);

		size_t pageUpSize = alignup(size, PageSize);
		size_t diff = pageUpSize - size;
		bigObjectStatus * objStatus = (bigObjectStatus *)HeapAllocator::allocate(sizeof(bigObjectStatus));
		void * ptr = MM::mmapAllocatePrivate(pageUpSize, NULL);
		void * objStartPtr = (void *)((char *)ptr + diff);
		acquireGlobalLock();
    _xmap.insert(objStartPtr, sizeof(void *), objStatus);
		releaseGlobalLock();

		objStatus->start = ptr;
		objStatus->pageUpSize = pageUpSize;
		objStatus->size = size;

		//PRDBG("BigHeap returning %p (begins @ %p), size %zu (actual %zu)", objStartPtr, ptr, size, pageUpSize);
		return objStartPtr;
	}

  size_t getObjectSize(void * addr) {
    bigObjectStatus * objStatus;
		if(!_xmap.find(addr, sizeof(void *), &objStatus)) {
			return -1;
		}
    return objStatus->size;
  }

  bool isLargeObject(void * addr) {
		bigObjectStatus * objStatus;
		return(_xmap.find(addr, sizeof(void *), &objStatus));
  }

	void deallocateToBigHeap(void * ptr) {
    bigObjectStatus * objStatus;
		if(!_xmap.find(ptr, sizeof(void *), &objStatus)) {
			PRINT("invalid or double free on address %p", ptr);
      printCallStack();
      exit(-1);
		}

		//PRDBG("BigHeap freed %p (begins @ %p), size %zu (actual %zu), objStatus @ %p",
		//				ptr, objStatus->start, objStatus->size, objStatus->pageUpSize, objStatus);
		acquireGlobalLock();
		_xmap.erase(ptr, sizeof(void *));
		releaseGlobalLock();
		MM::mmapDeallocate(objStatus->start, objStatus->pageUpSize);
		HeapAllocator::deallocate(objStatus);
	}

  void acquireGlobalLock() {
    spin_lock();
  } 
  
  void releaseGlobalLock() {
    spin_unlock();
  }
  
private:
	size_t _bigObjectStatusSize = sizeof(bigObjectStatus);
	unsigned _bigObjectStatusSizeShiftBits = LOG2(_bigObjectStatusSize);
	pthread_spinlock_t _spin_lock;
	typedef HashMap<void *, bigObjectStatus *, HeapAllocator> objectHashMap;
  objectHashMap _xmap;

  inline void spin_lock() {
    pthread_spin_lock(&_spin_lock);
  }
    
  inline void spin_unlock() {
    pthread_spin_unlock(&_spin_lock);
  }
};	

#endif // __BIGHEAP_HH__	
