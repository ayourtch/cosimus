
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


#include <stdlib.h>
#include <assert.h>
#include "lib_debug.h"
#include "lib_lists.h"


#define listitem_checksig(lptr) assert((lptr)->listitem_sig == LISTITEM_SIG)
/**
 * @defgroup lists List management
 */

/*@{*/

/* push an item onto list from left side (head) */

listitem_t *
lpush(listitem_t ** li, void *data)
{
  listitem_t *li2;

  if(li == NULL) {
    return NULL;
  }
  li2 = malloc(sizeof(listitem_t));
  if(li2 == NULL) {
    return NULL;
  }
  li2->listitem_sig = LISTITEM_SIG;

  if(*li == NULL) {
    li2->next = NULL;
    li2->prev = li2;            /* point to self - since this is the first and only one */
    li2->data = data;
  } else {
    /*
       this is not the first element, so we add from the left (head) 
     */
    listitem_checksig(*li);
    li2->next = *li;
    li2->prev = (*li)->prev;
    li2->data = data;
    (*li)->prev = li2;
  }
  (*li) = li2;
  return li2;
}

/* push an item onto list from right side (tail) */

listitem_t *
rpush(listitem_t ** li, void *data)
{
  listitem_t *li2;

  if(li == NULL) {
    return NULL;
  }
  li2 = malloc(sizeof(listitem_t));
  if(li2 == NULL) {
    return NULL;
  }
  li2->listitem_sig = LISTITEM_SIG;


  if(*li == NULL) {
    li2->next = NULL;
    li2->prev = li2;            /* point to self - since this is the first and only one */
    li2->data = data;
    *li = li2;
  } else {
    /*
       this is not the first element, so we add from the right (tail) 
     */
    assert((*li)->prev->next == NULL);  /* this has to be the last element */
    listitem_checksig(*li);
    listitem_checksig((*li)->prev);
    (*li)->prev->next = li2;
    li2->prev = (*li)->prev;
    (*li)->prev = li2;
    li2->next = NULL;
    li2->data = data;
  }
  return li2;
}

/* pop an item from the list from the left side: nb: this frees up the element,
 * so it is the responsibility of those receiving the pointer to free it up!
 */
void *
lpop(listitem_t ** li)
{
  listitem_t *li2;
  void *ret;

  if(li == NULL || *li == NULL) {
    return NULL;
  }

  listitem_checksig(*li);
  li2 = (*li);
  ret = li2->data;
  (*li) = li2->next;
  if(*li) {
    (*li)->prev = li2->prev;    /* fix the "last element" pointer */
  }
  li2->listitem_sig = 0;
  free(li2);
  return ret;
}


void *
rpop(listitem_t ** li)
{
  listitem_t *li2;
  void *ret;

  if(li == NULL || *li == NULL) {
    return NULL;
  }
  listitem_checksig(*li);
  listitem_checksig((*li)->prev);

  li2 = (*li)->prev;
  ret = li2->data;

  (*li)->prev = li2->prev;
  listitem_checksig((*li)->prev);

  (*li)->prev->next = NULL;


  if((*li) == li2) {
    /*
       this was a last element - so nullify the list pointer 
     */
    (*li) = NULL;
  }
  li2->listitem_sig = 0;
  free(li2);

  return ret;
}

int
lbelongs(listitem_t ** li, listitem_t * li_el)
{
  listitem_t *li2 = *li;

  while(li2) {
    listitem_checksig(*li); 
    listitem_checksig(li2);
    if(li2 == li_el) {
      return 1;
    }
    li2 = li2->next;
  }
  debug(DBG_GLOBAL, 0, "List check: %x is not in the list %x", li_el, *li);
  if (li_el) {
    listitem_checksig(li_el);
  }
  return 0;
}

void
ldelete(listitem_t ** li, listitem_t * li_el)
{
  //listitem_t *li2;

  debug(DBG_GLOBAL, 10, "deleting item %x from list %x", li_el, *li);
  if(li == NULL) {
    return;
  }
  if(*li == NULL) {
    return;
  }
  listitem_checksig(*li);
  if(li_el == *li) {
    debug(DBG_GLOBAL, 10, "lpop");
    lpop(li);
  } else {
    // we have at least two elements - else we'd do lpop
    if(li_el == (*li)->prev) {
      debug(DBG_GLOBAL, 10, "rpop");
      rpop(li);
    } else {
      li_el->prev->next = li_el->next;
      li_el->next->prev = li_el->prev;
      li_el->listitem_sig = 0;
      free(li_el);
    }
  }
}

/* return left data (without popping it out) */
void *
lpeek(listitem_t ** li)
{
  if(li == NULL || *li == NULL) {
    return NULL;
  }
  listitem_checksig(*li);
  return (*li)->data;
}

/* return right data (without popping it out) */
void *
rpeek(listitem_t ** li)
{
  if(li == NULL || *li == NULL) {
    return NULL;
  }
  assert((*li)->prev != NULL);  /* if (*li) is not null the ->prev must be also valid. */
  listitem_checksig(*li);
  return (*li)->prev->data;
}

/*@}*/
