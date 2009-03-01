/*
 * The file tweaked from its original looks by Andrew Yourtchenko 2009 
 */

/*
 * Copyright (c) 2008, 3Di Inc (www.3di.jp)
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * Neither the name of 3Di  nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if defined __cplusplus
        extern "C" {
#endif
#include <stdint.h>
#include "lib_uuid.h"

typedef int8_t s8t;
typedef uint8_t u8t;
typedef int16_t s16t;
typedef uint16_t u16t;
typedef int32_t s32t;
typedef uint32_t u32t;
typedef uint64_t u64t;
typedef float f32t;
typedef double f64t;

enum {
  APPENDED_ACKS  = 0x10,
  RESENT = 0x20,
  RELIABLE  = 0x40,
  ZEROCODED  = 0x80,
  Low = 1,
  Medium = 2,
  High  = 3
};


void GetAcks(int* count, u32t* acks, int maxAcks, u8t* udpMessage, int udpMessageLength);
int AppendAcks(u32t* acks, int numAcks, u8t* udpMessage, int udpMessageLength);

int IsReliable(u8t* data);
void SetReliable(u8t* data, int val);
int HasAcks(u8t* data);
void SetHasAcks(u8t* data, int val);

int IsZeroCoded(u8t* data);
void SetZeroCoded(u8t* data, int val);
u32t GetSequenceNumber(u8t* data);
void SetSequenceNumber(u8t* data, u32t seqNumber);
u16t GetPacketFrequency(u8t* data);
u16t GetPacketID(u8t* data);
u32t get_packet_global_id(u8t *data);
void SetPacketID(u8t* data, int frequency, u16t val);

void Header_UDP(u8t* data, u16t packetId, int frequency, u8t flags);
void LLUUID_UDP(uuid_t uu, u8t* data, int *i);
void UDP_LLUUID(uuid_t *uu, u8t* data, int *i);
void Variable2_UDP(const char *val, int length, u8t* data, int *i);
int UDP_Variable2(char *val, int maxlen, u8t* data, int *i);
void Variable1_UDP(const char *val, int length, u8t* data, int *i);
int UDP_Variable1(char *val, int maxlen, u8t* data, int *i);
char *UUIDFromU64(u64t value);
void LLQuaternion_UDP(f32t x, f32t y, f32t z, f32t w, u8t* data, int *i);
void UDP_LLQuaternion(f32t* x, f32t* y, f32t* z, f32t* w, u8t* data, int *i);
void LLVector3_UDP(f32t x, f32t y, f32t z, u8t* data, int *i);
void UDP_LLVector3(f32t* x, f32t* y, f32t* z, u8t* data, int *i);
void LLVector4_UDP(f32t x, f32t y, f32t z, f32t s, u8t* data, int *i);
void UDP_LLVector4(f32t* x, f32t* y, f32t* z, f32t* s, u8t* data, int *i);
void LLVector3d_UDP(f64t x, f64t y, f64t z, u8t* data, int *i);
void UDP_LLVector3d(f64t* x, f64t* y, f64t* z, u8t* data, int *i);
void Bool_UDP(int val, u8t* data, int *i);
void UDP_Bool(int* val, u8t* data, int *i);
void Fixed_UDP(const u8t *val, int size, u8t* data, int *i);
void UDP_Fixed(u8t *val, int size, u8t* data, int *i);
void S16_UDP(s16t val, u8t* data, int *i);
void UDP_S16(s16t* val, u8t* data, int *i);
void U64_UDP(u64t val, u8t* data, int *i);
void UDP_U64(u64t* val, u8t* data, int *i);
void F32_UDP(f32t val, u8t* data, int *i);
void UDP_F32(f32t* val, u8t* data, int *i);
void F64_UDP(f64t val, u8t* data, int *i);
void UDP_F64(f64t* val, u8t* data, int *i);
void U32_UDP(u32t val, u8t* data, int *i);
void UDP_U32(u32t *val, u8t* data, int *i);
void S32_UDP(s32t val, u8t* data, int *i);
void UDP_S32(s32t* val, u8t* data, int *i);
void U16_UDP(u16t val, u8t* data, int *i);
void UDP_U16(u16t* val, u8t* data, int *i);
void IPPORT_UDP(u16t val, u8t* data, int *i);
void UDP_IPPORT(u16t* val, u8t* data, int *i);
void IPADDR_UDP(u32t val, u8t* data, int *i);
void UDP_IPADDR(u32t *val, u8t* data, int *i);
void U8_UDP(u8t val, u8t* data, int *i);
void UDP_U8(u8t* val, u8t* data, int *i);
void S8_UDP(s8t val, u8t* data, int *i);
void UDP_S8(s8t* val, u8t* data, int *i);
void BOOL_UDP(int val, u8t* data, int *i);
void UDP_BOOL(int* val, u8t* data, int *i);

#if defined __cplusplus
    }
#endif
