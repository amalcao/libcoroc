// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_SUPPORT_HASH_
#define _TSC_SUPPORT_HASH_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "support.h"
#include "coroc_lock.h"

/** Max hash code.*/
#define MAX_HASH 2147483647
/** The initial capacity of the hash table. */
#define CAPACITY 128
/** The max string length. */
#define LEN 1024
/** The max searching time for an element. */
#define LIMIT 6

#define A 17
#define B 72343
#define C 19
#define D 89145
#define M 9275507
#define N 8232341

/** The entry of the hash table. */
typedef struct {
  uint64_t key;
  void *value;
} hash_entry_t;

/** The structure of the hash table. */
typedef struct {
  coroc_lock lock;
  int capacity;
  int cursor;
  hash_entry_t **table[2];
} hash_t;

void hash_init(hash_t *hash);
int hash_insert(hash_t *hash, uint64_t key, void *value);
void *hash_get(hash_t *hash, uint64_t key, bool remove);
void hash_fini(hash_t *hash);

void atomic_hash_init(hash_t *hash);
int atomic_hash_insert(hash_t *hash, uint64_t key, void *value);
void *atomic_hash_get(hash_t *hash, uint64_t key, bool remove);
void atomic_hash_fini(hash_t *hash);

#endif
