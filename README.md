# LibTSC -- A Time Sharing Coroutine Library

The LibTSC is a time sharing coroutine library for Unix like systems.

## Introduction

Coroutine is a kind of light-weight task mechanism which implemented in user space. As less resources usage and less overhead during context-switch than OS level threads or processes, the coroutines are well-suited for implementing the user level concurrent system or library.

The [Go](http://golang.org) and its ancestor [libtask](http://swtch.com/libtask/) use the coroutine mechanism to implement the user-level tasks, called Goroutine in Go and task in libtask.

The LibTSC is yet another coroutine library just like the libtask and Go.
Furthermore, our goal is to support both the multi-core platforms and time-sharing mechanism which not or not both supported in Go or libtask, or any other coroutine libraries.

In current version, we use the [ucontext](http://en.wikipedia.org/wiki/Setcontext) API defined in GNU libc to implement the coroutine, 
called `tsc_coroutine_t` in our library and use the POSIX Threads API to support the OS-level threads. 
And the per-thread signal mechanism is used to emulate the interrupt for computing-intensive coroutines, which is the basis of the time-sharing in libTSC.

## Build

LibTSC is designed for all Unix like systems, include Linux, *BSD and Darwin,
but only Linux (x86, amd64 and armv7) and Darwin (amd64) have been tested now.

It is simple to build the library and examples, just goto the root directory of the source code and type `make` in command line.

There are some building options:

- `enable_timer=1` to enable the time-sharing mechanism, which is disable default.
- `enable_debug=[0|1|2]` to set the debug level, 0 means no debug option but optimize the code with "-O2" option.
- `use_clang=1` to use the clang/llvm compiler to build the programs. This is the default setting for Darwin platform.
- `enable_splitstack=1` to enable the split-stack feature, make sure your complier (gcc 4.6.0+) and linker (GNU gold) support that feature!
- `enable_deadlock_detect=1` to enable the deadlock detecting. When all VPUs are sleep, the program will print the backtrace info for each suspended thread and quit.
- `enable_futex=1` to use the futex based lock mechanism instead of the pthread spinlock.

To build an optimized library with time-sharing support, just use the command:
		
		make enable_timer=1 enable_debug=0


## Examples

LibTSC is built as a static library now, in order to use it, you need to link it with your own programs.

Some examples are provided for users to test the library:

- **findmax.c**: for testing the basic thread and channel API
- **findmax_msg.c**: for testing the message-passing API
- **timeshare.c**: for testing the time-sharing mechanism
- **select.c**: for testing the select operation among multi-channels
- **ticker.c**: for testing the ticker/timer API
- **file.c**: for testing the file API
- **chan.c**: for testing the channel API
- **primes.c**: example migrated from libtask
- **tcpproxy.c**: example migrated from libtask
- **httpload.c**: example migrated from libtask
- **mandelbrot.c**: benchmark migrated from the [benchmarksgame.org](http://benchmarksgame.alioth.debian.org)
- **spectral-norm.c**: benchmark migrated from the [benchmarksgame.org](http://benchmarksgame.alioth.debian.org)

## Debug

We provide a python script to enhance the GDB to debug the applications using libTSC.

Make sure your GDB is compiled with the option "--with-python", 
you can open GDB and type such command to test whether python plugin is available for you:

        (gdb) python print(1 + 1)

You can go through if it output "2"; otherwise, try to re-compile your gdb.

At the beginning, you should load our simple script `src/gdb_hepler.py`:
        
        (gdb) source where/is/gdb_helper.py

then, you can use the command `info coroutines` to list all the live coroutines.

Further more, if you want to trace the stack frame of a given coroutine, you can type:

        (gdb) coroutine id backtrace

replace the *id* with the coroutine id number you want to trace, which can be found after running 
the command `info coroutines`.

If the *cmd* (*backtrace* in previous example) is omitted, and the coroutine with *id* is running, 
then you can switch to that coroutine.

This script is very simple and the functions provided are very limited now.
We hope that lots of new features will be added in the future.

## Platforms Requirement

Linux on x86/x86-64 or armv7 using the GCC toolchain,
if compiling with option `enable_splitstack=1`, you need to use GCC 4.6.0 or later, binutils 2.21 or later with `--enable-gold`.

OS X on x86-64 using the GCC or XCode toolchain,
the "splitstack" feature is not enable on OS X because we only support 64-bit arch, which not need the "splitstack".

The GDB debug helper script is available for each supported platforms, however,
make sure your GDB is version 7.0 or later and built with python enable.

## TODO

There are lots of things needed to improve both the functionality and the performance:

- A more efficient way for work-stealing among the schedulers (*A new random stealing mechanism is added by ZHJ*)
- Wrappers for the system calls which may suspend the current scheduler
- Asynchronous API for socket IO operations (**DNOE**)
- [Segment-stack](http://gcc.gnu.org/wiki/SplitStacks) support for coroutines (**DONE**)
- A more efficient memory management module, like [tc_malloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html)
- Garbage-Collection mechanism for auto deallocation (*A simple reference-counting mechanism is provided now*)

