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
 * @file   real.hh: provides wrappers to glibc thread & memory functions.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __REAL_HH_
#define __REAL_HH_

#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define DECLARE_WRAPPER(name) extern decltype(::name) * name;

namespace Real {
	void initializer();
	DECLARE_WRAPPER(free);
//	DECLARE_WRAPPER(calloc);
	DECLARE_WRAPPER(malloc);
//	DECLARE_WRAPPER(realloc);
  DECLARE_WRAPPER(pthread_create);
  DECLARE_WRAPPER(pthread_join);
  DECLARE_WRAPPER(pthread_kill);
};

#endif
