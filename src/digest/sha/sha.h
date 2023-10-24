#ifndef DIGEST_SHA_H_
#define DIGEST_SHA_H_
#include "digest.h"
#include "common/type.h"

// reference openssl
namespace digest {

using namespace common;

#define SHA_LBLOCK 16
#define SHA_CBLOCK (SHA_LBLOCK * 4)
#define SHA_LAST_BLOCK (SHA_CBLOCK - 8)
#define SHA_DIGEST_LENGTH 20

#define X(i) XX##i

/* default hash value */
#define INIT_DATA_h0 0x67452301UL
#define INIT_DATA_h1 0xefcdab89UL
#define INIT_DATA_h2 0x98badcfeUL
#define INIT_DATA_h3 0x10325476UL
#define INIT_DATA_h4 0xc3d2e1f0UL

/* const hash value for step */
#define K_00_19 0x5a827999UL  // step£º00~19
#define K_20_39 0x6ed9eba1UL  // step£º20~39
#define K_40_59 0x8f1bbcdcUL  // step£º40~59
#define K_60_79 0xca62c1d6UL  // step£º60~79

/* hash function for step */
#define F_00_19(b, c, d) ((((c) ^ (d)) & (b)) ^ (d))          // step£º00~19
#define F_20_39(b, c, d) ((b) ^ (c) ^ (d))                    // step£º20~39
#define F_40_59(b, c, d) (((b) & (c)) | (((b) | (c)) & (d)))  // step£º40~59
#define F_60_79(b, c, d) F_20_39(b, c, d)                     // step£º60~79

/* swap left and right by n */
#define ROTATE(a, n) (((a) << (n)) | (((a)&0xffffffff) >> (32 - (n))))

/* swap little-endian to big-endian, assign to l */
/* address increase ub32 size  */
#define HOST_c2l(c, l)                                                \
  (l = (((ulong)(*((c)++))) << 24), l |= (((ulong)(*((c)++))) << 16), \
   l |= (((ulong)(*((c)++))) << 8), l |= (((ulong)(*((c)++)))))

/* swap little-endian to big-endian, assign to c */
/* address increase ub32 size  */
#define HOST_l2c(l, c)                                                         \
  (*((c)++) = (ub8)(((l) >> 24) & 0xff), *((c)++) = (ub8)(((l) >> 16) & 0xff), \
   *((c)++) = (ub8)(((l) >> 8) & 0xff), *((c)++) = (ub8)(((l)) & 0xff), l)

#define HASH_MAKE_STRING(c, s) \
  do {                         \
    ulong ll;                  \
    ll = (c)->h0;              \
    (void)HOST_l2c(ll, (s));   \
    ll = (c)->h1;              \
    (void)HOST_l2c(ll, (s));   \
    ll = (c)->h2;              \
    (void)HOST_l2c(ll, (s));   \
    ll = (c)->h3;              \
    (void)HOST_l2c(ll, (s));   \
    ll = (c)->h4;              \
    (void)HOST_l2c(ll, (s));   \
  } while (0)

/*
 * SHA1£º
 * W(t) = S^1(W(t-3) XOR W(t-8) XOR W(t-14) XOR W(t-16)).
 */
#define Xupdate(a, ix, ia, ib, ic, id) \
  ((a) = (ia ^ ib ^ ic ^ id), ix = (a) = ROTATE((a), 1))

#define BODY_00_15(i, a, b, c, d, e, f, xi)                           \
  (f) = ROTATE((a), 5) + F_00_19((b), (c), (d)) + (e) + xi + K_00_19; \
  (b) = ROTATE((b), 30);

#define BODY_16_19(i, a, b, c, d, e, f, xi, xa, xb, xc, xd)       \
  Xupdate(f, xi, xa, xb, xc, xd);                                 \
  (f) += (e) + K_00_19 + ROTATE((a), 5) + F_00_19((b), (c), (d)); \
  (b) = ROTATE((b), 30);

#define BODY_20_31(i, a, b, c, d, e, f, xi, xa, xb, xc, xd)       \
  Xupdate(f, xi, xa, xb, xc, xd);                                 \
  (f) += (e) + K_20_39 + ROTATE((a), 5) + F_20_39((b), (c), (d)); \
  (b) = ROTATE((b), 30);

#define BODY_32_39(i, a, b, c, d, e, f, xa, xb, xc, xd)           \
  Xupdate(f, xa, xa, xb, xc, xd);                                 \
  (f) += (e) + K_20_39 + ROTATE((a), 5) + F_20_39((b), (c), (d)); \
  (b) = ROTATE((b), 30);

#define BODY_40_59(i, a, b, c, d, e, f, xa, xb, xc, xd)           \
  Xupdate(f, xa, xa, xb, xc, xd);                                 \
  (f) += (e) + K_40_59 + ROTATE((a), 5) + F_40_59((b), (c), (d)); \
  (b) = ROTATE((b), 30);

#define BODY_60_79(i, a, b, c, d, e, f, xa, xb, xc, xd)               \
  Xupdate(f, xa, xa, xb, xc, xd);                                     \
  (f) = xa + (e) + K_60_79 + ROTATE((a), 5) + F_60_79((b), (c), (d)); \
  (b) = ROTATE((b), 30);

struct SHA_CTX {
  // default hash value, and storage the final hashed value
  ub32 h0, h1, h2, h3, h4;
  // num_lower storage the lower 32 bits of data's length, by bits.
  // num_higher storage the higher 32 bits of data's length, by bits.
  ub32 num_lower, num_higher;
  // buff storage origin data, complement data, length aligned data
  ub32 data[SHA_LBLOCK];
  // record the length been used in buff
  ub32 num_used;
};

class Sha1Context : public IDigestInterface {
 public:
  virtual ub32 Context_Init();
  virtual ub32 Context_Update(const ub8 *data, const b64 len);
  virtual ub32 Context_Final(ub8 *md);
  // when sha only once, use this
  virtual ub8 *Digest(const ub8 *data, const b64 len, ub8 *md);

 private:
  void SHA1_Block_Data_Order(const void *p, b64 num_block);

 private:
  SHA_CTX ctx_;
};
}  // namespace digest
#endif