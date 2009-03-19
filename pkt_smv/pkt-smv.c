#include "lib_debug.h"
#include "lib_dbuf.h"
#include "lib_sock.h"
#include "lib_uuid.h"
#include "fmv.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


lua_State *L;

int smv_packet(int idx, dbuf_t *d) {
  int err;
  debug(0,0, "Packet: %s", global_id_str(get_packet_global_id(d->buf)));
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

int start_smv_listener(char *addr, int port) {
  sock_handlers_t *h;
  int idx;

  idx = bind_udp_listener_specific(addr, port, NULL);
  h = cdata_get_handlers(idx);
  h->ev_read = smv_packet;
  return idx;
}

int main(int argc, char *argv[])
{
  // set_debug_level(DBG_GLOBAL, 100);
  debug(0,0, "Starting SL(tm)-compatible packet listener");
  L = lua_open();
  luaL_openlibs(L);

  if(luaL_dofile(L,"startup.lua")!=0) {
    debug(0,0, "Lua error: %s", lua_tostring(L,-1));
    lua_close(L);
    exit(1);
  }

  start_smv_listener("0.0.0.0", 9000);

  while (1) {
    sock_one_cycle(1000);
  }
  return 1;
}
