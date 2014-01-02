# LibTSC -- A Time Sharing Coroutine Library

The LibTSC is a time sharing coroutine library for Unix like systems.

## Introduction

Coroutine is a kind of light-weight task mechanism which implemented in user space. As less resources usage and less overhead during context-switch than OS level threads or processes, the coroutines are well-suited for implementing the user level concurrent system or library.

The [Go](http://golang.org) and its ancestor [libtask](http://swtch.com/libtask/) use the coroutine mechanism to implement the user-level tasks, called Goroutine in Go and task in libtask.

The LibTSC is yet another coroutine library just like the libtask and Go.
Furthermore, our goal is to support both the multi-core platforms and time-sharing mechanism which not or not both supported in Go or libtask, or any other coroutine libraries.

In current version, we use the [ucontext](http://en.wikipedia.org/wiki/Setcontext) API defined in GNU libc to implement the coroutine, called `thread_t` in our library and use the POSIX Threads API to support the OS-level threads. 
And the per-thread signal mechanism is used to emulate the interrupt for computing-intensive coroutines, which is the basis of the time-sharing in libTSC.

## Build

LibTSC is designed for all Unix like systems, include Linux, *BSD and Darwin,
but only Linux (on x86 and amd64) and Darwin (amd64) have been tested now.

It is simple to build the library and examples, just goto the root directory of the source code and type `make` in command line.

There are some building options:

- `enable_timer=1` to enable the time-sharing mechanism, which is disable default.
- `enable_optimize=1` to build the target with `-O2` option, if not use this option, the debug mode library and examples will be built.
- `use_clang=1` to use the clang/llvm compiler to build the programs. This is the default setting for Darwin platform.
- `enable_splitstack=1` to enable the split-stack feature, make sure your complier (gcc 4.6.0+) and linker (GNU gold) support that feature!

To build an optimized library with time-sharing support, just use the command:
		
		make enable_timer=1 enable_optimize=1


## Examples

LibTSC is built as a static library now, in order to use it, you need to link it with your own programs.

Some examples are provided for users to test the library:

- **findmax.c**			for testing the basic thread and channel API
- **timeshare.c**		for testing the time-sharing mechanism
- **select.c**			for testing the select operation among multi-channels
- **primes.c**	 		example migrated from libtask
- **mandelbrot.c**		benchmark migrated from the [benchmarksgame.org](http://benchmarksgame.alioth.debian.org)
- **spectral-norm.c**	benchmark migrated from the [benchmarksgame.org](http://benchmarksgame.alioth.debian.org)

## TODO

There are lots of things needed to improve both the functionality and the performance:

- A more efficient way for work-stealing among the schedulers
- Wrappers for the system calls which may suspend the current scheduler
- Asynchronous API for socket IO operations
- [Segment-stack](http://gcc.gnu.org/wiki/SplitStacks) support for coroutines (**DONE**)
- A more efficient memory management module, like [tc_malloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html)
- Garbage-Collection mechanism for auto deallocation

