#include "lib_dbuf.h"
#include "lib_debug.h"
#include <stdlib.h>
#include <lauxlib.h>


static void *lua_checkdbuf(lua_State *L, int index)
{
  if(lua_islightuserdata(L, index)) {
    return lua_touserdata(L, index);
  } else {
    lua_pushstring(L, "dbuf expected as first argument");
    lua_error(L);
    return NULL;
  }

}

static int fn_set_debug_level(lua_State *L)
{
  int dbg = luaL_checkint(L, 1);
  int level = luaL_checkint(L, 2);
  int old_level = set_debug_level(dbg, level);
  lua_pushinteger(L, old_level);
  return 1;
}

static int fn_print_dbuf(lua_State *L)
{
  int logtype = luaL_checkint(L, 1);
  int loglevel = luaL_checkint(L, 2);
  dbuf_t *d = lua_checkdbuf(L, 3);
  print_dbuf(logtype, loglevel, d);
  return 0;
}

static int fn_dalloc(lua_State *L)
{
  int size = luaL_checkint(L, 1);
  dbuf_t *d = dalloc(size);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int fn_dunlock(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  dunlock(d);
  return 0;
}

static int fn_dlock(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  dlock(d);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int fn_dstr(lua_State *L)
{
  int len;
  const char *str = luaL_checklstring(L, 1, &len);
  dbuf_t *d = dalloc(len+10000);
  dmemcat(d, (char *)str, len);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int fn_dgetstr(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  lua_pushlstring(L, d->buf, d->dsize);
  return 1;
}

static int fn_dstrcat(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  int len;
  const char *str = luaL_checklstring(L, 2, &len);
  dmemcat(d, (char *)str, len);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int fn_test(lua_State *L)
{
  printf("test 1.4\n");
  return 0;
}

static const luaL_reg su_lib[] = {
  {"set_debug_level", fn_set_debug_level },
  {"print_dbuf", fn_print_dbuf },
  {"dalloc", fn_dalloc },
  {"dlock", fn_dlock },
  {"dunlock", fn_dunlock },
  {"dstr", fn_dstr },
  {"dgetstr", fn_dgetstr },
  {"dstrcat", fn_dstrcat },
  {"test", fn_test },
  {NULL, NULL}
};

LUA_API int luaopen_tstlib (lua_State *L) {
  luaL_openlib(L, "su", su_lib, 0);

 /* lua_getglobal(L, "su");
  lua_pushstring(L, "version");
  lua_pushstring(L, "1.0");
  lua_settable(L,-3);*/
  
  {
    /*
    int i=0;
    while(su_lib_str[i].str)
    {
      lua_pushstring(L, su_lib_str[i].str);
      lua_pushinteger(L, su_lib_str[i].value);
      lua_settable(L, -3);
      i++;
    }
    */
  }

  return 1;
}
