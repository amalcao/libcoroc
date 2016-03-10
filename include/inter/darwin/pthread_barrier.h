// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_OSX_PTHREAD_BARRIER_H
#define _TSC_OSX_PTHREAD_BARRIER_H

#ifdef __APPLE__

typedef int pthread_barrierattr_t;

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int count;
  int tripCount;
} pthread_barrier_t;

int pthread_barrier_init(pthread_barrier_t *, const pthread_barrierattr_t *,
                         unsigned int);
int pthread_barrier_destroy(pthread_barrier_t *);
int pthread_barrier_wait(pthread_barrier_t *);

#endif  // __APPLE__

#endif  // _TSC_OSX_PTHREAD_BARRIER_H
