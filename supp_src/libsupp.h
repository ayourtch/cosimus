#ifndef _LIBSUPP_H_
#define _LIBSUPP_H_

#include "lua.h"

#define lua_pcall_with_debug(L, nargs, nres, dbg, lvl) lua_pcall_with_debug_ex(L, nargs, nres, dbg, lvl, __FILE__, __LINE__)
int lua_pcall_with_debug_ex(lua_State *L, int nargs, int nresults, int dbgtype, int level, char *file, int lineno);
#endif
