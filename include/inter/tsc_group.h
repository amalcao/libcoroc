#ifndef _TSC_GROUP_H_
#define _TSC_GROUP_H_

#include "support.h"
#include "coroutine.h"

/* the structure for subtask group control */
typedef struct tsc_group {
  uint32_t count;
  uint32_t errors;
  tsc_lock lock;
  tsc_coroutine_t parent;
} *tsc_group_t;

/// Init the tsc_group_t 
static inline 
void tsc_group_init(tsc_group_t group) {
  assert(group != NULL);

  lock_init(& group->lock);
  group->count = 0;
  group->errors = 0;
  // this field will be assgined when parent task
  // calls the `tsc_group_sync' ..
  group->parent = NULL;
}

/// Alloc the group from heap and init it
tsc_group_t tsc_group_alloc(void);

/// Called when parent spawn a new subtask into this group
void tsc_group_add_task(tsc_group_t group);

/// Called when the subtask finish its work, notify the parent
void tsc_group_notify(tsc_group_t group, int retval);

/// Called by the parent to check if all subtasks are finish
bool tsc_group_check(tsc_group_t group);

/// Called by the parent, this will block the parent until all
/// subtasks are finish and call tsc_group_notify().
/// NOTE: this call will release the group, means that the group
///       CANNOT use more than once!!
int tsc_group_sync(tsc_group_t group);

#endif /* _TSC_GROUP_H_ */
