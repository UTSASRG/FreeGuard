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
 * @file   xthread.hh: thread-related functions implementation.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __XTHREAD_HH__
#define __XTHREAD_HH__

#include <unistd.h>
#include <new>
#include <sys/syscall.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "hashmap.hh"
#include "hashfuncs.hh"
#include "hashheapallocator.hh"
#include "log.hh"
#include "real.hh"
#include "threadstruct.hh"
#include "xdefines.hh"
#ifdef SSE2RNG
#include "sse2rng.h"
#endif

#ifdef CUSTOMIZED_STACK
extern intptr_t globalStackAddr;
#endif

class xthread {

	public:
	static xthread& getInstance() {
    static char buf[sizeof(xthread)];
    static xthread* xthreadObject = new (buf) xthread();
    return *xthreadObject;
  }

	void initialize() {
		_aliveThreads = 0;
    _threadIndex = 0;
    _totalThreads = MAX_ALIVE_THREADS;

		// Initialize the spin_lock
		pthread_spin_init(&_spin_lock, PTHREAD_PROCESS_PRIVATE);

    thread_t * thread;

    // Shared the threads information.
    memset(&_threads, 0, sizeof(_threads));

		// Initialize all 
    for(int i = 0; i < MAX_ALIVE_THREADS; i++) {
      thread = &_threads[i];

			// Those information that are only initialized once.
     	thread->available = true;
			thread->index = i;
	 }

		// Now we will intialize the initial thread
		initializeInitialThread();

		_xmap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, THREAD_MAP_SIZE);
		_xmap.insert((void*)pthread_self(), sizeof(void*), 0);
  }

  void finalize() {}
	
	void initializeInitialThread(void) {
		int tindex = allocThreadIndex();
   	assert(tindex == 0);

		thread_t * thread = getThread(tindex);
	
		// Initial myself, like threadIndex, tid
		initializeCurrentThread(thread);
	
		// Adding the thread's pthreadt.
		thread->pthreadt = pthread_self();
	}

	// This function will be called by allocThreadIndex, 
	// particularily by its parent (except the initial thread)
	inline void threadInitBeforeCreation(thread_t * thread) {
    pthread_spin_init(&thread->spinlock, 0);
  }

	// This function is only called in the current thread before the real thread function 
	void initializeCurrentThread(thread_t * thread) {
		SRAND(time(NULL));
		thread->tid = syscall(__NR_gettid);
		#ifndef CUSTOMIZED_STACK
		setThreadIndex(thread->index);
		#endif
	}

	/// @ internal function: allocation a thread index when spawning.
  /// Since we guarantee that only one thread can be in spawning phase,
  /// there is no need to acqurie the lock in this function.
  int allocThreadIndex() {
    int index = -1;

    thread_t* thread;
		if(_aliveThreads++ == _threadIndex) {
			index = _threadIndex++;
      thread = getThread(index);
			thread->available = false;
			threadInitBeforeCreation(thread);
		} else {
			for(int i = 0; i <= _threadIndex; i++) {
      	thread = getThread(i);
      	if(thread->available) {
        	thread->available = false;
					index = i;
					//threadInitBeforeCreation(thread);
					break;
				}
			}
		}
    return index;
  }

	inline void spin_lock(thread_t * thread) {
		pthread_spin_lock(&thread->spinlock);
	} 

	inline void spin_unlock(thread_t * thread) { 
		pthread_spin_unlock(&thread->spinlock); 
	}

  inline thread_t* getThread(int index) { return &_threads[index]; }
	#ifndef CUSTOMIZED_STACK
  inline thread_t* getThread() { return &_threads[getThreadIndex()]; }
	#endif

	int thread_create(pthread_t * tid, const pthread_attr_t * attr, threadFunction * fn, void * arg) {
		acquireGlobalLock();
		int tindex = allocThreadIndex();
		releaseGlobalLock();

		// Acquire the thread structure.
		thread_t* children = getThread(tindex);	
		children->startArg = arg;
		children->startRoutine = fn;
		children->index = tindex;

		#ifdef CUSTOMIZED_STACK
		pthread_attr_t iattr;
		if(attr == NULL) {
			pthread_attr_init(&iattr);
		} else {
			memcpy(&iattr, attr, sizeof(pthread_attr_t));
		}
		pthread_attr_setstack(&iattr, (void*)(globalStackAddr + (intptr_t)tindex * STACK_SIZE + GUARD_PAGE_SIZE), STACK_SIZE - 2 * GUARD_PAGE_SIZE);
		/* Guard pages have already been setted before main()
		if(0 != mprotect((void*)(childrenStackStart + STACK_SIZE - GUARD_PAGE_SIZE), GUARD_PAGE_SIZE, PROT_NONE)
				|| 0 != mprotect((void*)childrenStackStart, GUARD_PAGE_SIZE, PROT_NONE)) {
			fprintf(stderr, "Failed to set guard page for thread#%d\n", tindex);
			abort();
		}
		*/

		int result = Real::pthread_create(tid, &iattr, xthread::startThread, (void *)children);
		#else
		int result = Real::pthread_create(tid, attr, xthread::startThread, (void *)children);
		#endif

		if(result) {
			FATAL("thread_create failure");
		}
		
		// Setting up this in the main thread so that
		// pthread_join can always find its pthread_t. 
		// Otherwise, it can fail because the child have set this value.
		children->pthreadt = *tid;
		acquireGlobalLock();
		_xmap.insertIfAbsent((void*)*tid, sizeof(void*), tindex);
		releaseGlobalLock();
		return result;
	}

  static void * startThread(void * arg) {
    thread_t * current = (thread_t *) arg;

    xthread::getInstance().initializeCurrentThread(current);

		// Actually run this thread using the function call
    void * result = NULL;
    try{
      result = current->startRoutine(current->startArg);
    }
    catch (int err){
      if(err != PTHREADEXIT_CODE){
        throw err;
      }
    }

 	  return result;
  }

	int thread_join(pthread_t tid, void ** retval) {
		int joinretval;
		if((joinretval = Real::pthread_join(tid, retval)) == 0) {
				int joinee = -1;

				acquireGlobalLock();
				if(!_xmap.find((void *)tid, sizeof(void *), &joinee)) {
						PRERR("Cannot find joinee index for thread %p", (void *)tid);
				} else {
						_threads[joinee].available = true;
				}
				_aliveThreads--;
				releaseGlobalLock();
		}
		return joinretval;
	}

	void acquireGlobalLock() {
		spin_lock();
	}
	
	void releaseGlobalLock() {
		spin_unlock();
	}
	
private:
	inline void spin_lock() {
		pthread_spin_lock(&_spin_lock);
	} 

	inline void spin_unlock() { 
		pthread_spin_unlock(&_spin_lock); 
	}

	pthread_spinlock_t _spin_lock;
	// The maximum number of alive threads we can support.
  int _totalThreads;
	// The next available thread index for use by a new thread.
  int _threadIndex;
	typedef HashMap<void *, int, HeapAllocator> threadHashMap;
	threadHashMap _xmap; // The hash map that map the address of pthread_t to thread index.
	// Use _threadIndex as the total possible number of alive threads at
	// any given time during execution.
	int _aliveThreads;
  thread_t _threads[MAX_ALIVE_THREADS];
};
#endif
