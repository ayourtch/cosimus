#ifndef _LUA_FMV_H_
#define _LUA_FMV_H_

#include "lua.h"
#include <lauxlib.h>
#include <lualib.h>


void *luaL_checkuserdata(lua_State *L, int n);
extern const luaL_reg fmv_sta_lib[];

#endif
