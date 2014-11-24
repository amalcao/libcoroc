#include "tsc_group.h"
#include "vpu.h"

tsc_group_t tsc_group_alloc(void) {
  tsc_group_t group = TSC_ALLOC(sizeof(struct tsc_group));
  assert(group != NULL);
  tsc_group_init(group);
  return group;
}

void tsc_group_add_task(tsc_group_t group) {
  assert(group != NULL);
  TSC_ATOMIC_INC(group->count);
}

void tsc_group_notify(tsc_group_t group) {
  assert(group != NULL);
  // decrease the counter, if reach zero, 
  // try to wakeup the parent task!!
  if (TSC_ATOMIC_DEC(group->count) == 0) {
    lock_acquire(& group->lock);
    if (group->parent != NULL)
      vpu_ready(group->parent);
    lock_release(& group->lock);
  }
  return;
}

bool tsc_group_check(tsc_group_t group) {
  assert(group != NULL);
  return TSC_ATOMIC_READ(group->count) == 0;
}

void tsc_group_sync(tsc_group_t group) {
  // if all subtasks are finish, no need to sleep..
  if (tsc_group_check(group)) 
    goto __quit_sync;

  lock_acquire(& group->lock);
  // FIXME: Do we need to check again to avoid errors?
  if (TSC_ATOMIC_READ(group->count) == 0) {
    lock_release(& group->lock);
    goto __quit_sync;
  }
  group->parent = tsc_coroutine_self();
  // goto sleep and wait the last finished subtask to 
  // wakeup me!!
  vpu_suspend(& group->lock,
              (unlock_handler_t)lock_release);

__quit_sync:
  TSC_DEALLOC(group);
  return;
}

