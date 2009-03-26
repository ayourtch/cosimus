#include <stdio.h>
#include <stdlib.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "luazip.h"

#include "lib_debug.h"
#include "lib_uuid.h"
#include "lib_httpd.h"
#include "lib_os.h"
#include "fmv.h"
#include "pktsmv.h"
#include "libsupp.h"

LUALIB_API int luaopen_base64(lua_State *L);


lua_State *L;
int dump_and_leave = 0;

void
sigterm_handler(int x)
{
  debug(DBG_GLOBAL, 0, "SIG! param: %d", x);
  dump_and_leave = 1;
}


int main(int argc, char *argv[]) 
{
  //set_debug_level(DBG_GLOBAL, 100);
  set_signal_handler(SIGINT, sigterm_handler);
  
  L = lua_open();
  luaL_openlibs(L);
  luaopen_zip(L);
  luaopen_base64(L);
  luaopen_libfmv(L);
  luaopen_libpktsmv(L);
  luaopen_libsupp(L);
  if (luaL_dofile(L,argv[1])!=0) {
    fprintf(stderr,"%s\n",lua_tostring(L,-1));
  } else {
    while (!dump_and_leave) {
      sock_one_cycle(1000, L);
    }
    lua_getglobal(L, "interrupt_save_state");
    lua_pcall_with_debug(L, 0, 0, 0, 0);
  }
  lua_close(L);
  exit(1);
  return 0;
}
