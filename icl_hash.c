/**
 * @file icl_hash.c
 *
 * Dependency free hash table implementation.
 *
 * This simple hash table implementation should be easy to drop into
 * any other peice of code, it does not depend on anything else :-)
 *
 * @author Jakub Kurzak
 */
/* $Id: icl_hash.c 2838 2011-11-22 04:25:02Z mfaverge $ */
/* $UTK_Copyright: $ */
// I added mutexes for concurrency
#include "icl_hash.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

// divider for reducing the total mutexes
// Ex. nmutexes = nbuckets/mutexFactor
#define mutexFactor 64
#define BITS_IN_int (sizeof(int) * CHAR_BIT)
#define THREE_QUARTERS ((int)((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH ((int)(BITS_IN_int / 8))
#define HIGH_BITS (~((unsigned int)(~0) >> ONE_EIGHTH))
/**
 * A simple string hash.
 *
 * An adaptation of Peter Weinberger's (PJW) generic hashing
 * algorithm based on Allen Holub's version. Accepts a pointer
 * to a datum to be hashed and returns an unsigned integer.
 * From: Keith Seymour's proxy library code
 *
 * @param[in] key -- the string to be hashed
 *
 * @returns the hash index
 */
static unsigned int hash_pjw(void* key) {
  char* datum = (char*)key;
  unsigned int hash_value, i;

  if (!datum)
    return 0;

  for (hash_value = 0; *datum; ++datum) {
    hash_value = (hash_value << ONE_EIGHTH) + *datum;
    if ((i = hash_value & HIGH_BITS) != 0)
      hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
  }
  return (hash_value);
}

static int string_compare(void* a, void* b) {
  return (strcmp((char*)a, (char*)b) == 0);
}

/**
 * Create a new hash table.
 *
 * @param[in] nbuckets -- hint number of buckets to create
 * @param[in] hash_function -- pointer to the hashing function to be used
 * @param[in] hash_key_compare -- pointer to the hash key comparison function to
 * be used
 *
 * @returns pointer to new hash table.
 */

icl_hash_t* icl_hash_create(size_t nbucketsHint,
                            unsigned int (*hash_function)(void*),
                            int (*hash_key_compare)(void*, void*)) {
  icl_hash_t* ht;
  size_t i;

  ht = (icl_hash_t*)malloc(sizeof(icl_hash_t));
  if (!ht)
    return NULL;

  for (i = 64; i < nbucketsHint; i *= 2)
    ;
  size_t nbuckets = i;

  ht->listrwlockes = malloc(nbuckets / mutexFactor * sizeof(pthread_rwlock_t));
  for (i = 0; i < (nbuckets / mutexFactor); i++)
    pthread_rwlock_init(&(ht->listrwlockes[i]), NULL);

  pthread_mutex_init(&(ht->fieldMutex), NULL);
  ht->nentries = 0;
  ht->buckets = (icl_entry_t**)malloc(nbuckets * sizeof(icl_entry_t*));
  if (!ht->buckets)
    return NULL;

  ht->nbuckets = nbuckets;
  for (i = 0; i < ht->nbuckets; i++)
    ht->buckets[i] = NULL;

  ht->hash_function = hash_function ? hash_function : hash_pjw;
  ht->hash_key_compare = hash_key_compare ? hash_key_compare : string_compare;

  return ht;
}

/**
 * Search for an entry in a hash table.
 *
 * @param ht -- the hash table to be searched
 * @param key -- the key of the item to search for
 *
 * @returns pointer to the data corresponding to the key.
 *   If the key was not found, returns NULL.
 */

void* icl_hash_find(icl_hash_t* ht, void* key) {
  icl_entry_t* curr;
  unsigned int hash_val;

  if (!ht || !key)
    return NULL;

  hash_val = (*ht->hash_function)(key) % ht->nbuckets;
  pthread_rwlock_rdlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  for (curr = ht->buckets[hash_val]; curr != NULL; curr = curr->next)
    if (ht->hash_key_compare(curr->key, key)) {
      pthread_rwlock_unlock(
          &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
      return (curr->data);
    }
  pthread_rwlock_unlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  return NULL;
}

/**
 * Update an item into the hash table.
 *
 * @param ht -- the hash table
 * @param key -- the key of the new item
 * @param data -- pointer to the new item's data
 *
 * @returns 0 on success  Returns -1 on error.
 */

int icl_hash_update(icl_hash_t* ht, void* key, void* newData) {
  icl_entry_t* curr;
  unsigned int hash_val;

  if (!ht || !key)
    return -1;

  hash_val = (*ht->hash_function)(key) % ht->nbuckets;
  pthread_rwlock_wrlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  for (curr = ht->buckets[hash_val]; curr != NULL; curr = curr->next)
    if (ht->hash_key_compare(curr->key, key)) {
      curr->data = newData;
      pthread_rwlock_unlock(
          &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
      return 0;
    }
  pthread_rwlock_unlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  return -1;
}

/**
 * Insert an item into the hash table.
 *
 * @param ht -- the hash table
 * @param key -- the key of the new item
 * @param data -- pointer to the new item's data
 *
 * @returns pointer to the new item.  Returns NULL on error.
 */

icl_entry_t* icl_hash_insert(icl_hash_t* ht, void* key, void* data) {
  icl_entry_t* curr;
  unsigned int hash_val;

  if (!ht || !key)
    return NULL;

  hash_val = (*ht->hash_function)(key) % ht->nbuckets;

  pthread_rwlock_rdlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  for (curr = ht->buckets[hash_val]; curr != NULL; curr = curr->next)
    if (ht->hash_key_compare(curr->key, key)) {
      pthread_rwlock_unlock(
          &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
      return (NULL); /* key already exists */
    }

  /* if key was not found */
  curr = (icl_entry_t*)malloc(sizeof(icl_entry_t));
  if (!curr) {
    pthread_rwlock_unlock(
        &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
    return NULL;
  }

  curr->key = key;
  curr->data = data;
  curr->next = ht->buckets[hash_val]; /* add at start */

  pthread_rwlock_unlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));

  pthread_rwlock_wrlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  ht->buckets[hash_val] = curr;
  pthread_rwlock_unlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));

  pthread_mutex_lock(&(ht->fieldMutex));
  ht->nentries++;
  pthread_mutex_unlock(&(ht->fieldMutex));

  return curr;
}

/**
 * Free one hash table entry located by key (key and data are freed using
 * functions).
 *
 * @param ht -- the hash table to be freed
 * @param key -- the key of the new item
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int icl_hash_delete(icl_hash_t* ht,
                    void* key,
                    void (*free_key)(void*),
                    void (*free_data)(void*)) {
  icl_entry_t *curr, *prev;
  unsigned int hash_val;

  if (!ht || !key)
    return -1;
  hash_val = (*ht->hash_function)(key) % ht->nbuckets;

  prev = NULL;

  pthread_rwlock_wrlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  for (curr = ht->buckets[hash_val]; curr != NULL;) {
    if (ht->hash_key_compare(curr->key, key)) {
      if (prev == NULL) {
        ht->buckets[hash_val] = curr->next;
      } else {
        prev->next = curr->next;
      }
      if (*free_key && curr->key)
        (*free_key)(curr->key);
      if (*free_data && curr->data)
        (*free_data)(curr->data);
      pthread_mutex_lock(&(ht->fieldMutex));
      ht->nentries--;
      pthread_mutex_unlock(&(ht->fieldMutex));
      free(curr);

      pthread_rwlock_unlock(
          &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
      return 0;
    }
    prev = curr;
    curr = curr->next;
  }

  pthread_rwlock_unlock(
      &(ht->listrwlockes[hash_val % (ht->nbuckets / mutexFactor)]));
  return -1;
}

/**
 * Free hash table structures (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int icl_hash_destroy(icl_hash_t* ht,
                     void (*free_key)(void*),
                     void (*free_data)(void*)) {
  icl_entry_t *bucket, *curr, *next;
  size_t i;

  if (!ht)
    return -1;

  for (i = 0; i < ht->nbuckets; i++) {
    bucket = ht->buckets[i];
    for (curr = bucket; curr != NULL;) {
      next = curr->next;
      if (*free_key && curr->key)
        (*free_key)(curr->key);
      if (*free_data && curr->data)
        (*free_data)(curr->data);
      free(curr);
      curr = next;
    }
  }

  if (ht->buckets)
    free(ht->buckets);
  if (ht)
    free(ht);

  return 0;
}

/**
 * Dump the hash table's contents to the given file pointer.
 *
 * @param stream -- the file to which the hash table should be dumped
 * @param ht -- the hash table to be dumped
 *
 * @returns 0 on success, -1 on failure.
 */

int icl_hash_dump(FILE* stream, icl_hash_t* ht) {
  icl_entry_t *bucket, *curr;
  size_t i;

  if (!ht)
    return -1;

  for (i = 0; i < ht->nbuckets; i++) {
    pthread_rwlock_rdlock(
        &(ht->listrwlockes[i % (ht->nbuckets / mutexFactor)]));
    bucket = ht->buckets[i];
    for (curr = bucket; curr != NULL;) {
      if (curr->key)
        fprintf(stream, "icl_hash_dump: %s: %p\n", (char*)curr->key,
                curr->data);
      curr = curr->next;
    }

    pthread_rwlock_unlock(
        &(ht->listrwlockes[i % (ht->nbuckets / mutexFactor)]));
  }

  return 0;
}
