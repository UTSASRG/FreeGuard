# FreeGuard
FreeGuard: A Faster Secure Heap Allocator
--------------------------

Copyright (C) 2017 by [Sam Silvestro](sam.silvestro@utsa.edu),
[Hongyu Liu](liuhyscc@gmail.com), [Corey Crosser](corey.crosser@usma.edu),
[Zhiqiang Lin](zhiqiang.lin@utdallas.edu), and [Tongping Liu](http://www.cs.utsa.edu/~tongpingliu/).

FreeGuard is a fast, scalable, and memory-efficient
memory allocator that works for a range of applications on Unix-compatible systems.

FreeGuard is a drop-in replacement for malloc that can dramatically
improve application performance, especially for multithreaded programs
running on multiprocessors and multicore CPUs. No source code changes are
necessary: just link it in or set the proper environment variable (see
[Building FreeGuard](#building-freeguard), below).


Licensing
---------

FreeGuard is distributed under the GPL (v2.0), and can also be licensed
for commercial use.

Because of the restrictions imposed by the GPL license (you must make
your code open-source), commercial users of FreeGuard can purchase non-GPL
licenses through the University of Texas at San Antonio.
To obtain a license, please contact Tongping Liu directly
(tongping.liu@utsa.edu) and copy Sam Silvestro (sam.silvestro@utsa.edu).
