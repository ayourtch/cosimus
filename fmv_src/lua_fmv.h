#ifndef _LUA_FMV_H_
#define _LUA_FMV_H_

#include "lua.h"
#include <lauxlib.h>
#include <lualib.h>
#include "lib_uuid.h"
#include "sta_fmv.h"


void *luaL_checkuserdata(lua_State *L, int n);
extern const luaL_reg fmv_sta_lib[];

void luaL_checkx_uuid(lua_State *L, int narg, uuid_t *uuid);
void lua_pushx_uuid(lua_State *L, uuid_t *uuid);
u16t luaL_checkx_ipport(lua_State *L, int narg);
int luaL_checkx_bool(lua_State *L, int narg);
u64t luaL_checkx_u64(lua_State *L, int narg);
void lua_pushx_u64(lua_State *L, u64t u64);

/*
void
lua_pushx_ipaddr(lua_State *L, u32t ipaddr) {
  struct in_addr in_addr;
  in_addr.s_addr = ipaddr;
  lua_pushstring(L, inet_ntoa(in_addr));
}

u32t
luaL_checkx_ipaddr(lua_State *L, int narg) {
  struct in_addr in_addr;
  const char *s = luaL_checkstring(L, narg);
  if(0 == inet_aton(s, &in_addr)) {
    luaL_error(L, "Expected IP Address as arg %d, got: '%s'", narg, s);
  }
  return in_addr.s_addr;
}
*/


#endif
