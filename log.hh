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
 * @file   log.hh: logging and debug printing macros.
 *          Color codes from SNAPPLE: http://sourceforge.net/projects/snapple/
 * @author Tongping Liu <http://www.cs.utsa.edu/~tongpingliu/>, Charlie Curtsinger
 * @author Sam Silvestro <sam.silvestro@utsa.edu>
 */
#ifndef __LOG_HH__
#define __LOG_HH__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xdefines.hh"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#define NORMAL_CYAN "\033[36m"
#define NORMAL_MAGENTA "\033[35m"
#define NORMAL_BLUE "\033[34m"
#define NORMAL_YELLOW "\033[33m"
#define NORMAL_GREEN "\033[32m"
#define NORMAL_RED "\033[31m"

#define BRIGHT_CYAN "\033[1m\033[36m"
#define BRIGHT_MAGENTA "\033[1m\033[35m"
#define BRIGHT_BLUE "\033[1m\033[34m"
#define BRIGHT_YELLOW "\033[1m\033[33m"
#define BRIGHT_GREEN "\033[1m\033[32m"
#define BRIGHT_RED "\033[1m\033[31m"

#define ESC_INF NORMAL_CYAN
#define ESC_DBG NORMAL_GREEN
#define ESC_WRN BRIGHT_YELLOW
#define ESC_ERR BRIGHT_RED
#define ESC_END "\033[0m"

#define OUTPUT write

#ifndef NDEBUG
/**
 * Print status-information message: level 0
 */
#define PRINF(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 1) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_INF "%lx [INFO]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),    \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

/**
 * Print status-information message: level 1
 */
#define PRDBG(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 2) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_DBG "%lx [DBG]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

/**
 * Print warning message: level 2
 */
#define PRWRN(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 3) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_WRN "%lx [WRN]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }
#else
#define PRINF(fmt, ...)
#define PRDBG(fmt, ...)
#define PRWRN(fmt, ...)
#endif

/**
 * Print error message: level 3
 */
#define PRERR(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 4) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_ERR "%lx [ERR]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

// Can't be turned off. But we don't want to output those line number information.
#define PRINT(fmt, ...)                                                                            \
  {                                                                                                \
    ::snprintf(getThreadBuffer(), LOG_SIZE, BRIGHT_MAGENTA fmt ESC_END "\n", ##__VA_ARGS__);       \
    OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                   \
  }

/**
 * Print fatal error message, the program is going to exit.
 */

#define FATAL(fmt, ...)                                                                            \
  {                                                                                                \
    ::snprintf(getThreadBuffer(), LOG_SIZE,                                                        \
               ESC_ERR "%lx [FATALERROR]: %20s:%-4d: " fmt ESC_END "\n",                \
               pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__);                                 \
    OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                   \
    exit(-1);                                                                                      \
  }

// Check a condition. If false, print an error message and abort
#define REQUIRE(cond, ...)                                                                         \
  if(!(cond)) {                                                                                    \
    FATAL(__VA_ARGS__)                                                                             \
  }

#endif
