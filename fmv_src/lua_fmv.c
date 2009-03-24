#include "sta_fmv.h"
#include "lua_fmv.h"
#include "lib_dbuf.h"

static int uuid_are_strings = 1;

void
luaL_checkx_uuid(lua_State *L, int narg, uuid_t *uuid) {
  size_t sz;
  const char *s = luaL_checklstring(L, narg, &sz);
  if (uuid_are_strings) {
    UUIDFromString(s, uuid);
  } else {
    if(sz != sizeof(uuid_t)) {
      luaL_error(L, "Expected UUID (%d bytes string) as arg %d, got %d bytes", sizeof(uuid_t), narg, sz);
    }
    memcpy(uuid, s, sz);
  }
}

void
lua_pushx_uuid(lua_State *L, uuid_t *uuid) {
  if(uuid_are_strings) {
    char str[40];
    UUIDToString(str, uuid);
    lua_pushstring(L, (const char *) str);
  } else {
    lua_pushlstring(L, (const char *) uuid, sizeof(uuid_t));
  }
}

u16t
luaL_checkx_ipport(lua_State *L, int narg) {
  int port = luaL_checkint(L, narg);
  if ((port < 0) || (port > 65535)) {
    luaL_error(L, "Expected TCP/IP Port (0..65535) as arg %d, got %d", narg, port);
  }
  return port;
}

int
luaL_checkx_bool(lua_State *L, int narg) {
  return lua_toboolean(L, narg);
}

u64t
luaL_checkx_u64(lua_State *L, int narg) {
  size_t sz;
  const char *s = luaL_checklstring(L, narg, &sz);
  u64t u64;
  if(sz != sizeof(u64t)) {
    luaL_error(L, "Expected u64t (%d bytes string) as arg %d, got %d bytes", sizeof(u64t), narg, sz);
  }
  memcpy(&u64, s, sz);
  return u64;
}

void
lua_pushx_u64(lua_State *L, u64t u64) {
  lua_pushlstring(L, (void *)&u64, sizeof(u64t));
}

/*
void
lua_pushx_ipaddr(lua_State *L, u32t ipaddr) {
  struct in_addr in_addr;
  in_addr.s_addr = ipaddr;
  lua_pushstring(L, inet_ntoa(in_addr));
}

u32t
luaL_checkx_ipaddr(lua_State *L, int narg) {
  struct in_addr in_addr;
  const char *s = luaL_checkstring(L, narg);
  if(0 == inet_aton(s, &in_addr)) {
    luaL_error(L, "Expected IP Address as arg %d, got: '%s'", narg, s);
  }
  return in_addr.s_addr;
}
*/



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
  dbuf_t *d = PacketNew(1500);
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


static int 
lua_fn_MaybeZeroEncodePacket(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  dbuf_t *d1 = MaybeZeroEncodePacket(d);
  if(d1 == NULL) {
    return 0;
  }
  lua_pushlightuserdata(L, d1);
  return 1;
}

static int 
lua_fn_MaybeZeroDecodePacket(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  dbuf_t *d1 = MaybeZeroDecodePacket(d);
  if(d1 == NULL) {
    return 0;
  }
  lua_pushlightuserdata(L, d1);
  return 1;
}


static int 
lua_fn_GetRegionHandle(lua_State *L) {
  u32t x = luaL_checkint(L, 1);
  u32t y = luaL_checkint(L, 1);
  u64t result = (((u64t)y) << 40) + (((u64t)x << 8));
  lua_pushx_u64(L, result);
  return 1;
}

static int 
lua_fn_IsReliable(lua_State *L) {
  dbuf_t *d = luaL_checkuserdata(L, 1);
  lua_pushboolean(L, IsReliable(d->buf));
  return 1;
}

static int 
lua_fn_F32_UDP(lua_State *L) {
  float f = luaL_checknumber(L, 1);
  int offs = 0;
  u8t bytes[4];
  F32_UDP(f, bytes, &offs);
  lua_pushlstring(L, bytes, 4);
  return 1;
}

static int
lua_fn_uuid_create(lua_State *L) {
  uuid_t uuid;
  uuid_create(&uuid);
  lua_pushx_uuid(L, &uuid);
  return 1;
}

static int
lua_fn_uuid_from_bytes(lua_State *L) {
  size_t sz;
  const char *s = luaL_checklstring(L, 1, &sz);
  int offs = 0;
  uuid_t uuid;
  if(sz < sizeof(uuid_t)) {
    luaL_error(L, "Expected UUID (%d bytes string) as arg %d, got %d bytes", sizeof(uuid_t), 1, sz);
  }
  UDP_LLUUID(&uuid, (void *)s, &offs);
  lua_pushx_uuid(L, &uuid);
  return 1;
}



const luaL_reg fmv_sta_lib[] = {
  { "packet_new", lua_fn_packet_new },
  { "packet_lock", lua_fn_packet_lock },
  { "packet_unlock", lua_fn_packet_unlock },
  { "GetSequenceNumber", lua_fn_GetSequenceNumber },
  { "SetSequenceNumber", lua_fn_SetSequenceNumber },
  { "MaybeZeroEncodePacket", lua_fn_MaybeZeroEncodePacket },
  { "MaybeZeroDecodePacket", lua_fn_MaybeZeroDecodePacket },
  { "GetRegionHandle", lua_fn_GetRegionHandle },
  { "IsReliable", lua_fn_IsReliable },
  { "F32_UDP", lua_fn_F32_UDP }, 
  { "uuid_create", lua_fn_uuid_create },
  { "uuid_from_bytes", lua_fn_uuid_from_bytes },
  { NULL, NULL }
};

