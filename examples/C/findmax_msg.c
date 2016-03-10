// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "libtsc.h"

#define MAX 16

void gen_array_elem(int* array, int size) {
  int i;

  srand(array);

  for (i = 0; i < size; i++) {
    array[i] = rand();
  }
}

bool check_max_elem(int* array, int size, int max) {
  int i;
  for (i = 0; i < size; i++) {
    if (array[i] > max) return false;
  }

  return true;
}

/* -- the slave entry -- */
int find_max(void* arg) {
  tsc_coroutine_t parent = (tsc_coroutine_t)arg;
  tsc_coroutine_t self = tsc_coroutine_self();
  int size, *buf;

  // firstly, get the size ..
  tsc_recvp(&buf, &size, true);
  if (size < sizeof(int)) tsc_coroutine_exit(-1);

  size /= sizeof(int);

  if (size > 1) {
    if (size > 2) {

      tsc_coroutine_t slave0, slave1;
      int sz0, sz1;

      sz0 = size / 2;
      sz1 = size - sz0;

      slave0 = tsc_coroutine_allocate(find_max, tsc_refcnt_get(self), "s0",
                                      TSC_COROUTINE_NORMAL, 
                                      TSC_DEFAULT_PRIO, 0);
      tsc_send(slave0, buf, sz0 * sizeof(int));

      slave1 = tsc_coroutine_allocate(find_max, tsc_refcnt_get(self), "s1",
                                      TSC_COROUTINE_NORMAL,
                                      TSC_DEFAULT_PRIO, 0);
      tsc_send(slave1, buf + sz0, sz1 * sizeof(int));

      tsc_recv(&buf[0], sizeof(int), true);
      tsc_recv(&buf[1], sizeof(int), true);
    }
    if (buf[0] < buf[1]) buf[0] = buf[1];
  }

  tsc_send(parent, buf, sizeof(int));

  free(buf);

  tsc_refcnt_put((tsc_refcnt_t)(parent));
  tsc_coroutine_exit(0);
}

/* -- the user entry -- */
int main(void* arg) {
  int max = 0, size = MAX;
  int* array = malloc(size * sizeof(int));
  tsc_coroutine_t slave = NULL;
  tsc_coroutine_t self = tsc_coroutine_self();

  gen_array_elem(array, size);
  slave = tsc_coroutine_allocate(find_max, tsc_refcnt_get(self), "s",
                                 TSC_COROUTINE_NORMAL, 
                                 TSC_DEFAULT_PRIO, 0);

  tsc_send(slave, array, size * sizeof(int));  // tsc_send the content of array
  tsc_recv(&max, sizeof(int), true);

  printf("The MAX element is %d\n", max);
  if (!check_max_elem(array, size, max)) printf("The answer is wrong!\n");

  free(array);
  tsc_coroutine_exit(0);
}
