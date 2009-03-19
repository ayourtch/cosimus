#include "lib_debug.h"
#include "lib_dbuf.h"
#include "lib_sock.h"
#include "lib_uuid.h"
#include "fmv.h"
#include "lua.h"


static int smv_packet(int idx, dbuf_t *d, void *ptr) {
  int err;
  lua_State *L = ptr;
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

static const luaL_reg smvlib[] = {
  { "start_listener", lua_fn_start_listener },
  { "run_cycles", lua_fn_run_cycles },
  { NULL, NULL }
};

LUA_API int luaopen_smv (lua_State *L) {
  luaL_openlib(L, "smv", smvlib, 0);
  return 1;
}

