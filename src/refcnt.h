#ifndef _TSC_REFCNT_H_
#define _TSC_REFCNT_H_

typedef void (*release_handler_t)(void *);

typedef struct tsc_refcnt {
  uint32_t count;
  release_handler_t release;
} *tsc_refcnt_t;

static inline void tsc_refcnt_init(tsc_refcnt_t ref,
                                   release_handler_t release) {
  ref->count = 0;
  ref->release = release;
}

static inline void *tsc_refcnt_get(tsc_refcnt_t ref) {
  TSC_ATOMIC_INC(ref->count);
  return (void *)ref;
}

static inline void tsc_refcnt_put(tsc_refcnt_t ref) {
  if (TSC_ATOMIC_DEC(ref->count) == 0) (ref->release)(ref);
}

#endif  // _TSC_REFCNT_H_
