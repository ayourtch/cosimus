/**************************************************************************
*
*  Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.
*
*  Permission is hereby granted, free of charge, to any person obtaining 
* a copy of this software and associated documentation files (the "Software"), 
* to deal in the Software without restriction, including without limitation 
* the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom 
* the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included 
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
* OR OTHER DEALINGS IN THE SOFTWARE. 
*
*****************************************************************************/
#include "lib_debug.h"
#include "lib_dbuf.h"
#include "lib_sock.h"
#include "lib_uuid.h"
#include "fmv.h"
#include "lua.h"

#include <math.h>
#include "libsupp.h"
#include <assert.h>

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

void smv_set_height_map_at(int x, int y, float z)
{
  if( (x >= 0) && (y >= 0) && (x <= 255) && (y <= 255) ) {
    my_height_map[y*256 + x] = z;
  } else {
    debug(0,0, "Invalid X/Y coordinates for set_height_map_at: %d, %d", x, y);
  }
}

/* 
 * Short helper function which should allow to 
 * avoid boundary checks in the getter below.
 * The edges of the heightmap are simply assumed to be stretching 
 * into infinity - for now.
 * Later if there's ever "direct neighbourships" - 
 * this could go and poke into the neighbouring heightmap as well.
 */

static float H(int x, int y)
{
  if (x < 0) { x = 0; } if (x > 255) { x = 255; }
  if (y < 0) { y = 0; } if (y > 255) { y = 255; }
  return my_height_map[y*256 + x];
}



/**
 * For now, we interpolates the Z coordinate using a simple algorithm:
 *
 *  - take the x-y square with the center in the middle, 
 *    that contains our x-y coordinates. 
 *  - put a dot in the middle of it.
 *  - use that dot as a means to split the square into 3 triangles
 *  - select the triangle that the user's supplied coordinates fall into.
 *  - return Z coordinate for the user's point on that respective plane.
 * 
 *  As we probably would need the normal vector too in the near future,
 *  split the calculation of the plane coefficient into a separate function.
 *
 **/

static void calc_abcmd(float x, float y, float *A, float *B, float *C, float *MD)
{
  float x1, x2, x3, y1, y2, y3, z1, z2, z3;
  int x0, y0;


  // will use in indices and as a base. It's truncated x,y.
  x0 = (int)(x); y0 = (int)(y); 

  // 3rd point is the average of all 4 points of the bounding x-y square.

  x3 = 0.5 + x0; 
  y3 = 0.5 + y0;
  z3 = 0.25 * ( H(x0, y0) + H(x0+1, y0) + H(x0, y0+1) + H(x0+1, y0+1) );

  /* 
   * Now we need to find which triangle we're in. Let's imagine this 
   * x/y square being a postcard, with the (x0,y0) its 
   * left-top corner. Then (x3,y3) divides it into four triangles.
   * Let's number them from 1 to 4:
   *
   * (x0,y0)  (x0+1, y0)
   *  |      /
   *  v     v
   *  +-----+---> X
   *  |\ 1 /| 
   *  | \ / |
   *  |4 + 2|
   *  | / \ |
   *  |/ 3 \|
   *  +-----+ <-- (x0+1, y0+1)
   *  |
   *  v
   *  Y
   * 
   * To figure out which of the triangles holds the (x,y) we need to find the position
   * of that dot reative to two diagonals.
   *
   * First - if x - x0> y - y0 then we're either in 1 or 2, else if 
   * x < y then we're in either 3 or 4. If we just use that to fix the x1/y1 pair to 
   * the common point, I think we will get the "flipover" effect. So we'll assign 
   * the coordinates for x1, x2 always in pairs.
   * 
   * Second bisection - if (x0+1-x > y-y0) then it is either in 1 or 4, otherwise 
   * it is in 2 or 3. 
   * 
   * The border where the strict inequality does not hold - can be calculated
   * using either of the plane equations. So we save some typing.
   *
   * Intuitively, the direction of the normal vector, should be dependant on the 
   * "rotation" of the points - so we can't "optimize" by using one condition to 
   * assign the common value. We assign the coords for points "clockwise",
   * taking into the account that (x3, y3) is already assigned.
   *
   * (x0,y0)  (x0+1, y0)
   *  |      /
   *  v     v
   *  +-----+---> X
   *  |\ 1 /| 
   *  | \ / |
   *  |4 + 2|
   *  | / \ |
   *  |/ 3 \|
   *  +-----+ <-- (x0+1, y0+1)
   *  | \
   *  v  (x0, y0+1)
   *  Y
   * 
   */

  if(x0+1-x > y-y0) { // either #1 or #4
    // and according to gnuplot I mixed 1 and 4... odd.
    if(x-x0 > y-y0) { // #1
      x1 = x0; y1 = y0; z1 = H(x0,y0);
      x2 = x0+1.0; y2 = y0; z2 = H(x0+1,y0);
    } else { // #4
      x1 = x0; y1 = y0+1; z1 = H(x0,y0+1);
      x2 = x0; y2 = y0; z2 = H(x0,y0);
    }
  } else { // either #2 or #3
    // hmm on the graph looks like I mixed 2 and 3, yet I miss it...
    if(x-x0 > y-y0) { // #2
      x1 = x0+1.0; y1 = y0; z1 = H(x0+1,y0);
      x2 = x0+1.0; y2 = y0+1.0; z2 = H(x0+1,y0+1);
    } else { // #3
      x1 = x0+1.0; y1 = y0+1.0; z1 = H(x0+1,y0+1);
      x2 = x0; y2 = y0+1.0; z2 = H(x0,y0+1);
    }
  }

  /*
   * now we have the three points. Calculate the coefficients
   * for the equation of the plane in the form:
   * "Ax + By + Cz = MD" (MD == minus D)
   */

  *A = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
  *B = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
  *C = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
  *MD = x1 * (y2*z3 - y3*z2) + x2 * (y3*z1 - y1*z3) + x3*(y1*z2 - y2*z1);
}


float smv_get_height_map_at(float x, float y)
{
  float z, A,B,C,MD;
  
  // clamp the x and y
  if (x < 0.0) { x = 0.0; } if (x > 255.0) { x = 255.0; }
  if (y < 0.0) { y = 0.0; } if (y > 255.0) { y = 255.0; }
  
  // Calculate the plane coefficients for the equation
  // Ax + By + Cz = MD
  calc_abcmd(x, y, &A, &B, &C, &MD);

  // and do the final result
  z = (MD - A*x - B*y) / C;
  return z;
}

static int
lua_fn_smv_set_height_map_at(lua_State *L)
{
  int x = luaL_checkint(L, 1);
  int y = luaL_checkint(L, 2);
  float z = luaL_checknumber(L, 3);

  if( (x >= 0) && (y >= 0) && (x <= 255) && (y <= 255) ) {
    smv_set_height_map_at(x, y, z);
  } else {
    luaL_error(L, "set_height_map_at - arguments are out of range: (%d, %d, %f)", x, y, z);
  }
  return 0;
}

static int
lua_fn_smv_get_height_map_at(lua_State *L)
{
  float x = luaL_checknumber(L, 1);
  float y = luaL_checknumber(L, 2);
  float z = smv_get_height_map_at(x, y);
  lua_pushnumber(L, z);
  return 1;
}


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
      //my_height_map[y*256 + x] = 0.3;
    }
  }
  


  for(i=0;i<=15;i++) {
    patches[i] = 0xFFFF;
  }
  while(!done) {
    pkt = MakeLayerPatches(patches, patches); 
    assert(pkt->dsize < pkt->size);
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
  { "set_height_map_at", lua_fn_smv_set_height_map_at },
  { "get_height_map_at", lua_fn_smv_get_height_map_at },
  { NULL, NULL }
};

LUA_API int luaopen_libpktsmv (lua_State *L) {
  printf("Address of event handler: %p\n", smv_packet);
  luaL_openlib(L, "smv", smvlib, 0);
  return 1;
}

