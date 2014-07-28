#include "tsc_hash.h"

static inline uint64_t __hash_fun0(uint64_t code) { return (A * code + B) % M; }

static inline uint64_t __hash_fun1(uint64_t code) { return (C * code + D) % N; }

static void __resize(hash_t *ht) {
  hash_entry_t **tmp, *e;
  int i, j, newpos;
  int capacity = 2 * (ht->capacity);

  for (i = 0; i < 2; i++) {
    tmp = malloc(capacity * sizeof(hash_entry_t *));
    memset(tmp, 0, capacity * sizeof(hash_entry_t *));

    for (j = 0; j < ht->capacity; j++) {
      e = ht->table[i][j];
      newpos =
          i ? __hash_fun1(e->key) % capacity : __hash_fun0(e->key) % capacity;

      tmp[newpos] = ht->table[i][j];
    }

    free(ht->table[i]);
    ht->table[i] = tmp;
  }
  ht->capacity = capacity;

  return;
}

static int __add(hash_t *ht, hash_entry_t *e) {
  hash_entry_t *tmp = NULL;
  int i = 0, k, pos;

  for (k = 0; k < LIMIT; k++) {
    pos = i ? __hash_fun1(e->key) % (ht->capacity)
            : __hash_fun0(e->key) % (ht->capacity);

    tmp = ht->table[i][pos];

    if (tmp == NULL) {
      ht->table[i][pos] = e;
      return 0;
    }

    if (e->key == tmp->key) {
      return -1;
    }

    ht->table[i][pos] = e;
    e = tmp;
    i = (i + 1) % 2;
  }

  __resize(ht);
  return __add(ht, e);
}

void hash_init(hash_t *hash) {
  int i;
  hash->capacity = CAPACITY;
  for (i = 0; i < 2; i++) {
    hash->table[i] = malloc(CAPACITY * sizeof(hash_entry_t *));
    memset(hash->table[i], 0, sizeof(hash_entry_t *) * CAPACITY);
  }
}

void hash_fini(hash_t *hash) {
  int i, j;
  for (i = 0; i < 2; i++)
    if (hash->table[i]) {
      for (j = 0; j < hash->capacity; j++)
        if (hash->table[i][j]) free(hash->table[i][j]);
      free(hash->table[i]);
    }
}

int hash_insert(hash_t *hash, uint64_t key, void *value) {
  hash_entry_t *e = NULL;

  // Init the new element.
  e = malloc(sizeof(hash_entry_t));
  e->key = key;
  e->value = value;

  return __add(hash, e);
}

void *hash_get(hash_t *hash, uint64_t key, bool remove) {
  int i, pos;
  void *value = NULL;
  hash_entry_t *e;

  for (i = 0; i < 2; i++) {
    pos = i ? __hash_fun1(key) % hash->capacity
            : __hash_fun0(key) % hash->capacity;
    e = hash->table[i][pos];

    if (e && (e->key == key)) {
      value = e->value;
      if (remove) {
        hash->table[i][pos] = NULL;
        free(e);
      }
      break;
    }
  }

  return value;
}

#if 0
int set(hash_t *ht, const char *key, char *value)
{
  int hash = __hash_code(key);
  int i, pos;
  hash_entry_t *ret;

  for (i=0; i<2; i++) {
      pos =  __hash_fun[i](hash) % ht->capacity ;
      ret = ht->table[i][pos];

      if ( ret && !strcmp(ret->key, key) ) {
          strcpy(ret->value, value);
          return 0;
      }
  }

  return -1;
}


int enumerate(hash_t *ht)
{
  if ( ht == NULL )
    return -1;

  ht->cursor = 0;

  return 0;
}

int next(hash_t *ht, hash_entry_t **entry)
{
  if ( ht == NULL )
    return -1;

  int tno, no, found = FALSE;

  tno = ht->cursor / ht->capacity;
  no = ht->cursor % ht->capacity;

  while ( tno <= 1 ) {
      for ( ; no < ht->capacity && found == FALSE; no++ ) {
          if ( ht->table[tno][no] ) {
              *entry = ht->table[tno][no];
              found = TRUE;
          }
          ht->cursor ++;
      }
      tno ++;
      no = 0;
  }

  if ( found == TRUE )
    return 0;

  ht->cursor = 0;
  return -1;
}
#endif

void atomic_hash_init(hash_t *hash) {
  lock_init(&hash->lock);
  hash_init(hash);
}

int atomic_hash_insert(hash_t *hash, uint64_t key, void *value) {
  lock_acquire(&hash->lock);
  int ret = hash_insert(hash, key, value);
  lock_release(&hash->lock);
  return ret;
}

void *atomic_hash_get(hash_t *hash, uint64_t key, bool remove) {
  lock_acquire(&hash->lock);
  void *value = hash_get(hash, key, remove);
  lock_release(&hash->lock);
  return value;
}
