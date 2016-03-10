// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include "tsc_group.h"
#include "vpu.h"

tsc_group_t tsc_group_alloc(void) {
  tsc_group_t group = TSC_ALLOC(sizeof(struct tsc_group));
  assert(group != NULL);
  tsc_group_init(group);
  return group;
}

#if 0
void tsc_group_add_task(tsc_group_t group) {
  assert(group != NULL);
  // TSC_ATOMIC_INC(group->count);
    lock_acquire(& group->lock);
  group->count++;
    lock_release(& group->lock);
}

void tsc_group_notify(tsc_group_t group, int retval) {
  assert(group != NULL);
  tsc_coroutine_t parent = NULL;

    lock_acquire(& group->lock);
  // increase the error number
  if (retval != 0)
    group->errors++; // TSC_ATOMIC_INC(group->errors);

  // decrease the counter, if reach zero, 
  // try to wakeup the parent task!!
  // if (TSC_ATOMIC_DEC(group->count) == 0) {
  if (--(group->count) == 0) {
    parent = group->parent;
  }
    lock_release(& group->lock);
  
  if (parent != NULL)
    vpu_ready(parent);

  return;
}

bool tsc_group_check(tsc_group_t group) {
  assert(group != NULL);
  return TSC_ATOMIC_READ(group->count) == 0;
}

int tsc_group_sync(tsc_group_t group) {
#if 0
  // if all subtasks are finish, no need to sleep..
  if (tsc_group_check(group)) 
    goto __quit_sync;
#endif
  lock_acquire(& group->lock);
  // FIXME: Do we need to check again to avoid errors?
  if (/*TSC_ATOMIC_READ*/(group->count) == 0) {
    lock_release(& group->lock);
    goto __quit_sync;
  }
  group->parent = tsc_coroutine_self();
  // goto sleep and wait the last finished subtask to 
  // wakeup me!!
  vpu_suspend(& group->lock,
              (unlock_handler_t)lock_release);

__quit_sync:
  return group->errors;
}
#else

void tsc_group_add_task(tsc_group_t group) {
  assert(group != NULL);
  TSC_ATOMIC_INC(group->count);
}

void tsc_group_notify(tsc_group_t group, int retval) {
  assert(group != NULL);
  tsc_coroutine_t parent;
  
  if (retval != 0) 
    TSC_ATOMIC_INC(group->errors);

  if (TSC_ATOMIC_DEC(group->count) == 0) {
    lock_acquire(& group->lock);
      parent = group->parent;
    lock_release(& group->lock);
    
    if (parent != NULL) 
      vpu_ready(parent, true);
  }
  return;
}

int tsc_group_sync(tsc_group_t group) {
  assert(group != NULL);

  if (TSC_ATOMIC_DEC(group->count) > 0) {
    lock_acquire(& group->lock);
    if (TSC_ATOMIC_READ(group->count) > 0) {
      group->parent = tsc_coroutine_self();
      vpu_suspend(& group->lock, 
                  (unlock_handler_t)lock_release);
    }
  }

  return group->errors;
}

#endif
