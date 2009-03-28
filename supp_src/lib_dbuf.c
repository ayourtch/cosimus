

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

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "lib_debug.h"
#include "lib_dbuf.h"

/**
 * @defgroup dbuf dbuf - simple data buffer management
 */

/*@{*/

void
print_dbuf(int logtype, int loglevel, dbuf_t * d)
{
  if(is_debug_on(logtype, loglevel)) {
    debug(logtype, loglevel, "dbuf: %x", d);
    debug(logtype, loglevel, "size: %6d, dsize: %6d", d->size, d->dsize);
    debug(logtype, loglevel, "allocator backtrace size: %d",
          d->allocator.size);
    print_backtrace_t(logtype, loglevel, &d->allocator);
    debug(logtype, loglevel, "lock count: %d", d->lock);
    debug(logtype, loglevel, "releaser backtrace size: %d", d->releaser.size);
    print_backtrace_t(logtype, loglevel, &d->releaser);
  }
}


//listitem_t *dbuf_list = NULL;

enum { DBUF_SIGNATURE = 0xDB0F510A };

void
dsetsig(dbuf_t * d)
{
  d->signature = DBUF_SIGNATURE;
  d->savebuf = d->buf;
}

void
dchecksig(dbuf_t * d)
{
  if(d->signature != DBUF_SIGNATURE) {
    debug(DBG_GLOBAL, 0, "dchecksig: signature for %x is %x", d,
          d->signature);
    print_dbuf(0, 0, d);
    print_backtrace();
    assert(d->signature == DBUF_SIGNATURE);
  }
  if(d->savebuf != d->buf) {
    debug(DBG_GLOBAL, 0, "dchecksig: error with %x: savebuf: %x, buf: %x", d,
          d->savebuf, d->buf);
    print_dbuf(0, 0, d);
    print_backtrace();
    assert(d->savebuf == d->buf);
  }
}

void
print_dbufs(void)
{
  /*
  dbuf_t *d;
  int count = 0;
  listitem_t *li = dbuf_list;

  debug(DBG_GLOBAL, 0, "DBUF list: %x", dbuf_list);
  while(li) {
    d = li->data;
    if(d != NULL) {
      print_dbuf(0, 0, d);
    } else {
      debug(DBG_GLOBAL, 0, "dbuf ptr is NULL - not good!");
    }
    li = li->next;
    count++;
  }
  debug(DBG_GLOBAL, 0, "Total dbufs: %d", count);
  */
}

void
check_dbuf_list(void)
{
/*
  listitem_t *li = dbuf_list;

  while(li) {
    if(li->data == NULL) {
      debug(DBG_GLOBAL, 0, "dbuf ptr is NULL - not good!");
      assert(0);
    }
    li = li->next;
  }
*/
}


/**
 * Allocate a new dbuf. Return NULL if something did not work
 *
 * @param size the size of the data buffer to allocate.
 *
 * @see dunlock dlock dalloczf
 * @return the pointer to the newly allocated dbuf
 *
 */

dbuf_t *
dalloc(int size)
{
  dbuf_t *d;

  void *buf;

  debug(DBG_GLOBAL, 50, "dalloc attempt of %d bytes", size);
  check_dbuf_list();

  if(size == 0) {
    buf = NULL;
  } else {
    buf = malloc(size);
    if(buf == NULL) {
      return NULL;
    }
  }
  d = malloc(sizeof(dbuf_t));
  bzero(d, sizeof(*d));
  if(d == NULL) {
    free(buf);
    return NULL;
  }
  d->lock = 1;                  /* first allocation is also a lock */
  d->size = size;
  d->dsize = 0;                 /* empty */
  d->bitpos = 0; 		/* empty */
  d->elsize = 1;                /* by default = 1 byte */
  d->growsize = DMEMCAT_CHUNK_INCREMENT;
  d->buf = buf;
  d->destructor = NULL;
  dsetsig(d);
  get_backtrace(&d->allocator);
  bzero(&d->releaser, sizeof(d->releaser));
  //LISTXXX d->list_entry = lpush(&dbuf_list, d);
  debug(DBG_MEMORY, 10, "dalloc of %d bytes, result %x", size, d);
  check_dbuf_list();
  return d;
}


dbuf_t *
dsetusig(dbuf_t *d, const char *uptype_sig)
{
  if(d && uptype_sig) {
    d->uptype_sig = (void *)uptype_sig;
  } 
  return d;

}

int
dcheckusig(dbuf_t *d, const char *uptype_sig)
{
  int result = 0;
  if(d) {
    result = (d->uptype_sig == uptype_sig);
    if(!result) {
      debug(DBG_MEMORY, 0, "uptype sig check failed for %x, expected: %s, got: %s", d, uptype_sig, d->uptype_sig);
    }
  } else {
    result = 1;
  }
  return result;
}

void
ddestructor(dbuf_t * d, dbuf_destructor_t destr)
{
  d->destructor = destr;
  debug(DBG_MEMORY, 10, "ddestructor set destructor %x for dbuf %x",
        d->destructor, d);
}

/**
 * dalloc with zero-fill  
 *
 * @param size the size of the data buffer
 *
 * @see dalloc
 *
 * @return the pointer to the newly allocated dbuf
 *
 */

dbuf_t *
dalloczf(int size)
{
  dbuf_t *d = dalloc(size);

  if(d) {
    d->dsize = d->size;
    bzero(d->buf, d->size);
  }
  return d;
}

/**
 * lock dbuf  - to prevent premature freeing 
 *
 * @param ptr pointer to the dbuf to lock.
 *
 * @return dbuf that was passed as argument
 *
 * @see dunlock dalloc
 */

dbuf_t *
dlock(void *ptr)
{
  dbuf_t *d = ptr;

  if(d == NULL) {
    return NULL;
  }
  check_dbuf_list();
  dchecksig(d);
  d->lock++;
  debug(DBG_MEMORY, 10, "dlock of %x, new lock count is %d", d, d->lock);
  return d;
}

/**
 * unlock the dbuf. if the reference count becomes zero, 
 * it is freed. the allocation of block sets the lock count to 1
 *
 * @param ptr pointer to the dbuf to unlock
 *
 * @see dlock dalloc
 */

void
dunlock(void *ptr)
{
  dbuf_t *d = ptr;

  check_dbuf_list();

  if(d != NULL) {
    dchecksig(d);
    d->lock--;
    debug(DBG_MEMORY, 10, "dunlock of %x, new lock count is %d", d, d->lock);
    assert(d->lock >= 0);
    if (d->dsize > d->size) {
      debug(0,0, "dsize > size, impossible. Memory corruption has/will occur. dumping the block and quitting");
      debug_dump(0,0, d->buf, d->size);
      assert(0 == "Buffer corruption");
    }
    
    if(d->lock == 0) {
      if(d->destructor != NULL) {
        debug(DBG_MEMORY, 10, "dunlock calling destructor %x", d->destructor);
        d->destructor(d);
      }
      get_backtrace(&d->releaser);
      /* LISTXXX
      if(lbelongs(&dbuf_list, d->list_entry)) {
        ldelete(&dbuf_list, d->list_entry);
      } else {
        assert("Corrupt dbuf list!" == NULL);
      }
      print_dbuf(1013, 11, d);
      */
      memset(d->buf, 0xCA, d->size);
      free(d->buf);
      //memset(d, 0xCA, sizeof(*d));
      d->signature = 0xCACACACA;
      d->uptype_sig = NULL;
      free(d);
    }
  }
  check_dbuf_list();
}

/** 
 * Resize the data portion of the dbuf (available data buffer). 
 *
 * @param d pointer to the dbuf
 * @param size the new target size of dbuf
 *
 * @return 0 if error
 * @return 1 if successful
 */

int
dresize(dbuf_t * d, size_t size)
{
  void *buf = realloc(d->buf, size);

  dchecksig(d);

  debug(DBG_MEMORY, 10,
        "dresize of %x, curr size: %d, dsize: %d, req size: %d, old buf: %x, new buf: %x",
        d, d->size, d->dsize, size, d->buf, buf);
  if(buf == NULL) {
    return 0;
  }
  if(size < d->size) {
    print_backtrace();
    assert(0);
    free(buf);
  }
  /*
     sanitize the data size pointers 
   */
  if(size < d->size) {
    d->size = size;
    if(d->dsize < d->size) {
      d->dsize = d->size;
    }
  } else {
    // the new size is bigger, so just adjust the total size
    d->size = size;
  }
  d->buf = buf;
  dsetsig(d);
  debug(DBG_MEMORY, 10, "dresize, new size: %d, new dsize: %d",
        d->size, d->dsize);
  return 1;
}

/**
 * Grow the dbuf data buffer size by some amount
 *
 * @param d pointer to the dbuf
 * @param delta by how many bytes to grow the buffer.
 *
 * @return 0 if error
 * @return 1 if successful
 */

int
dgrow(dbuf_t * d, size_t delta)
{
  return dresize(d, d->size + delta);
}

/** 
 * Allocate a buffer with a copy of string in it 
 * (exclude the null terminator from dsize portion - *BUT*
 * put it into buffer, so the string functions will work
 *
 * @param str the source string
 *
 * @return pointer to the newly allocated dbuf
 *
 * @see dalloc dalloczf
 */

dbuf_t *
dstrcpy(const char *str)
{
  dbuf_t *temp = dalloc(strlen(str) + 1);

  if(temp) {
    strcpy((void *) temp->buf, str);
    temp->dsize = temp->size - 1;
  }
  return temp;
}

dbuf_t *
dsubstrcpy(dbuf_t *d, int start, int howmany)
{
  dbuf_t *temp = NULL;
  if ((start < d->dsize) && (start + howmany <= d->dsize)) {
    temp = dalloc(howmany);
    memcpy(temp->buf, &d->buf[start], howmany);
  }
  return temp;
}

/**
 * Concatenate a chunk of memory of a specified size
 * to the existing dbuf.
 *
 * @param d pointer to the target dbuf to append to
 * @param str source memory/string to append from
 * @param total how many bytes to copy (if -1 - then assume 
 *        the source memory is a string and figure it out.
 *
 * @return 0 if error
 * @return 1 if ok
 *
 * @see dstrcpy
 */

int
dmemcat(dbuf_t * d, void *str, int total)
{
  dchecksig(d);
  if(total == -1) {
    total = strlen(str);
  }
  if((d->size - d->dsize) < total + 1) {
    int delta = (1 + (1 + total) / d->growsize) * d->growsize;

    debug(DBG_GLOBAL, 250,
          "size: %d, dsize: %d, total: %d - need to grow", d->size,
          d->dsize, total);
    if(!dgrow(d, delta)) {
      debug(DBG_GLOBAL, 2, "failed to grow dbuf by %d bytes", delta);
      return 0;                 // failed to grow
    } else {
      debug(DBG_GLOBAL, 250, "grown the dbuf by %d bytes", delta);
    }
  }
  memcpy(&d->buf[d->dsize], str, total);
  d->dsize += total;
  return 1;
}

/**
 * Append a string to an existing dbuf - with putting the trailing
 * zero-terminator "just on the writing position"
 *
 * @param d pointer to the dbuf
 * @param str source string to append
 * @param total the length of the string
 *
 * @see dmemcat dstrcpy
 *
 */

int
dstrcat(dbuf_t * d, char *str, int total)
{
  dchecksig(d);
  dmemcat(d, str, total);
  if(d->dsize >= d->size) {
    dgrow(d, 10);
  }
  d->buf[d->dsize] = 0;         // null-terminate to keep the zero "ahead"
  return 1;
}

/**
 * Extended append from one dbuf into another dbuf
 *
 * @param target the dbuf to copy to
 * @param source the dbuf to copy from
 * @param start the starting offset of data in source
 * @param len the total amount of bytes to copy
 *
 * @return 0 error
 * @return 1 ok
 *
 * @see dstrcat
 *
 */

int
dxcat(dbuf_t * target, dbuf_t * source, int start, int len)
{
  dchecksig(target);
  dchecksig(source);
  if(len == -1) {
    len = source->dsize - start;
  }
  if(len < 0) {
    debug(DBG_GLOBAL, 0, "negative length to dxcat!");
    print_backtrace();
  }


  if(start + len > source->dsize) {
    debug(DBG_MEMORY, 10,
          "dxcat: Trying to copy % bytes from %d offset, but the total len is %d");
    return 0;
  }

  debug(DBG_MEMORY, 10, "dxcat of %d bytes, taken from pos %d\n", len, start);

  if(dgrow(target, len)) {
    memcpy(&target->buf[target->dsize], &(source->buf[start]), len);
    debug(DBG_MEMORY, 10, "dsize before grow: %d, delta: %d",
          target->dsize, source->dsize);
    target->dsize = target->dsize + len;
    debug(DBG_MEMORY, 10, "new dsize after grow: %d", target->dsize);
    return 1;
  } else {
    return 0;
  }
}

/**
 * append the data from source dbuf to the target dbuf 
 *
 * @param target destination dbuf to append to
 * @param source source dbuf to copy from
 *
 * @return from dxcat
 *
 * @see dxcat
 */

int
dconcat(dbuf_t * target, dbuf_t * source)
{
  return dxcat(target, source, 0, -1);
}

/**
 * Append-fill the dbuf with some value.
 *
 * @param target the dbuf to append to
 * @param c the value to append-fill
 * @param count the number of times c has to be appended
 *
 * @return 0 if error
 * @return otherwise, the number of times c was put inplace.
 *
 */

int
dappendfill(dbuf_t * target, char c, int count)
{
  void *sav;

  if(count == 0) {
    return 0;
  }
  debug(DBG_GLOBAL, 5, "dappendfill of %d, %d times\n", c, count);
  sav = target->buf;
  if(dgrow(target, count)) {
    memset(&target->buf[target->dsize], c, count);
    debug(DBG_MEMORY, 10, "dsize before grow: %d, delta: %d",
          target->dsize, count);
    target->dsize = target->dsize + count;
    debug(DBG_MEMORY, 10, "new dsize after grow: %d", target->dsize);
    debug(DBG_MEMORY, 10, "old buf before grow: %x, new buf: %x", sav,
          target->buf);
    return count;
  } else {
    return 0;
  }

}

/**
 * like a memscan but returns offset in dbuf.
 *
 * @param d pointer to the dbuf
 * @param c character to find
 * @param start from which index to start searching
 *
 * @return the index into the d, where the occurence happens.
 * otherwise return value is the index of the first-past-end character
 *
 * @see dmemcasescan
 */

int
dmemscan(dbuf_t * d, char c, int start)
{
  int i;

  for(i = start; (i < d->dsize) && (d->buf[i] != c); i++);      /* nothing */
  return i;
}

/**
 * same as memscan, except this one is case-insensitive.
 *
 * @see memscan
 *
 */

int
dmemcasescan(dbuf_t * d, char c, int start)
{
  int i;

  c = tolower(c);
  for(i = start; (i < d->dsize) && (tolower(d->buf[i]) != c); i++);     /* nothing */
  return i;
}

char _dprintf_temp_buf[1024];
void
dprintf(dbuf_t * d, char *fmt, ...)
{
  va_list ap;
  int total;

  va_start(ap, fmt);
  total = vsnprintf(_dprintf_temp_buf, sizeof(_dprintf_temp_buf), fmt, ap);
  debug(DBG_GLOBAL, 250, "buffer for dprintf: '%s' (%d bytes long)\n",
        _dprintf_temp_buf, total);
  dstrcat(d, _dprintf_temp_buf, total);
  va_end(ap);
}

int
dappendfile(dbuf_t * d, char *fname)
{
  int fd;
  struct stat filestat;
  int nread;
  int result = 1;

  fd = open(fname, O_RDONLY);
  if(fd != -1) {
    if(fstat(fd, &filestat) == 0) {
      debug(DBG_GLOBAL, 5, "file size: %d", filestat.st_size);
      if(dgrow(d, filestat.st_size)) {
        nread = read(fd, &d->buf[d->dsize], filestat.st_size);
        if(nread != filestat.st_size) {
          debug(DBG_GLOBAL, 1,
                " dappendfile %s: tried to load %d bytes, loaded %d bytes",
                fname, filestat.st_size, nread);
          result = 0;
        } else {
          d->dsize += nread;
        }
      } else {
        debug(DBG_GLOBAL, 1,
              "Could not dgrow by %d bytes to load file %s",
              filestat.st_size, fname);
        result = 0;
      }
    } else {
      debug(DBG_GLOBAL, 1, "Could not stat file %s, error: %s",
            fname, strerror(errno));
      result = 0;
    }
    close(fd);
  } else {
    debug(DBG_GLOBAL, 1, "Could not open the file %s, error: %s",
          fname, strerror(errno));
    result = 0;
  }
  return result;
}

/**
 * write the dbuf contents to a file
 */

int
dwritefile(dbuf_t * d, char *fname)
{
  int fd;
  long nwrote;

  fd = open(fname, O_WRONLY | O_CREAT);
  if(fd != -1) {
    nwrote = write(fd, d->buf, d->dsize);
    if(nwrote < d->dsize) {
      debug(DBG_GLOBAL, 1, "Wrote only %d bytes out of %d", nwrote, d->dsize);
    }
    close(fd);
    return 1;
  } else {
    debug(DBG_GLOBAL, 0, "Could not open file %s for writing to save dbuf %x",
          fname, d);
    return 0;
  }
}



dbufr_t *
dropen(dbuf_t * dbuf)
{
  dbufr_t *r = malloc(sizeof(dbufr_t));

  r->d = dbuf;
  r->offset = 0;
  return r;
}

void
drclose(dbufr_t * dr)
{
  free(dr);
};

/*@}*/
