#include "lib_dbuf.h"
#include "lib_debug.h"
#include "lib_sock.h"
#include "lib_os.h"
#include "lib_httpd.h"
#include <stdlib.h>
#include <lauxlib.h>
#include "lua_libsu_int.h"
#include "libsupp.h"


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
    luaL_error(L, "dbuf expected as first argument");
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

static int lua_fn_dcheck(lua_State *L)
{
  dbuf_t *d = lua_checkdbuf(L, 1);
  if(d->dsize > d->size) {
   // luaL_error(L, "dsize > size")
   lua_pushnumber(L, d->dsize);
   lua_pushnumber(L, d->size);
   return 2;
  } else {
   return 0;
  }
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

char *tcp_connect_sig = "tcp_connect_outgoing";

// Common event handler that calls Lua for outgoing TCP connection events
static int
tcp_connect_handler_call(int idx, char *event, dbuf_t *data)
{
  appdata_lua_outgoing_tcp_t *ad;
  dbuf_t *d = cdata_get_appdata_dbuf(idx, tcp_connect_sig);
  lua_State *L;
  int result = 0;
  if (d) {
    ad = (void*) d->buf;
    L = ad->L;
    lua_getglobal(L, ad->lua_handler_name);
    lua_pushnumber(L, idx);
    lua_pushstring(L, event);
    lua_pushlightuserdata(L, data);
    lua_pcall_with_debug(L, 3, 1, 0, 0);
    result = lua_tonumber(L, 1);
    lua_pop(L, 1);
  } else {
    debug(0, 0, "TCP connect handler idx %d, corrupt or absent appdata", idx);
    result = -1;
  }
  return result;
}

static void 
ev_tcp_connect_channel_ready(int idx, void *u_ptr)
{
  debug(0, 0, "channel ready!");
  tcp_connect_handler_call(idx, "channel_ready", NULL);
}

static int
ev_tcp_connect_read(int idx, dbuf_t *d, void *u_ptr)
{
  return tcp_connect_handler_call(idx, "read", d);
}

static int
ev_tcp_connect_closed(int idx, void *u_ptr)
{
  appdata_lua_outgoing_tcp_t *ad;
  dbuf_t *d = cdata_get_appdata_dbuf(idx, tcp_connect_sig);
  int ret = tcp_connect_handler_call(idx, "closed", NULL);
  if (d) {
    ad = (void*) d->buf;
    free(ad->lua_handler_name);
  }
  return ret;
}

static int
ev_tcp_newconn(int idx, int parent, void *u_ptr)
{
  return tcp_connect_handler_call(idx, "newconn", NULL);
}

static int 
lua_fn_sock_tcp_connect(lua_State *L) {
  int error = 0;
  char *addr = (void *)luaL_checkstring(L, 1);
  int port = luaL_checkint(L, 2);
  char *lua_handler_name = strdup(luaL_checkstring(L, 3));
  int idx = initiate_connect(addr, port);

  if (idx >= 0) {
    appdata_lua_outgoing_tcp_t *ad;
    dbuf_t *d = dsetusig(dalloczf(sizeof(appdata_http_t)), tcp_connect_sig);
    if (d) {
      sock_handlers_t *h;
      ad = (void*)d->buf;
      ad->lua_handler_name = lua_handler_name;
      ad->L = L;
      cdata_set_appdata_dbuf(idx, d);
      h = cdata_get_handlers(idx);
      h->ev_channel_ready = ev_tcp_connect_channel_ready;
      h->ev_read = ev_tcp_connect_read;
      h->ev_closed = ev_tcp_connect_closed;
      h->ev_newconn = ev_tcp_newconn;
      debug(0, 0, "Set the new socket with handler: %s", lua_handler_name);

      lua_pushnumber(L, idx);
    } else {
      error = 1;
    }
  } else {
    error = 1;
  }
  if (error) {
    free(lua_handler_name);
    lua_pushnil(L);
  }
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
lua_fn_libsupp_init(lua_State *L)
{
  void *ptr = NULL;
  if (!lua_isnil(L, 1)) {
    ptr = lua_touserdata(L, 1);
  }
  return 0;
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

static int 
lua_fn_get_http_data(lua_State *L)
{
  int n = lua_gettop(L);
  char *key;
  dbuf_t *d;
  appdata_http_t *appdata;
  if(n == 2 && lua_isuserdata(L, -2) ) {
    d = (void *)lua_touserdata(L, -2);
    key = (void*) lua_tostring(L, -1);
    if( (appdata = http_dbuf_get_appdata(d)) ) {
      if(strcmp(key, "querystring") == 0) {
        lua_pushstring(L, appdata->http_querystring);
      } else if(strcmp(key, "referer") == 0) {
        lua_pushstring(L, appdata->http_referer);
      } else if(strcmp(key, "http_11") == 0) {
        lua_pushnumber(L, appdata->http_11);
      } else if(strcmp(key, "method") == 0) {
        lua_pushnil(L);
      } else if(strcmp(key, "post_content_length") == 0) {
        lua_pushnumber(L, appdata->post_content_length);
      } else if(strcmp(key, "post_data_length") == 0) {
        lua_pushnumber(L, appdata->post_content_got_length);
      } else if(strcmp(key, "post_data") == 0) {
        if (appdata->post_content_buf) {
          lua_pushlstring(L, appdata->post_content_buf->buf, appdata->post_content_buf->dsize);
        } else {
          lua_pushnil(L);
        }
      }
    } else {
      lua_pushstring(L, "Wrong kind of userdata supplied");
      lua_error(L);
    }
  } else {
    lua_pushstring(L, "Need two arguments to get http data: userdata and field name");
    lua_error(L);
  }
  return 1; /* single result */
}

static int lua_http_handler(dbuf_t *dad, dbuf_t *dh, dbuf_t *dd)
{
  lua_State *L;
  char *body;
  size_t body_len;
  char *header;
  size_t header_len;
  int err;
  appdata_http_t *ad = http_dbuf_get_appdata(dad);

  if(ad) {
    L = ad->L;
    lua_getglobal(L, ad->lua_handler_name);
    lua_pushstring(L, ad->http_path);
    lua_pushlightuserdata(L, dad);
    lua_pushlightuserdata(L, dh);
    lua_pushlightuserdata(L, dd);
    printf("Lua handler\n");
    err = lua_pcall(L, 4, 2, 0);
    if(err) {
      debug(DBG_GLOBAL, 0, "Lua error: %s", lua_tostring(L,-1));
      lua_pop(L, 1);
    } else {
      /* get the result */
      body = (void*) lua_tolstring(L, -1, &body_len);
      header = (void*) lua_tolstring(L, -2, &header_len);
      dmemcat(dd, body, body_len);
      dmemcat(dh, header, header_len);
      dprintf(dh, "\r\n");
      lua_pop(L, 1);
      lua_pop(L, 1);
      debug(DBG_GLOBAL, 3, "Explicitly returned %d header chars, %d body chars\n", header_len, body_len);
    }
  } else {
    debug(DBG_GLOBAL, 0, "Wrong http appdata supplied to handler");
  }

  return 0;
}


http_handler_func_t lua_http_dispatcher(dbuf_t *dad)
{
  appdata_http_t *ad = http_dbuf_get_appdata(dad);
  if (ad) {
    printf("Lua dispatcher called for URI: %s\n", ad->http_path);
    return lua_http_handler;
  } else {
    return NULL;
  }
}


int http_start_listener_lua(char *addr, int port, http_dispatcher_func_t dispatcher, lua_State *L, char *lua_handler_name)
{
  int idx = http_start_listener(addr, port, dispatcher);
  if (idx >= 0) {
    dbuf_t *dp = cdata_get_appdata_dbuf(idx, http_appdata_sig);
    appdata_http_t *ad = http_dbuf_get_appdata(dp);
    ad->lua_handler_name = strdup(lua_handler_name);
    ad->L = L;
  }
  return idx; 
}

static int
lua_fn_http_start_listener(lua_State *L)
{
  char *addr = (void *)luaL_checkstring(L, 1);
  int port = luaL_checkint(L, 2);
  char *handler_name = (void *) luaL_checkstring(L, 3);
  int result = (0 <= http_start_listener_lua(addr, port, lua_http_dispatcher, L, handler_name));
  lua_pushboolean(L, result);
  return 1;
}




static const luaL_reg su_lib[] = {
  {"set_debug_level", lua_fn_set_debug_level },
  {"print_dbuf", lua_fn_print_dbuf },
  {"dalloc", lua_fn_dalloc },
  {"dlock", lua_fn_dlock },
  {"dunlock", lua_fn_dunlock },
  {"dcheck", lua_fn_dcheck },
  {"dstr", lua_fn_dstr },
  {"dgetstr", lua_fn_dgetstr },
  {"dstrcat", lua_fn_dstrcat },

  {"cdata_get_remote4", lua_fn_cdata_get_remote4 },
  {"cdata_check_remote4", lua_fn_cdata_check_remote4 },
  {"sock_send_data", lua_fn_sock_send_data },
  { "sock_tcp_connect", lua_fn_sock_tcp_connect },
  { "run_cycles", lua_fn_run_cycles },
  { "http_start_listener", lua_fn_http_start_listener },
  { "libsupp_init", lua_fn_libsupp_init },
  { "get_file_mtime", lua_fn_get_file_mtime },
  { "get_http_data", lua_fn_get_http_data },

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
