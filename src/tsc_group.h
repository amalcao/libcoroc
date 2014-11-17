#ifndef _TSC_GROUP_H_
#define _TSC_GROUP_H_

#include "support.h"
#include "coroutine.h"

/* the structure for subtask group control */
typedef struct {
  uint32_t count;
  lock lock;
  tsc_coroutine_t parent;
} tsc_group_t;

/// Init the tsc_group_t 
static inline 
void tsc_group_init(tsc_group_t *group) {
  assert(group != NULL);

  lock_init(& group->lock);
  group->count = 0;
  // this field will be assgined when parent task
  // calls the `tsc_group_sync' ..
  group->parent = NULL;
}

#if USE_FUTEX_LOCK
# define TSC_GROUP_INITIALIZER \
  { .count = 0, .lock = 0, .parent = NULL } 
#endif // USE_FUTEX_LOCK

/// Called when parent spawn a new subtask into this group
static inline
void tsc_group_add_task(tsc_group_t *group) {
  assert(group != NULL);
  TSC_ATOMIC_INC(& group->count);
}

/// Called when the subtask finish its work, notify the parent
void tsc_group_notify(tsc_group_t *group);

/// Called by the parent to check if all subtasks are finish
bool tsc_group_check(tsc_group_t *group);

/// Called by the parent, this will block the parent until all
/// subtasks are finish and call tsc_group_notify().
void tsc_group_sync(tsc_group_t *group);

#endif /* _TSC_GROUP_H_ */
