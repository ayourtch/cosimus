
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

#ifndef _LIB_DBUF_H_
#define _LIB_DBUF_H_

#ifdef WITH_EFENCE
#include <efence.h>
#endif


#include <stdint.h>
#include <alloca.h>
#include <string.h>
#include "lib_debug.h"

typedef void (*dbuf_destructor_t) (void *dbuf);

/**
 * This structure allows for flexible manipulation with the data.
 * The very first member is the pointer to the buffer with actual data,
 * so the dbuf_t can be used to manage the memory for sometype_t, and then
 * by declaring the variable to be sometype_t ** 
 * (which is a dbuf_t * under the hood), one can have a direct access to 
 * the fields of the corresponding data structure, and typecasting it to
 * (dbuf_t *) allows to do raw memory manipulations.
 */
typedef struct dbuf {

  uint8_t *buf;   /**< pointer to the actual data */

  char *uptype_sig;  /**< possible sig pointer to check the higher types */

  int lock;   /**< free the data up once we decrement it down to zero */

  int size;   /**< size of the buf */

  int dsize;   /**< data size in the buf */

  int bitpos;  /**< bit position for bitpacking */

  int elsize;   /**< element size of the buf - for array-like containers */

  int growsize;   /**< increments in which to grow, if needed - in bytes. */

  dbuf_destructor_t destructor; /**< destructor function for this dbuf */

  void *savebuf;  /**< backup pointer to the buffer, used for error checking */

  long signature;   /**< signature of dbuf, used for error checking */

  void *list_entry; /**< entry in the global list of dbufs */

  backtrace_t allocator; /**< backtrace of the allocator */

  backtrace_t releaser; /**< backtrace of the releaser */
} dbuf_t;

/**
 * "reader" type for dbuf - as you can have many of them, 
 * we need to split it separately because the different readers can be at
 * different points in dbuf at different times.
 */
typedef struct dbufr {

    /** pointer to the dbuf where we are reading from */
  dbuf_t *d;

    /** offset of the next byte to be "read" */
  int offset;
} dbufr_t;

/* like strcat, also grow the buffer if necessary. 1=success, 0=failure 
 * len is the length of the meaningful data, or "-1" if this is a regular string
 */
enum {
  DMEMCAT_CHUNK_INCREMENT = 1024,
};


dbuf_t *dalloc(int size);
dbuf_t * dsetusig(dbuf_t *d, const char *uptype_sig);
int dcheckusig(dbuf_t *d, const char *uptype_sig);

dbuf_t *dalloczf(int size);
dbuf_t *dlock(void *ptr);
int dgrow(dbuf_t * d, size_t delta);
void ddestructor(dbuf_t * d, dbuf_destructor_t destr);
void dunlock(void *ptr);

void dsend(int idx, dbuf_t * d);


void dxprintf(dbuf_t * d, char *fmt, ...);

dbuf_t *dstrcpy(const char *str);
dbuf_t *dsubstrcpy(dbuf_t *d, int start, int howmany);


int dconcat(dbuf_t * target, dbuf_t * source);
int dmemscan(dbuf_t * d, char c, int start);
void print_dbufs(void);
void print_dbuf(int logtype, int loglevel, dbuf_t * d);


int dmemcat(dbuf_t * d, void *str, int total);
int dstrcat(dbuf_t * d, char *str, int total);
int dxcat(dbuf_t * target, dbuf_t * source, int start, int len);

int dappendfill(dbuf_t * target, char c, int count);

int dappendfile(dbuf_t * d, char *fname);
int dwritefile(dbuf_t * d, char *fname);


dbufr_t *dropen(dbuf_t * dbuf);
void drclose(dbufr_t * dr);

#endif

