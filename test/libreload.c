#include <stdlib.h>
#include "lua.h"
#include "lualib.h"
#include <lauxlib.h>

static int
fn_child(lua_State *L) {
  int err;
  const char *fname = luaL_checkstring(L, 1);
  const char *arg = luaL_checkstring(L, 2);
  lua_State *L1 = lua_open();
  luaL_openlibs(L1);
  lua_getglobal(L1, "require");
  lua_pushstring(L1, fname);
  err = lua_pcall(L1, 1, 0, 0);
  if(err) {
    lua_pushstring(L, "error_require");
    lua_pushstring(L, lua_tostring(L1, -1));
  } else {
    lua_getglobal(L1, "main");
    lua_pushstring(L1, arg);
    err = lua_pcall(L1, 1, 1, 0);
    if(err) {
      lua_pushstring(L, "error_run");
      lua_pushstring(L, lua_tostring(L1, -1));
    } else {
      lua_pushstring(L, "return");
      lua_pushstring(L, lua_tostring(L1, -1));
    }
  }
  lua_close(L1);
  return 2;
}


static const luaL_reg libreload[] = {
  {"child", fn_child },
  {NULL, NULL}
};

LUA_API int luaopen_libreload (lua_State *L) {
  luaL_openlib(L, "reload", libreload, 0);
  return 1;
}
