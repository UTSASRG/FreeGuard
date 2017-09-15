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


Building FreeGuard
-------------------------

To build FreeGuard, simply run `make SSE2RNG=1`.

	% make SSE2RNG=1

This will incorporate Intel's fast, SSE2-optimized random number generator (RNG)
into FreeGuard, which will dramatically increase its performance in some applications.

Alternatively, when built with no additional flags (i.e., simply `make`), FreeGuard
will utilize the default C library rand() function instead.

You can then use FreeGuard by either linking it to your executable, or
by setting the `LD_PRELOAD` environment variable, as in:

	% export LD_PRELOAD=/path/to/libfreeguard.so

After doing so, all executables run in the same shell will automatically utilize
FreeGuard. Alternatively, we can specify that only a given application be linked to
FreeGuard, like so:

	% LD_PRELOAD=/path/to/libfreeguard.so /app/to/run


Technical Information
---------------------

For technical details of FreeGuard, please refer to [FreeGuard: A
Faster Secure Heap Allocator](https://arxiv.org/abs/1709.02746),
by Sam Silvestro, Hongyu Liu, Corey Crosser, Zhiqiang Lin, and Tongping
Liu. The 24th ACM Conference on Computer and Communications Security
(CCS'17). Dallas, TX, October 2017.
