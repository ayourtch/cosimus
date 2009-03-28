

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

#ifndef __LIB_HASH__
#define __LIB_HASH__

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>


LIST_HEAD(hash_entry_head, hash_entry_t);
typedef struct hash_entry_head hash_entry_head_t;

typedef int hcallback_func_t(void *key, int key_len, void *data_old, void *data_new);

typedef struct hash_entry_t {
  LIST_ENTRY(hash_entry_t) entries; // linking with the other elements in the hash array index
  LIST_ENTRY(hash_entry_t) all_entries; // linking with the other elements in the iterator list
  void *key;
  int key_len;
  void *data;
  // flag whether this entry marked for deletion
  int is_deleted;
  // counts iterators that sit on this hen
  int lock_count;
  // callback function to indicate the actual and final deletion of this element
  // in case of iterators, etc.
  hcallback_func_t *do_delete;
  // Destructor function
  hcallback_func_t *destructor;
} hash_entry_t;


typedef struct htable_t {
  hash_entry_head_t *buckets;
  // list for the iterator traversals
  hash_entry_head_t all_entries;
  int index_bits;
  int default_key_len;
} htable_t;

/* a self-sufficient hashtable iterator */
typedef struct htable_iter_t {
  htable_t *ht;
  hash_entry_t *hen;
} htable_iter_t;


htable_t *halloc(int default_key_len, int index_bits);
void *hfind(htable_t *ht, void *key, int key_len);
void *hfinds(htable_t *ht, char *key);
char *hfindss(htable_t *ht, char *key);

void *hinsert(htable_t *ht, void *key, int key_len, void *data, hcallback_func_t *destructor, hcallback_func_t *can_replace, 
             hcallback_func_t *do_delete, int *did_delete);
void *hinserts(htable_t *ht, char *key, void *data, hcallback_func_t *destructor, hcallback_func_t *can_replace, 
             hcallback_func_t *do_delete, int *did_delete);

int hinsertss(htable_t *ht, char *key, char *data, 
             hcallback_func_t *can_replace, 
             hcallback_func_t *do_delete, int *did_delete);


void *hdelete(htable_t *ht, void *key, int key_len, hcallback_func_t *can_delete, 
             hcallback_func_t *do_delete, int *did_delete);
void *hdeletes(htable_t *ht, char *key, hcallback_func_t *can_delete, 
             hcallback_func_t *do_delete, int *did_delete);

int hdeletess(htable_t *ht, char *key, hcallback_func_t *can_delete, 
             hcallback_func_t *do_delete, int *did_delete);


htable_iter_t *hiter_first(htable_t *ht);
int hiter_has_items(htable_iter_t *iter);
int hiter_next(htable_iter_t *iter);
void *hiter_key(htable_iter_t *iter);
int hiter_key_len(htable_iter_t *iter);
void *hiter_data(htable_iter_t *iter);
void  hiter_free(htable_iter_t *iter);


#endif
