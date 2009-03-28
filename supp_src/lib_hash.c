
/**************************************************************************
*
*  Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.
*
*  Permission is hereby granted, free of charge, to any person obtaining 
* a copy of this software and associated documentation files (the "Software"), 
* to deal in the Software without restriction, including without limitation 
* the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom 
* the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included 
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
* OR OTHER DEALINGS IN THE SOFTWARE. 
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <assert.h>
#include <string.h>

#include "lib_hash_func.h"
#include "lib_hash.h"
#include "lib_debug.h"

#define HASH_INIT_VAL 0x12392739

#define null_if_null(something) if((something) == NULL) { return NULL; }

htable_t *halloc(int default_key_len, int index_bits)
{
  htable_t *ht = calloc(1, sizeof(htable_t));
  null_if_null(ht);

  assert(index_bits > 1 && index_bits <= 31);

  /*
   * NB: As the buckets are zero-initialized, it is the same
   * as calling LIST_INIT(&ht->buckets[i]) for every i
   */
  ht->buckets = calloc(hashsize(index_bits), sizeof(hash_entry_head_t));
  if (ht->buckets == NULL) {
    free(ht);
    return NULL;
  }

  ht->index_bits = index_bits;
  ht->default_key_len = default_key_len;
  return ht;
}

uint32_t h_index(htable_t *ht, void *key, int key_len)
{
  uint32_t idx;
  assert(ht != NULL);
  idx = hashmask(ht->index_bits) & hashlittle(key, key_len, HASH_INIT_VAL);
  //printf("h_index: (%llx): %x (key: '%s', key_len=%d)\n", (long long)ht, idx, (char *)key, key_len);
  //debug(0,0, "h_index: (%llx): %x (key: '%s', key_len=%d)\n", (long long)ht, idx, (char *)key, key_len);
  //debug_dump(0,0, key, key_len);
  return idx;
}

hash_entry_t *h_entry(htable_t *ht, uint32_t index, void *key, int key_len)
{
  assert(ht != NULL);
  hash_entry_t *hen;

  for(hen = ht->buckets[index].lh_first; hen != NULL; hen = hen->entries.le_next) {
    if(!hen->is_deleted && memcmp(key, hen->key, key_len) == 0) {
      return hen;
    }
  }
  return NULL;
}


/*
 * Hash find function
 */

void *hfind(htable_t *ht, void *key, int key_len)
{
  uint32_t index;
  hash_entry_t *hen;

  null_if_null(ht);

  index = h_index(ht, key, key_len);
  hen = h_entry(ht, index, key, key_len);
  return hen ? hen->data : NULL;
}

void *hfinds(htable_t *ht, char *key)
{
  return hfind(ht, key, strlen(key)+1);
}

char *hfindss(htable_t *ht, char *key)
{
  return (char *)hfind(ht, key, strlen(key)+1);
}


hash_entry_t *h_alloc_hen(int key_len) {
  hash_entry_t *hen;
  hen = calloc(1, sizeof(hash_entry_t));
  if (hen == NULL || ((hen->key = malloc(key_len)) == NULL)) {
    if (hen != NULL) { free(hen); }
    return NULL;
  } else {
    hen->key_len = key_len;
    return hen;
  }
}

void h_do_delete(htable_t *ht, hash_entry_t *hen, hcallback_func_t *do_delete)
{
  // unconditional removal. Should not be called directly because does not know about iterators...
  if(do_delete) {
    do_delete(hen->key, hen->key_len, hen->data, NULL);
  }
  LIST_REMOVE(hen, entries);
  LIST_REMOVE(hen, all_entries);
  if(hen->destructor) {
    hen->destructor(hen->key, hen->key_len, hen->data, NULL);
  }
  free(hen->key);
  free(hen);
}

int h_try_delete(htable_t *ht, hash_entry_t *hen, hcallback_func_t *do_delete, int *did_delete)
{
  int result;
  if(hen->lock_count == 0) {
    h_do_delete(ht, hen, do_delete);
    result = 1;
  } else {
    /* 
     * there are iterators sitting on this element, so we 
     * can only mark it for later deletion 
     */
    //printf("Deferring deletion for key '%s'...\n", (char *)hen->key);
    hen->is_deleted = 1;
    hen->do_delete = do_delete;
    result = 0;
  }
  if (did_delete) { *did_delete = result; }
  return result;
}


/*
 * A fancy insertion function.
 *
 * It takes the key+key length, the data pointer,
 * and a callback pointer to a function which 
 * dictates whether the replacement in the hash
 * for a given key is OK or not, and also 
 * can output some diagnostic information if needed.
 *
 * The return of this function is the data value 
 * that got "purged" - obviously, in case of no collision 
 * it is NULL, and in case of inability to insert, it might be
 * the original data pointer!
 *
 * So, the success of the function is when the return is != data
 */

void *
hinsert(htable_t *ht, void *key, int key_len, void *data, hcallback_func_t *destructor, 
                      hcallback_func_t *can_replace, hcallback_func_t *do_delete, int *did_delete)
{
  uint32_t index;
  hash_entry_t *hen;
  hash_entry_t *hen_old;
  void *data_old = NULL;

  null_if_null(ht);
  
  index = h_index(ht, key, key_len);
  hen = h_entry(ht, index, key, key_len);

  hen_old = hen;
  data_old = hen ? hen->data : NULL;

  if (did_delete) { *did_delete = 0; }

  if(can_replace != NULL && !can_replace(key, key_len, data_old, data)) {
    return data;
  }
  if (NULL == (hen = h_alloc_hen(key_len))) {
    return data;
  }
  if (hen_old) {
    /*
     * entry already exists - we are about to replace it.
     *
     * If not for the iterators, we'd be all fine and dandy.
     *
     * But there can be iterators referencing this item
     * (hence, can't wipe out key/data).
     */
    h_try_delete(ht, hen_old, do_delete, did_delete);
  } else {
    if (did_delete) { *did_delete = 0; }
  }

  /* 
   * fill in the new hen
   */

  memcpy(hen->key, key, key_len);
  hen->data = data;
  hen->destructor = destructor;
  LIST_INSERT_HEAD(&ht->buckets[index], hen, entries);
  // inserting in the head of all entries ensures we don't have
  // to mess with the iterators
  LIST_INSERT_HEAD(&ht->all_entries, hen, all_entries);
  // successfully inserted 
  return data_old;
}

void *
hinserts(htable_t *ht, char *key, void *data, hcallback_func_t *destructor, hcallback_func_t *can_replace, hcallback_func_t *do_delete, int *did_delete)
{
  return hinsert(ht, key, strlen(key)+1, data, destructor, can_replace, do_delete, did_delete);
}

static int h_string_destructor(void *key, int key_len, void *data_old, void *data_new)
{
  free(data_old);
  return 1;
}

int
hinsertss(htable_t *ht, char *key, char *data, hcallback_func_t *can_replace, hcallback_func_t *do_delete, int *did_delete)
{
  char *new = strdup(data);
  if(new == hinsert(ht, key, strlen(key)+1, new, h_string_destructor, can_replace, do_delete, did_delete)) {
    //printf("Could not insert!\n");
    free(new);
    return 0;
  } else {
    //printf("inserted [%s] = '%s'!\n", key, data);
    return 1;
  }
}



/*
 * hash deletion function
 *
 * similarly to hash insertion function, accepts the callback parameter, 
 * which specifies whether it can delete or not
 *
 * returns the data pointer that has been deleted or marked for deletion.
 * The "do_delete" callback is called when the item is indeed deleted.
 * The "did_delete" variable gets updated with the info if the item was really
 * deleted or only marked for deletion.
 *
 * NULL if not found or not deleted
 */

void *hdelete(htable_t *ht, void *key, int key_len, hcallback_func_t *can_delete, hcallback_func_t *do_delete, int *did_delete)
{
  uint32_t index;
  hash_entry_t *hen;
  void *data_old;

  null_if_null(ht);
  
  index = h_index(ht, key, key_len);
  hen = h_entry(ht, index, key, key_len);
  if (hen == NULL) {
    if (did_delete) { *did_delete = 0; }
    return NULL;
  } else {
    if(can_delete == NULL || can_delete(key, key_len, hen->data, NULL)) {
      data_old = hen->data;
      h_try_delete(ht, hen, do_delete, did_delete);
      return data_old;
    } else {
      // aren't going to delete anything even though found - not allowed.
      if (did_delete) { *did_delete = 0; }
      return NULL;
    }
  }
}

void *hdeletes(htable_t *ht, char *key, hcallback_func_t *can_delete, hcallback_func_t *do_delete, int *did_delete)
{
  return hdelete(ht, key, strlen(key)+1, can_delete, do_delete, did_delete);
}

int hdeletess(htable_t *ht, char *key, hcallback_func_t *can_delete, hcallback_func_t *do_delete, int *did_delete)
{
  return (NULL != hdelete(ht, key, strlen(key)+1, can_delete, do_delete, did_delete));
}

void h_lock_hen_iter(htable_t *ht, hash_entry_t *hen)
{
  if(hen) {
    hen->lock_count++;
  }
}

void h_unlock_hen_iter(htable_t *ht, hash_entry_t *hen)
{
  if(hen) {
    hen->lock_count--;
    if (hen->lock_count == 0 && hen->is_deleted) {
      h_do_delete(ht, hen, hen->do_delete);
    }
  }
}

htable_iter_t *hiter_first(htable_t *ht)
{
  htable_iter_t *iter;
  null_if_null(ht);
  iter = calloc(1, sizeof(htable_iter_t));
  iter->ht = ht;
  iter->hen = ht->all_entries.lh_first;
  h_lock_hen_iter(ht, iter->hen);
  return iter;
}

int hiter_has_items(htable_iter_t *iter)
{
  return iter && iter->hen;
}

int hiter_next(htable_iter_t *iter)
{
  hash_entry_t *hen_next = iter->hen ? iter->hen->all_entries.le_next : NULL;

  h_lock_hen_iter(iter->ht, hen_next);
  h_unlock_hen_iter(iter->ht, iter->hen);
  iter->hen = hen_next;
  return hen_next != NULL;
}

void *hiter_key(htable_iter_t *iter) 
{
  null_if_null(iter);
  null_if_null(iter->hen);

  return iter->hen->key;
}

int hiter_key_len(htable_iter_t *iter) 
{
  return iter && iter->hen ? iter->hen->key_len : 0;
}


void *hiter_data(htable_iter_t *iter) 
{
  null_if_null(iter);
  null_if_null(iter->hen);
  
  return iter->hen->data;
}

void hiter_free(htable_iter_t *iter) 
{
  if (iter) {
    h_unlock_hen_iter(iter->ht, iter->hen);
    free(iter);
  }
}
