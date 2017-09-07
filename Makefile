SRCS = real.cpp							\
			 xthread.cpp          \
			 libfreeguard.cpp

INCS = bibopheap.hh				\
		bigheap.hh						\
		dlist.h               \
		hashfuncs.hh					\
		hashheapallocator.hh	\
		hashmap.hh						\
		list.hh								\
		log.hh								\
		mm.hh									\
		real.hh								\
		slist.h               \
		threadstruct.hh				\
		xdefines.hh						\
		xthread.hh

DEPS = $(SRCS) $(INCS)

CXX = clang++ 

ifndef DEBUG_LEVEL
DEBUG_LEVEL = 3
endif

#CFLAGS += -O2 -Wall --std=c++11 -g -fno-omit-frame-pointer -DDEBUG_LEVEL=$(DEBUG_LEVEL)
#CFLAGS += -O2 -Wall --std=c++11 -g -fno-omit-frame-pointer -DDEBUG_LEVEL=$(DEBUG_LEVEL) -DCUSTOMIZED_STACK -DENABLE_GUARDPAGE -DCFREELIST
#CFLAGS += -O2 -Wall --std=c++11 -g -fno-omit-frame-pointer -DDEBUG_LEVEL=$(DEBUG_LEVEL) -DCUSTOMIZED_STACK -DENABLE_GUARDPAGE -DRANDOM_GUARD
#CFLAGS += -O2 -Wall --std=c++11 -g -fno-omit-frame-pointer -DDEBUG_LEVEL=$(DEBUG_LEVEL) -DCUSTOMIZED_STACK -DENABLE_GUARDPAGE -DRANDOM_GUARD
CFLAGS += -O2 -Wall --std=c++11 -g -fno-omit-frame-pointer -DNDEBUG -DCUSTOMIZED_STACK -DENABLE_GUARDPAGE -DRANDOM_GUARD -DUSE_CANARY -DFIFO_FREELIST
CFLAGS2 = -O2 --std=c++11 -g -fno-omit-frame-pointer

# NDEBUG overrules DEBUG_LEVEL macro in log.hh
ifdef NDEBUG
CFLAGS += -DNDEBUG
endif
#ifdef MANYBAGS
CFLAGS += -DMANYBAGS
#endif

INCLUDE_DIRS = -I. -I/usr/include/x86_64-linux-gnu/c++/4.8/ -I./rng
LIBS     := dl pthread

ifdef SSE2RNG
CFLAGS += -DSSE2RNG -msse2
SRCS += rng/sse2rng.c
endif
ifdef ARC4RNG
CFLAGS += -DARC4RNG
SRCS += rng/arc4random.o rng/arc4random_uniform.o
TARGETS = arc4random.o arc4random_uniform.o $(TARGETS)
endif

TARGETS = libfreeguard.so

all: $(TARGETS)

rng/arc4random.o: rng/arc4random.c
	clang -fPIC -O2 -DNDEBUG -I. -c rng/arc4random.c -o rng/arc4random.o

rng/arc4random_uniform.o: rng/arc4random_uniform.c
	clang -fPIC -O2 -DNDEBUG -I. -c rng/arc4random_uniform.c -o rng/arc4random_uniform.o


libfreeguard.so: $(DEPS)
	$(CXX) $(CFLAGS) $(INCLUDE_DIRS) -shared -fPIC $(SRCS) -o libfreeguard.so -ldl -lpthread -lrt

clean:
	rm -f $(TARGETS)
