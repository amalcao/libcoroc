#ifndef _TSC_SUPPORT_NOTIFY_H_
#define _TSC_SUPPORT_NOTIFY_H_

#include <pthread.h>

#include "support.h"
#include "async.h"

// the notify API, just wrap the pthread_cont_t now!
typedef struct {
  uint32_t key;
  int64_t ns;
} tsc_notify_t;

// the internal sleep / tsleep

void tsc_notify_wakeup(tsc_notify_t *note);
void __tsc_notify_tsleep(void *);

static inline void tsc_notify_clear(tsc_notify_t *note) {
  note->key = 0;
  note->ns = 0;
}

static inline void tsc_notify_tsleep(tsc_notify_t *note, int64_t ns) {
  if (ns > 0) {
    note->ns = ns;
    __tsc_notify_tsleep(note);
  }
}

static inline void tsc_notify_tsleep_u(tsc_notify_t *note, int64_t ns) {
  // check the waiting time first
  if (ns > 0) {
    // waiting for the signal ..
    note->ns = ns;
    tsc_async_request_submit(__tsc_notify_tsleep, note);
  }
}

// user task space nanosleep call
extern void __tsc_nanosleep(void *);
static inline void tsc_nanosleep(uint64_t ns) {
  if (ns == 0) return;
  tsc_async_request_submit(__tsc_nanosleep, &ns);
}

#endif  // _TSC_SUPPORT_NOTIFY_H_
