
/*
 * Copyright (c) Andrew Yourtchenko <ayourtch@gmail.com>, 2008-2009
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Cosimus Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
