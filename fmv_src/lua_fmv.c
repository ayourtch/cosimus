#include "sta_fmv.h"
#include "lua_fmv.h"
#include "lib_dbuf.h"

void *luaL_checkuserdata(lua_State *L, int n)
{
  if(lua_isuserdata(L, n)) {
    return lua_touserdata(L, n);
  } else {
    luaL_error(L, "parameter %d should be a dbuf ", n);
    return NULL;
  }
}

static int
lua_fn_packet_new(lua_State *L) {
  dbuf_t *d = dalloc(1500);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int
lua_fn_packet_lock(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  dlock(d);
  lua_pushlightuserdata(L, d);
  return 1;
}

static int
lua_fn_packet_unlock(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  dunlock(d);
  return 0;
}

static int
lua_fn_GetSequenceNumber(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  lua_pushinteger(L, GetSequenceNumber(d->buf));
  return 1;
}

static int
lua_fn_SetSequenceNumber(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  u32t seq = luaL_checkint(L, 2);
  SetSequenceNumber(d->buf, seq);
  return 0;
}


const luaL_reg fmv_sta_lib[] = {
  { "packet_new", lua_fn_packet_new },
  { "packet_lock", lua_fn_packet_lock },
  { "packet_unlock", lua_fn_packet_unlock },
  { "GetSequenceNumber", lua_fn_GetSequenceNumber },
  { "SetSequenceNumber", lua_fn_SetSequenceNumber },
  { NULL, NULL }
};

