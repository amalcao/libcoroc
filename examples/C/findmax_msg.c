// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "libcoroc.h"

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
  coroc_coroutine_t parent = (coroc_coroutine_t)arg;
  coroc_coroutine_t self = coroc_coroutine_self();
  int size, *buf;

  // firstly, get the size ..
  coroc_recvp(&buf, &size, true);
  if (size < sizeof(int)) coroc_coroutine_exit(-1);

  size /= sizeof(int);

  if (size > 1) {
    if (size > 2) {

      coroc_coroutine_t slave0, slave1;
      int sz0, sz1;

      sz0 = size / 2;
      sz1 = size - sz0;

      slave0 = coroc_coroutine_allocate(find_max, coroc_refcnt_get(self), "s0",
                                      TSC_COROUTINE_NORMAL, 
                                      TSC_DEFAULT_PRIO, 0);
      coroc_send(slave0, buf, sz0 * sizeof(int));

      slave1 = coroc_coroutine_allocate(find_max, coroc_refcnt_get(self), "s1",
                                      TSC_COROUTINE_NORMAL,
                                      TSC_DEFAULT_PRIO, 0);
      coroc_send(slave1, buf + sz0, sz1 * sizeof(int));

      coroc_recv(&buf[0], sizeof(int), true);
      coroc_recv(&buf[1], sizeof(int), true);
    }
    if (buf[0] < buf[1]) buf[0] = buf[1];
  }

  coroc_send(parent, buf, sizeof(int));

  free(buf);

  coroc_refcnt_put((coroc_refcnt_t)(parent));
  coroc_coroutine_exit(0);
}

/* -- the user entry -- */
int main(void* arg) {
  int max = 0, size = MAX;
  int* array = malloc(size * sizeof(int));
  coroc_coroutine_t slave = NULL;
  coroc_coroutine_t self = coroc_coroutine_self();

  gen_array_elem(array, size);
  slave = coroc_coroutine_allocate(find_max, coroc_refcnt_get(self), "s",
                                 TSC_COROUTINE_NORMAL, 
                                 TSC_DEFAULT_PRIO, 0);

  coroc_send(slave, array, size * sizeof(int));  // coroc_send the content of array
  coroc_recv(&max, sizeof(int), true);

  printf("The MAX element is %d\n", max);
  if (!check_max_elem(array, size, max)) printf("The answer is wrong!\n");

  free(array);
  coroc_coroutine_exit(0);
}
