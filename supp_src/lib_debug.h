
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
