#ifndef __LIB_HASH_FUNC__
#define __LIB_HASH_FUNC__

#include <stdint.h>

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
 * -------------------------------------------------------------------------------
 *  hashlittle() -- hash a variable-length key into a 32-bit value
 *    k       : the key (the unaligned variable-length array of bytes)
 *    length  : the length of the key, counting by bytes
 *    initval : can be any 4-byte value
 *        Returns a 32-bit value.  Every bit of the key affects every bit of
 *        the return value.  Two keys differing by one or two bits will have
 *        totally different hash values.
 *
 *        The best hash table sizes are powers of 2.  There is no need to do
 *        mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 *        use a bitmask.  For example, if you need only 10 bits, do
 *        h = (h & hashmask(10));
 *        In which case, the hash table should have hashsize(10) elements.
 *
 *        If you are hashing n strings (uint8_t **)k, do it like this:
 *          for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);
 *
 *        By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
 *        code any way you wish, private, educational, or commercial.  It's free.
 *
 *        Use for hash table lookup, or anything where one collision in 2^^32 is
 *        acceptable.  Do NOT use for cryptographic purposes.
 * -------------------------------------------------------------------------------
 */

uint32_t hashlittle(const void *key, size_t length, uint32_t initval);


#endif


