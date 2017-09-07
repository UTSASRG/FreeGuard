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
 * @file   hashfuncs.hh: functions related to hash table implementation.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __HASHFUNCS_HH__
#define __HASHFUNCS_HH__

#include <stdint.h>
#include <string.h>

class HashFuncs {
public:
  // The following functions are borrowed from the STL.
  static size_t hashString(char* start, size_t len) {
  //static size_t hashString(const void* start, size_t len) {
    unsigned long __h = 0;
    char* __s = start;
    //char* __s = (char*)start;
		int i = 0;
    //auto i = 0;

    for(; i <= (int) len; i++, ++__s)
      __h = 5 * __h + *__s;

    return size_t(__h);
  }

  static size_t hashInt(const int x, size_t) { return x; }

  static size_t hashLong(long x, size_t) { return x; }

  static size_t hashUnsignedlong(unsigned long x, size_t) { return x; }

  static size_t hashAddr(void* addr, size_t) {
    unsigned long key = (unsigned long)addr;
    key ^= (key << 15) ^ 0xcd7dcd7d;
    key ^= (key >> 10);
    key ^= (key << 3);
    key ^= (key >> 6);
    key ^= (key << 2) + (key << 14);
    key ^= (key >> 16);
    return key;
  }

	static size_t hashAddrs(intptr_t* addr, size_t len) {
		size_t result = ~0;
		for(size_t i = 0; i < len; i++) result ^= addr[i];
		return result;
	}

  static bool compareAddr(void* addr1, void* addr2, size_t) { return addr1 == addr2; }

  static bool compareInt(int var1, int var2, size_t) { return var1 == var2; }

  //static bool compareString(const char* str1, const char* str2, size_t len) {
  static bool compareString(char* str1, char* str2, size_t len) {
    return strncmp(str1, str2, len) == 0;
  }
};

#endif
