
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



#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/errno.h>
#include <execinfo.h>

#include "lib_debug.h"



/**
 * @defgroup debug Debugging facilities
 */

 /*
    @{ 
  */

/** an array for storing which debug levels are currently active */
static int debuglevels[65536];

/** 
 * the "global" debug level for the debug types 
 * that are not within the array 
 */
static int debuglevel = 0;

int
set_debug_level(int type, int level)
{
  int ret;

  if(type >= 0) {
    ret = debuglevels[type];
    debuglevels[type] = level;
  } else {
    ret = debuglevel;
    debuglevel = level;
  }
  return ret;
}

int
get_debug_level()
{
  return debuglevel;
}


int
is_debug_on(int type, int level)
{
  if((type < 65536 && debuglevels[type] >= level)
     || (type >= 65536 && debuglevel >= level)) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * Routine to print the debug messages with timestamp.
 *
 * @param type the type of the debug
 * @param level and the level of the debug message (0=always shown)
 * @param fmt printf-style format string, followed by additional parameter
 *
 * @return the number of characters printed from the debug message
 * @see debug_dump
 */
int
debug(int type, int level, const char *fmt, ...)
{
  va_list ap;
  int result = 0;
  //struct tm *tm;
  struct timeval tv;
  char date_buf[256];

  if(is_debug_on(type, level)) {

    gettimeofday(&tv, NULL);
    asctime_r(localtime(&tv.tv_sec), date_buf);
    date_buf[strlen(date_buf) - 6] = 0;


    fprintf(stderr, "%s.%06d LOG-%04d-%04d: ", date_buf, (int) tv.tv_usec,
            type, level);
    va_start(ap, fmt);
    result = vfprintf(stderr, fmt, ap);
    /* CONSOLEXXX

    if(need_console_debugs()) {
      console_printf("\n%s.%06d LOG-%04d-%04d: ", date_buf, (int) tv.tv_usec,
                     type, level);
      console_vprintf(fmt, ap);
      console_putstr("\n");
    }
    */
    va_end(ap);
    fprintf(stderr, "\n");
  }
  return result;
}

/**
 * Print the hexdump of a the memory if the debug level is appropriate
 *
 * @see debug
 *
 * @param type the type of the debug
 * @param level the level at which to print this dump
 * @param addr address of the block of memory to dump
 * @param len how big is the block of memory to dump
 * @return always returns 1
 */

int
debug_dump(int type, int level, void *addr, int len)
{
  char tohex[] = "0123456789ABCDEF";
  int i = 0;
  unsigned char *pc = addr;

  char buf0[32];                // offset
  char buf1[64];                // hex
  char buf2[64];                // literal

  char *pc1;
  char *pc2;

  // if (debuglevel < level) {
  if(!is_debug_on(type, level)) {
    return 0;
  }

  while(--len >= 0) {
    if(i % 16 == 0) {
      sprintf(buf0, "%08x", i);
      buf1[0] = 0;
      buf2[0] = 0;
      pc1 = buf1;
      pc2 = buf2;
    }
    *pc1++ = tohex[*pc >> 4];
    *pc1++ = tohex[*pc & 15];
    *pc1++ = ' ';

    if(*pc >= 32 && *pc < 127) {
      *pc2++ = *pc;
    } else {
      *pc2++ = '.';
    }
    i++;
    pc++;
    if(i % 16 == 0) {
      *pc1 = 0;
      *pc2 = 0;
      debug(type, level, "%s:   %s  %s", buf0, buf1, buf2);
    }

  }
  if(i % 16 != 0) {
    while(i % 16 != 0) {
      *pc1++ = ' ';
      *pc1++ = ' ';
      *pc1++ = ' ';
      *pc2++ = ' ';
      i++;
    }
    *pc1 = 0;
    *pc2 = 0;
    debug(type, level, "%s:   %s  %s", buf0, buf1, buf2);
  }
  return 1;
}

/**
 * Simplistic error check function for fatal errors - checks 
 * the supplied value to be nonzero. 
 * If it is negative, print the supplied message, 
 * current string for errno,and die.
 *
 * @param x the value to test to be nonnegative
 * @param msg the error message to print
 */

void
notminus(int x, char *msg)
{
  if(x < 0) {
    debug(DBG_GLOBAL, 0, "ERROR: %s, system error: %s", msg, strerror(errno));
    exit(1);
  }
}

/** 
 * Put the backtrace into the memory
 * @param bt pointer to the backtrace array
 */

void
get_backtrace(backtrace_t * bt)
{
  bt->size = backtrace(bt->addresses, sizeof(bt->addresses) / sizeof(void *));
}

/**
 * return the symbolic name of the function pointer, the caller needs to free the memory!
 * param fptr function pointer
 */
char *get_symbol_name(void *fptr)
{
  char **strings;
  char *str;
  strings = backtrace_symbols(&fptr, 1);
  str = strdup(strings[0]);
  free(strings);
  return str;
}

/**
 * Obtain a backtrace and print it to stdout. 
 * Borrowed from http://www.gnu.org/software/libtool/manual/libc/Backtraces.html 
 */
void
print_backtrace_t(int debugtype, int debuglevel, backtrace_t * bt)
{
  char **strings;
  size_t i;

  strings = backtrace_symbols(bt->addresses, bt->size);
  debug(debugtype, debuglevel, "backtrace of %zd stack frames.", bt->size);
  for(i = 0; i < bt->size; i++)
    debug(debugtype, debuglevel, "%d: %s", i, strings[i]);
  free(strings);
}

void
print_backtrace_t_hex(int debugtype, int debuglevel, backtrace_t * bt)
{
  int i;

  debug(debugtype, debuglevel, "backtrace of %zd stack frames.", bt->size);
  for(i = 0; i < bt->size; i++)
    debug(debugtype, debuglevel, "%d: %x", i, bt->addresses[i]);
}


void
print_backtrace(void)
{
  backtrace_t bt;

  get_backtrace(&bt);
  debug(DBG_GLOBAL, 0, "Current backtrace:");
  print_backtrace_t(0, 0, &bt);
}

/*@}*/
