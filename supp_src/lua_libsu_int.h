#ifndef __LUA_LIBSU_H__
#define __LUA_LIBSU_H__

#include "lua.h"

typedef struct appdata_lua_outgoing_tcp_t_tag {
  char *lua_handler_name;
  void *L; 
} appdata_lua_outgoing_tcp_t;

#endif
