
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

#ifndef __LIB_DEBUG_H__
#define __LIB_DEBUG_H__

#define soft_assert(x) { if (!(x)) { debug(DBG_GLOBAL, 1, "Soft assert failed at %s:%d: %s", __FILE__, __LINE__, __STRING(x)); } }


enum {
  DBG_GLOBAL_DEBUG_LEVEL = -1,
  DBG_GLOBAL = 1000,
  DBG_SLPKT_TEMPLATE,
  DBG_SSL,
  DBG_TIMERS,
  DBG_CONSOLE,
  DBG_SLIST,
  DBG_SLPKT_TEMPLATE_MEM = 1013,
  DBG_MEMORY,
  DBG_SLPKT_TMP = 2000,
  DBG_SLPKT = 2001,
  DBG_HA_GENERAL = 4000,
  DBG_HA_IMAGE,
  DBG_HA_CHAT,
  DBG_HA_SIMULATOR,
  DBG_HA_TERRAIN,
  DBG_HA_OBJECT,
};

/**
 * structure to store the backtrace
 */
typedef struct {
  int size;
  void *addresses[10];
} backtrace_t;



int debug(int type, int level, const char *fmt, ...);
int set_debug_level(int type, int level);
int get_debug_level();
int is_debug_on(int type, int level);
int debug_dump(int type, int level, void *addr, int len);
char *get_symbol_name(void *fptr);
void print_backtrace(void);
void print_backtrace_t(int debugtype, int debuglevel, backtrace_t * bt);
void get_backtrace(backtrace_t * bt);
void notminus(int x, char *msg);




#endif
