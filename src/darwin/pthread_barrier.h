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
