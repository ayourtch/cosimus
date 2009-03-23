#include "lib_debug.h"
#include "lib_dbuf.h"
#include "lib_sock.h"
#include "lib_uuid.h"
#include "fmv.h"
#include "lua.h"


static int smv_packet(int idx, dbuf_t *d0, void *ptr) {
  int err;
  dbuf_t *d;
  lua_State *L = ptr;

  d = MaybeZeroDecodePacket(d0);
  lua_getglobal(L, "smv_packet");
  lua_pushnumber(L, idx);
  lua_pushlightuserdata(L, d);

  err = lua_pcall(L, 2, 1, 0);
  if(err) {
    debug(DBG_GLOBAL, 0, "Lua error: %s", lua_tostring(L,-1));
    lua_pop(L, 1);
  } else {
    lua_pop(L, 1);
  }
  dunlock(d);
  return 1;
}

static int 
lua_fn_start_listener(lua_State *L) {
  const char *addr = luaL_checkstring(L, 1);
  int port = luaL_checkint(L, 2);
  sock_handlers_t *h;
  int idx;

  idx = bind_udp_listener_specific((void*)addr, port, NULL);
  h = cdata_get_handlers(idx);
  h->ev_read = smv_packet;
  return idx;
}

static const luaL_reg smvlib[] = {
  { "start_listener", lua_fn_start_listener },
  { NULL, NULL }
};

LUA_API int luaopen_libpktsmv (lua_State *L) {
  luaL_openlib(L, "smv", smvlib, 0);
  return 1;
}

