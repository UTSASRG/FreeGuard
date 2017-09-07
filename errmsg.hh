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
 * @file   errmsg.hh: prints callstack information.
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef ERRMSG_H
#define ERRMSG_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <execinfo.h>
#include "xdefines.hh"
#include "log.hh"

#define PREV_INSTRUCTION_OFFSET 1
#define FILENAMELENGTH 256

void printCallStack(){
  void* array[256];
  int frames = backtrace(array, 256);

  char main_exe[FILENAMELENGTH];
  readlink("/proc/self/exe", main_exe, FILENAMELENGTH);

#ifdef CUSTOMIZED_STACK
  int threadIndex = getThreadIndex(&frames);
#else
  int threadIndex = getThreadIndex();
#endif

  char buf[256];
  for(int i = 0; i < frames; i++) {
    void* addr = (void*)((unsigned long)array[i] - PREV_INSTRUCTION_OFFSET);
    PRINT("\tThread %d: callstack frame %d: %p\t", threadIndex, i, addr);
    sprintf(buf, "addr2line -a -i -e %s %p", main_exe, addr);
    system(buf);
  }
}

#endif
