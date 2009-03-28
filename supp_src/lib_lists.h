
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
#ifndef __LIB_LISTS_H__
#define __LIB_LISTS_H__

enum {
  LISTITEM_SIG = 0x115ABBEC
};

typedef struct listitem {
  struct listitem *next;
  struct listitem *prev;
  /*
     in the head element "prev" to the last(tail) element.
     * In all the others this points to the previous one - so this makes 
     * a circular unidirectional list from tail towards head.
   */
  int listitem_sig;
  void *data;
} listitem_t;


/* push an item onto list from left side (head) */

listitem_t *lpush(listitem_t ** li, void *data);

/* push an item onto list from right side (tail) */

listitem_t *rpush(listitem_t ** li, void *data);

/* pop an item from the list from the left side: nb: this frees up the element,
 * so it is the responsibility of those receiving the pointer to free it up!
 */
void *lpop(listitem_t ** li);


void *rpop(listitem_t ** li);

int
  lbelongs(listitem_t ** li, listitem_t * li_el);

void
  ldelete(listitem_t ** li, listitem_t * li_el);

/* return left data (without popping it out) */
void *lpeek(listitem_t ** li);

/* return right data (without popping it out) */
void *rpeek(listitem_t ** li);

#endif
