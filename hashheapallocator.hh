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
 * @file   hashheapallocator.hh: support functions allocate memory for hash table.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __HASHHEAPALLOCATOR_HH__
#define __HASHHEAPALLOCATOR_HH__

#include "real.hh"

class HeapAllocator {
public:
  static void* allocate(size_t sz) {
    void* ptr = Real::malloc(sz);
    if(!ptr) {
      return NULL;
    }
    return ptr;
  }

  static void deallocate(void* ptr) {
    Real::free(ptr);
    ptr = NULL;
  }
};
#endif
