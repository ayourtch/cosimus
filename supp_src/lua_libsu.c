#include "lib_dbuf.h"
#include "lib_debug.h"
#include "lib_sock.h"
#include "lib_os.h"
#include "lib_httpd.h"
#include <stdlib.h>
#include <lauxlib.h>


int lua_pcall_with_debug_ex(lua_State *L, int nargs, int nresults, int dbgtype, int level, char *file, int lineno)
{
  int err = lua_pcall(L, nargs, nresults, 0);
  if(err != 0) {
    debug(dbgtype, level, "Lua error while performing lua_pcall at %s:%d: %s",
         file, lineno, lua_tostring(L, 1));
  }
  return err;
}

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

static int lua_fn_set_debug_level(lua_State *L)
{
  int dbg = luaL_checkint(L, 1);
  int level = luaL_checkint(L, 2);
  int old_level = set_debug_level(dbg, level);
  lua_pushinteger(L, old_level);
  return 1;
}

static int lua_fn_print_dbuf(lua_State *L)
{
  int logtype = luaL_checkint(L, 1);
  int loglevel = luaL_checkint(L, 2);
  dbuf_t *d = lua_checkdbuf(L, 3);
  print_dbuf(logtype, loglevel, d);
  return 0;
}

static int lua_fn_dalloc(lua_State *L)
{
  int size = luaL_checkint(L, 1);
  dbuf_t *d = dalloc(size);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int lua_fn_dunlock(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  dunlock(d);
  return 0;
}

static int lua_fn_dlock(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  dlock(d);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int lua_fn_dstr(lua_State *L)
{
  int len;
  const char *str = luaL_checklstring(L, 1, &len);
  dbuf_t *d = dalloc(len+10000);
  dmemcat(d, (char *)str, len);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int lua_fn_dgetstr(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  lua_pushlstring(L, d->buf, d->dsize);
  return 1;
}

static int lua_fn_dstrcat(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  int len;
  const char *str = luaL_checklstring(L, 2, &len);
  dmemcat(d, (char *)str, len);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int 
lua_fn_cdata_get_remote4(lua_State *L) {
  int idx = luaL_checkint(L, 1);
  uint32_t addr;
  uint16_t port;
  if (cdata_get_remote4(idx, &addr, &port)) {
    lua_pushinteger(L, addr);
    lua_pushinteger(L, port);
    return 2;
  } else {
    return 0;
  }
}

static int 
lua_fn_cdata_check_remote4(lua_State *L) {
  int idx = luaL_checkint(L, 1);
  int addr = luaL_checkint(L, 2);
  int port = luaL_checkint(L, 3);
  lua_pushinteger(L, cdata_check_remote4(idx, addr, port));
  return 1;
}

static int 
lua_fn_sock_send_data(lua_State *L) {
  int idx = luaL_checkint(L, 1);
  dbuf_t *d = lua_checkdbuf(L, 2);
  int nwrote = sock_send_data(idx, d);
  lua_pushinteger(L, nwrote);
  return 1;
}

static int
lua_fn_run_cycles(lua_State *L) {
  int total_counter = luaL_checkint(L, 1);
  int counter = total_counter;
  int total_events = 0;
  while(counter != 0) {
    total_events += sock_one_cycle(1000, (void *)L);
    if(counter > 0) {
      counter--;
    }
  }
  // Crude average load
  lua_pushinteger(L, (float)total_events / (float)total_counter);
  return 1;
}

static int
lua_fn_http_start_listener(lua_State *L)
{
  const char *addr = luaL_checkstring(L, 1);
  int port = luaL_checkint(L, 2);
  http_start_listener((void *)addr, port, NULL);
  return 0;
}

static int
lua_fn_libsupp_init(lua_State *L)
{
  void *ptr = NULL;
  if (!lua_isnil(L, 1)) {
    ptr = lua_touserdata(L, 1);
  }
  ptr = libsupp_init(ptr);
  lua_pushlightuserdata(L, ptr);
  return 1;
}

static int
lua_fn_get_file_mtime(lua_State *L)
{
  char *fname = (void *)luaL_checkstring(L, 1);
  time_t mtime = get_file_mtime(fname);
  if (mtime > 0) {
    lua_pushnumber(L, mtime);
  } else {
    lua_pushnil(L);
  }
  return 1;
}


static const luaL_reg su_lib[] = {
  {"set_debug_level", lua_fn_set_debug_level },
  {"print_dbuf", lua_fn_print_dbuf },
  {"dalloc", lua_fn_dalloc },
  {"dlock", lua_fn_dlock },
  {"dunlock", lua_fn_dunlock },
  {"dstr", lua_fn_dstr },
  {"dgetstr", lua_fn_dgetstr },
  {"dstrcat", lua_fn_dstrcat },

  {"cdata_get_remote4", lua_fn_cdata_get_remote4 },
  {"cdata_check_remote4", lua_fn_cdata_check_remote4 },
  {"sock_send_data", lua_fn_sock_send_data },
  { "run_cycles", lua_fn_run_cycles },
  { "http_start_listener", lua_fn_http_start_listener },
  { "libsupp_init", lua_fn_libsupp_init },
  { "get_file_mtime", lua_fn_get_file_mtime },

  {NULL, NULL}
};

LUA_API int luaopen_libsupp (lua_State *L) {
  luaL_openlib(L, "su", su_lib, 0);

  lua_getglobal(L, "su");
  lua_pushstring(L, "version");
  lua_pushstring(L, "1.0");
  lua_settable(L,-3);
  
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
