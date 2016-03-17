// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_GROUP_H_
#define _TSC_GROUP_H_

#include "support.h"
#include "coroutine.h"

/* the structure for subtask group control */
typedef struct coroc_group {
  uint32_t count;
  uint32_t errors;
  coroc_lock lock;
  coroc_coroutine_t parent;
} *coroc_group_t;

/// Init the coroc_group_t 
static inline 
void coroc_group_init(coroc_group_t group) {
  assert(group != NULL);

  lock_init(& group->lock);
  group->count = 1;
  group->errors = 0;
  // this field will be assgined when parent task
  // calls the `coroc_group_sync' ..
  group->parent = NULL; 
}

/// Alloc the group from heap and init it
coroc_group_t coroc_group_alloc(void);

/// Called when parent spawn a new subtask into this group
void coroc_group_add_task(coroc_group_t group);

/// Called when the subtask finish its work, notify the parent
void coroc_group_notify(coroc_group_t group, int retval);

/// Called by the parent to check if all subtasks are finish
bool coroc_group_check(coroc_group_t group);

/// Called by the parent, this will block the parent until all
/// subtasks are finish and call coroc_group_notify().
/// NOTE: this call will release the group, means that the group
///       CANNOT use more than once!!
int coroc_group_sync(coroc_group_t group);

#endif /* _TSC_GROUP_H_ */
