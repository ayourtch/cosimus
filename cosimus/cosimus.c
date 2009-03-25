#include <stdio.h>
#include <stdlib.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "lib_debug.h"
#include "lib_uuid.h"
#include "lib_httpd.h"
#include "fmv.h"
#include "pktsmv.h"
#include "libsupp.h"


int main(int argc, char *argv[]) 
{
  lua_State *L;
  //set_debug_level(DBG_GLOBAL, 100);
  debug(0,0, "Hello there, sizeof of uuid_t: %d!", sizeof(uuid_t));
  http_start_listener("127.0.0.1", 12346, NULL);
  
  L = lua_open();
  luaL_openlibs(L);
  luaopen_libfmv(L);
  luaopen_libpktsmv(L);
  luaopen_libsupp(L);
  if (luaL_dofile(L,argv[1])!=0) {
    fprintf(stderr,"%s\n",lua_tostring(L,-1));
  } else {
    while (1) {
      sock_one_cycle(1000, L);
    }
  }
  lua_close(L);
  return 0;
}
