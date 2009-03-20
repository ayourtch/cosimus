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

    lua_getglobal(L1, "libreload");
    lua_pushstring(L1, "parent_L");
    // printf("Setting Lua parent to %x\n", (unsigned int)L);
    lua_pushlightuserdata(L1, L);
    lua_settable(L1,-3);
    lua_pop(L1, 1);

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

/*
 * Pass some userdata to parent
 * execute the function in parent's space with two parameters
 * string and userdata
 */
static int
fn_parent_run(lua_State *L1) {
  lua_State *L;
  char *func = (void *)luaL_checkstring(L1, 1);
  char *s_arg = (void *)luaL_checkstring(L1, 2);
  void *p_arg = lua_touserdata(L1, 3);
  int err;

  lua_getglobal(L1, "libreload");
  lua_pushstring(L1, "parent_L");
  lua_gettable(L1, -2);
  L = lua_touserdata(L1, -1);
  lua_pop(L1, 1);
  //printf("Lua parent from parent_run: %x\n", (unsigned int) L);

  if (L) {
    //printf("Calling: %s with arg %s\n", func, (char *)s_arg);
    lua_getglobal(L, func);
    lua_pushstring(L, s_arg);
    lua_pushlightuserdata(L, p_arg);
    err = lua_pcall(L, 2, 1, 0);
    if (err) {
      printf("Lua parent error: %s\n", lua_tostring(L, -1));
    }
    p_arg = lua_touserdata(L, -1);
    if (p_arg) {
      lua_pushlightuserdata(L1, p_arg);
    } else {
      lua_pushnil(L1);
    }
  } else {
    lua_pushnil(L1);
  }
  return 1;
}


static const luaL_reg libreload[] = {
  {"child", fn_child },
  {"parent_run", fn_parent_run },
  {NULL, NULL}
};

LUA_API int luaopen_libreload (lua_State *L) {
  luaL_openlib(L, "libreload", libreload, 0);
  return 1;
}
