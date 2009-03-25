#include "lib_debug.h"
#include "lib_dbuf.h"
#include "lib_sock.h"
#include "lib_uuid.h"
#include "fmv.h"
#include "lua.h"

#include <math.h>
#include "libsupp.h"

static void
dpack_bit(dbuf_t *dbuf, int bit)
{
  if (bit) {
    dbuf->buf[dbuf->dsize] |= (0x80 >> (dbuf->bitpos));
  } else {
    dbuf->buf[dbuf->dsize] &= (uint8_t)(0xff7f >> (dbuf->bitpos));
  }
  debug(0,2000, "bit at %d bitpos at %d set to %d, result byte: %x", 
	dbuf->bitpos, dbuf->dsize, bit, dbuf->buf[dbuf->dsize]);
  if(++dbuf->bitpos > 7) {
    dbuf->bitpos = 0;
    dbuf->dsize++;
    if(dbuf->dsize >= dbuf->size) {
      dgrow(dbuf, 100);
    }
    dbuf->buf[dbuf->dsize] = 0;
  }
}

static void dpack_Nbit(dbuf_t *dbuf, uint32_t val, int nbit) {
  int i;
  //debug(DBG_HA_TERRAIN,1, "packing %x into %d bit", val, nbit);
  while (nbit > 8) {
    dpack_Nbit(dbuf, val & 0xff, 8);
    val = val >> 8;
    nbit -= 8;
  }
  for(i=1; i<=nbit; i++) {
    dpack_bit(dbuf, val & ((uint32_t)1 << (nbit-i)));
  }
}

static void dpack_8bit(dbuf_t *dbuf, uint8_t byte) {
  debug(DBG_HA_TERRAIN,1, "packing %x", byte);
  dpack_Nbit(dbuf, byte, 8);
}


void
static dpack_bit_array(dbuf_t *dbuf, uint8_t *arr, int len) {
  int i;
  debug(DBG_HA_TERRAIN,1, "packing into %d byte into bit array", len);
  for(i=0; i<len; i++) {
    dpack_8bit(dbuf, arr[i]);
  }
}


float OO_SQRT2 = M_SQRT1_2; //0.7071067811865475244008443621049f;
//M_PI
//M_SQRT1_2
int tables_init = 0;

float CosineTable16[16 * 16];
float QuantizeTable16[16 * 16];
int CopyMatrix16[16 * 16];

static void check_tables()
{
  int u, n, i, j, diag, right, count;
  if (tables_init) { return; }
  float hposz = (float)M_PI * 0.5f / 16.0f;

  // Init the cosine table
  for (u = 0; u < 16; u++) {
    for (n = 0; n < 16; n++) {
      CosineTable16[u * 16 + n] = (float)cos((2.0 * (float)n + 1.0) * (float)u * hposz);
    }
  }

  // Quantize table
  for (u = 0; u < 16; u++) {
    for (n = 0; n < 16; n++) {
      QuantizeTable16[u * 16 + n] = 1.0f / (1.0f + 2.0f * ((float)n + (float)u));
    }
  }

  // Build copy matrix
  j = i = 0;
  diag = 0;
  right = 1;
  count = 0;
  while (i < 16 && j < 16) {
    CopyMatrix16[j * 16 + i] = count++;
    if (!diag) {
      if(right) {
        if (i < 16 - 1) i++; else j++;
        right = 0; diag = 1;
      } else {
        if (j < 16 - 1) j++; else i++;
        right = 1; diag = 1;
      }

    } else {
      if(right) {
        i++; j--; if (i == 16 - 1 || j == 0) diag = 0;
      } else {
        i--; j++; if (j == 16 - 1 || i == 0) diag = 0;
      }
    }

  }
  
  tables_init = 1;
}

void dct_line16(float *linein, float *lineout, int line)
{
  float total = 0.0;
  int line_size = line * 16;
  int n,u;

  for(n = 0; n < 16; n++)  total += linein[line_size + n];
  lineout[line_size] = OO_SQRT2 * total;
  for(u = 1; u < 16; u++) {
    total = 0.0;
    for (n = 0; n < 16; n++) total += linein[line_size + n] * CosineTable16[u * 16 + n];
    lineout[line_size + u] = total;
  }
}

void dct_column16(float *linein, int *lineout, int column)
{
  float total = 0.0;
  float oosob = 2.0 / 16.0;
  int n,u;
  
  for(n = 0; n < 16; n++) total += linein[16 * n + column];
  lineout[CopyMatrix16[column]] = (int)(OO_SQRT2 * total * oosob * QuantizeTable16[column]);
  for (u = 1; u < 16; u++) {
    total = 0.0;
    for (n = 0; n < 16; n++) total += linein[16 * n + column] * CosineTable16[u * 16 + n];
    lineout[CopyMatrix16[16 * u + column]] = (int)(total * oosob * QuantizeTable16[16 * u + column]);
  }
}

void CreatePatch(dbuf_t *dbuf, float *heightmap, int patch_x, int patch_y)
{
  float zmin = -99999999.0;
  float zmax = 99999999.0;
  float h;

  float hdr_dcoffset;
  uint16_t hdr_range;
  uint8_t hdr_quantwbits = 0x88;
  uint16_t hdr_patch_ids = ((patch_x & 0xF)<<5) + (patch_y & 0xf);
  // int prequant = 10;
  float norm_block[16*16];
  float ftemp[16*16];
  int itemp[16*16];
  int last_nonzero_itemp = -1;
  int wbits = 0;

  int ix, iy;
  int k = 0;

  debug(DBG_HA_TERRAIN,1, "patch - prescan start...");
  check_tables();

  /*
   * prescan - get the min and range 
   */
  zmin = zmax = heightmap[0];
  for(iy = patch_y * 16; iy < (patch_y + 1) * 16; iy++) {
    for(ix = patch_x * 16; ix < (patch_x + 1) * 16; ix++) {
      h = heightmap[ix + iy*256];
      zmin = h < zmin ? h : zmin;
      zmax = h > zmax ? h : zmax;
    }
  }
  hdr_dcoffset = zmin;
  hdr_range = (uint16_t)(zmax - zmin + 1.0);

  debug(DBG_HA_TERRAIN,1, "patch - compress start...");

  /* Compress patch - CompressPatch */
  {
    float oozrange = 1.0 / (float)hdr_range;
    float range = 1024.0; 
    float premult = oozrange * range;
    float sub = 512.0 + hdr_dcoffset * premult;
    
    k = 0;
    for(iy = patch_y * 16; iy < (patch_y + 1) * 16; iy++) {
      for(ix = patch_x * 16; ix < (patch_x + 1) * 16; ix++) {
        norm_block[k++] = heightmap[iy * 256 + ix] * premult - sub;
      }
    }
  }

  // TODO: DCT!
  memset(ftemp, 0, sizeof(ftemp));
  memset(itemp, 0, sizeof(itemp));
  for(k = 0; k < 16; k++) { dct_line16(norm_block, ftemp, k); }
  for(k = 0; k < 16; k++) { dct_column16(ftemp, itemp, k); }

  debug(DBG_HA_TERRAIN,1, "patch - encode header start...");
  /* Encode Patch header EncodePatchHeader - wbits */

  {
    int max = 0;
    
    for(k=0; k < 16*16; k++) {
      max = abs(itemp[k]) > max ? abs(itemp[k]) : max;
      if(itemp[k] != 0) { last_nonzero_itemp = k; }
    }
    while (max > 0 && wbits < 16) {
      max = max >> 1;
      wbits++;
    }
    if (wbits < 9) {
      wbits = 9;
    }
    wbits++; // sign
    if (wbits > 17) {
      // error!
      wbits = 15;
    }
    
    hdr_quantwbits = (hdr_quantwbits & 0xF0) + (wbits - 2); 
    dpack_8bit(dbuf, hdr_quantwbits);
    debug(DBG_HA_TERRAIN,1, "patch - encode hdr_dcoffset of %f...", hdr_dcoffset);
    dpack_bit_array(dbuf, (uint8_t *)&hdr_dcoffset, sizeof(float));   
    debug(DBG_HA_TERRAIN,1, "patch - encode hdr_range of %x...", hdr_range);
    dpack_bit_array(dbuf, (uint8_t *)&hdr_range, 2);
    debug(DBG_HA_TERRAIN,1, "patch - encode hdr_patch_ids of %x...", hdr_patch_ids);
    dpack_Nbit(dbuf, hdr_patch_ids, 10);
  }  

  debug(DBG_HA_TERRAIN,1, "patch - encode start...");

  /* Encode Patch */
  {
    for(k=0; k <= last_nonzero_itemp; k++) {
      debug(DBG_HA_TERRAIN,1, "patch - encode k=%x, val: %x, %d bits...", k, itemp[k], wbits);
      if (itemp[k] == 0) {
        dpack_bit(dbuf, 0);
      } else if (itemp[k] > 0) {
        // positive value
        dpack_Nbit(dbuf, 0x6, 3);
        if (itemp[k] > (1 << wbits)) { itemp[k] = (1 << wbits); }
        dpack_Nbit(dbuf, itemp[k], wbits);
      } else {
        itemp[k] = 0-itemp[k];
        // negative value
        dpack_Nbit(dbuf, 0x7, 3);
        if (itemp[k] > (1 << wbits)) { itemp[k] = (1 << wbits); }
        dpack_Nbit(dbuf, itemp[k], wbits);
      }
    }
    if (last_nonzero_itemp < 16*16-1) {
      dpack_Nbit(dbuf, 0x2, 2);
    }
  }
}





enum { LayerLand = 0x4C };

float my_height_map[256*256];

dbuf_t *MakeLayerPatches(uint16_t *patch_set, uint16_t *patch_set_remaining)
{
  //-- fixme defvar_slpacket_new(pkt, PackLayerData);
  int px, py;
  int full = 0;
  int i;
  dbuf_t *pkt = PacketNew();



  // STRIDE = 264 (2 bytes)
  // patchsize = 8
  // type = 0x4C
  // ... patches ...
  // END_OF_PATCHES=97

  dbuf_t *bytes = dalloc(1536);
  if (patch_set_remaining) {
    for(i=0; i<16;i++) {
      patch_set_remaining[i] = patch_set[i];
    }
  }

  dpack_Nbit(bytes, 8, 8); // LSB stride
  dpack_Nbit(bytes, 1, 8); // MSB of stride
  dpack_Nbit(bytes, 16, 8); // patch size
  dpack_Nbit(bytes, LayerLand, 8); // layer type 

  for(px=0;px<16 && !full;px++) {
    for(py=0;py<16 && !full;py++) {
      if (patch_set_remaining[py] & (1 << px)) {
        CreatePatch(bytes, my_height_map, px, py);
        if (bytes->dsize > 600) {
          full = 1;
        } 
        if(patch_set_remaining) {
          patch_set_remaining[py] &= (0x7FFF >> (15-px)) | (0xFFFE << px);
        }
      }
    }
  }

  // End of patches
  dpack_Nbit(bytes, 97, 8);
  dpack_Nbit(bytes, 0, 8);


  LayerDataHeader(pkt);
  LayerData_LayerID(pkt, LayerLand);
  LayerData_LayerData(pkt, bytes->buf, bytes->dsize);

  dunlock(bytes);

  debug(DBG_HA_TERRAIN,1, "About to send the layer data...");
  // FIXME: send !!! osi_send_immed_free(sess, pkt)
  return pkt;
}


/* Function is called with the single parameter - the session table for 
the session that needs to receive the layer data */
int
lua_fn_SendLayerData(lua_State *L)
{
  uint16_t patches[16];
  int i;
  int done = 0;
  dbuf_t *pkt;

  int x, y;

  for(x=0; x<256; x++) {
    for(y=0; y<256; y++) {
      my_height_map[y*256 + x] = 30 * cos(((float)x - 128.0)/64.0) * cos(((float)y - 128.0)/64.0);
      // Flat terrain
      my_height_map[y*256 + x] = 0.3;
    }
  }
  


  for(i=0;i<=15;i++) {
    patches[i] = 0xFFFF;
  }
  while(!done) {
    pkt = MakeLayerPatches(patches, patches); 
    lua_getglobal(L, "smv_send_then_unlock");
    lua_pushvalue(L, -2); // the sess entry
    lua_pushlightuserdata(L, pkt);
    lua_pcall_with_debug(L, 2, 0, DBG_GLOBAL, 0);

    done = 1;
    for(i=0;i<=15;i++) {
      if(patches[i]) done = 0;
    }
  }
  return 0;
}


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
  { "SendLayerData", lua_fn_SendLayerData },
  { NULL, NULL }
};

LUA_API int luaopen_libpktsmv (lua_State *L) {
  printf("Address of event handler: %x\n", (unsigned int)smv_packet);
  luaL_openlib(L, "smv", smvlib, 0);
  return 1;
}

