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
 * @file   threadstruct.hh: internal per-thread structure.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __THREADSTRUCT_HH__
#define __THREADSTRUCT_HH__

#include <stddef.h>
#include <ucontext.h>
#include <pthread.h>
#include "xdefines.hh"

extern "C" {
		typedef void * threadFunction(void *);
		typedef struct thread {
				// Whether the entry is available so that allocThreadIndex can use this one
				bool available;

				// Identifications
				pid_t tid;
				pthread_t pthreadt;
				int index;

				// Only used in thread joining so that my parent can wait on it.
				pthread_spinlock_t spinlock;

				// Starting parameters
				threadFunction * startRoutine;
				void * startArg;

				// Printing buffer to avoid lock conflict. 
				// fprintf will use a shared buffer and can't be used in signal handler
				char outputBuf[LOG_SIZE];
		} thread_t;
};
#endif
