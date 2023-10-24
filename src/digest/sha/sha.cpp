#include "sha.h"

#include <string.h>
namespace digest {
ub32 Sha1Context::Context_Init() {
  memset(&ctx_, 0x00, sizeof(ctx_));
  ctx_.h0 = INIT_DATA_h0;
  ctx_.h1 = INIT_DATA_h1;
  ctx_.h2 = INIT_DATA_h2;
  ctx_.h3 = INIT_DATA_h3;
  ctx_.h4 = INIT_DATA_h4;
  return 0;
}

ub32 Sha1Context::Context_Update(const ub8 *data, const b64 len) {
  ub32 lower = 0;
  ub32 offset = 0;
  ub32 num_block = 0;
  b64 length = 0;
  ub8 *p = nullptr;
  if (len == 0) {
    return 1;
  }

  // convert length by ub8 to length by bit
  lower = (ctx_.num_lower + (((ub32)len) << 3)) & 0xffffffffUL;
  // when happen overflow
  if (lower < ctx_.num_lower) {
    ++ctx_.num_higher;
  }

  // update length of data
  ctx_.num_higher += (ub32)(len >> 29);
  ctx_.num_lower = lower;

  offset = ctx_.num_used;
  length = len;
  // when last time's data remains
  if (0 < offset) {
    p = (ub8 *)ctx_.data;
    // complement buff by move data
    if (length >= SHA_CBLOCK || length + offset >= SHA_CBLOCK) {
      memcpy(p + offset, data, SHA_CBLOCK - offset);
      SHA1_Block_Data_Order(p, 1);
      offset = SHA_CBLOCK - offset;
      data += offset;
      length -= offset;
      ctx_.num_used = 0;
      memset(p, 0, SHA_CBLOCK);
    } else {
      memcpy(p + offset, data, length);
      ctx_.num_used += (ub32)length;
      return 1;
    }
  }

  num_block = length / SHA_CBLOCK;
  // when length bigger than SHA_CBLOCK
  if (num_block > 0) {
    SHA1_Block_Data_Order(data, num_block);
    offset = num_block * SHA_CBLOCK;
    data += offset;
    length -= offset;
  }

  // when remains length smaller than SHA_CBLOCK
  if (length != 0) {
    p = (ub8 *)ctx_.data;
    memcpy(p, data, length);
    ctx_.num_used = (ub32)length;
  }
  return 1;
}

ub32 Sha1Context::Context_Final(ub8 *md) {
  ub8 *p = (ub8 *)ctx_.data;
  size_t n = ctx_.num_used;

  /* complement 1000 0000 */
  p[n] = 0x80;
  n++;

  /* if n > 56byte(448bit) */
  if (n > (SHA_CBLOCK - 8)) {
    memset(p + n, 0, SHA_CBLOCK - n);
    n = 0;
    SHA1_Block_Data_Order(p, 1);
  }
  /* complement 0000 0000 .... .... */
  memset(p + n, 0, SHA_CBLOCK - 8 - n);

  /* append data size to end */
  p += SHA_CBLOCK - 8;
  (void)HOST_l2c(ctx_.num_higher, p);
  (void)HOST_l2c(ctx_.num_lower, p);

  /* the end digest */
  p -= SHA_CBLOCK;
  SHA1_Block_Data_Order(p, 1);

  /* clean */
  ctx_.num_used = 0;
  memset(p, 0x00, SHA_CBLOCK);

  /* genarate digest messgae */
  HASH_MAKE_STRING(&ctx_, md);

  return 1;
}

ub8 *Sha1Context::Digest(const ub8 *data, const b64 len, ub8 *md) {
  static ub8 m[SHA_DIGEST_LENGTH];
  if (md == nullptr) {
    md = m;
  }
  Context_Init();
  Context_Update(data, len);
  Context_Final(md);
  return md;
}

void Sha1Context::SHA1_Block_Data_Order(const void *p, b64 num_block) {
  const ub8 *data = (const ub8 *)p;
  register ub32 A, B, C, D, E, T, l;
  ub32 XX0, XX1, XX2, XX3, XX4, XX5, XX6, XX7, XX8, XX9, XX10, XX11, XX12,
      XX13, XX14, XX15;

  /*
   * SHA1 Document£º
   * The words of the first 5-word buffer are labeled A,B,C,D,E.
   * The words of the second 5-word buffer are labeled H0, H1, H2, H3, H4.
   * Let A = H0, B = H1, C = H2, D = H3, E = H4.
   */
  A = ctx_.h0;
  B = ctx_.h1;
  C = ctx_.h2;
  D = ctx_.h3;
  E = ctx_.h4;

  for (;;) {
    // for check endianness
    const union {
      long one;
      char little;
    } is_endian = {1};

    /*
     * SHA1 Document£º
     * The words of the 80-word sequence are labeled W(0), W(1),..., W(79).
     * Divide M(i) into 16 words W(0), W(1), ... , W(15), where W(0) is the
     left-most word.
     * For t = 0 to 15 do:
        T = S^5(A) + F((B), (C), (D)) + (E) + W(t) + K(t);
        E = D;
        D = C;
        C = S^30(B);
        B = A;
        A = T;

     * fact do:
        T = S^5(A) + F((B), (C), (D)) + (E) + W(t) + K(t);
        B = S^30(B);
     * and change the significance when next call
    */
    if (!is_endian.little && sizeof(ub32) == 4 && ((size_t)p % 4) == 0) {
      const ub32 *W = (const ub32 *)data;

      X(0) = W[0];
      BODY_00_15(0, A, B, C, D, E, T, X(0));
      X(1) = W[1];
      BODY_00_15(1, T, A, B, C, D, E, X(1));
      X(2) = W[2];
      BODY_00_15(2, E, T, A, B, C, D, X(2));
      X(3) = W[3];
      BODY_00_15(3, D, E, T, A, B, C, X(3));
      X(4) = W[4];
      BODY_00_15(4, C, D, E, T, A, B, X(4));
      X(5) = W[5];
      BODY_00_15(5, B, C, D, E, T, A, X(5));
      X(6) = W[6];
      BODY_00_15(6, A, B, C, D, E, T, X(6));
      X(7) = W[7];
      BODY_00_15(7, T, A, B, C, D, E, X(7));
      X(8) = W[8];
      BODY_00_15(8, E, T, A, B, C, D, X(8));
      X(9) = W[9];
      BODY_00_15(9, D, E, T, A, B, C, X(9));
      X(10) = W[10];
      BODY_00_15(10, C, D, E, T, A, B, X(10));
      X(11) = W[11];
      BODY_00_15(11, B, C, D, E, T, A, X(11));
      X(12) = W[12];
      BODY_00_15(12, A, B, C, D, E, T, X(12));
      X(13) = W[13];
      BODY_00_15(13, T, A, B, C, D, E, X(13));
      X(14) = W[14];
      BODY_00_15(14, E, T, A, B, C, D, X(14));
      X(15) = W[15];
      BODY_00_15(15, D, E, T, A, B, C, X(15));

      data += SHA_CBLOCK;
    } else {
      (void)HOST_c2l(data, l);
      X(0) = l;
      (void)HOST_c2l(data, l);
      X(1) = l;
      BODY_00_15(0, A, B, C, D, E, T, X(0));
      (void)HOST_c2l(data, l);
      X(2) = l;
      BODY_00_15(1, T, A, B, C, D, E, X(1));
      (void)HOST_c2l(data, l);
      X(3) = l;
      BODY_00_15(2, E, T, A, B, C, D, X(2));
      (void)HOST_c2l(data, l);
      X(4) = l;
      BODY_00_15(3, D, E, T, A, B, C, X(3));
      (void)HOST_c2l(data, l);
      X(5) = l;
      BODY_00_15(4, C, D, E, T, A, B, X(4));
      (void)HOST_c2l(data, l);
      X(6) = l;
      BODY_00_15(5, B, C, D, E, T, A, X(5));
      (void)HOST_c2l(data, l);
      X(7) = l;
      BODY_00_15(6, A, B, C, D, E, T, X(6));
      (void)HOST_c2l(data, l);
      X(8) = l;
      BODY_00_15(7, T, A, B, C, D, E, X(7));
      (void)HOST_c2l(data, l);
      X(9) = l;
      BODY_00_15(8, E, T, A, B, C, D, X(8));
      (void)HOST_c2l(data, l);
      X(10) = l;
      BODY_00_15(9, D, E, T, A, B, C, X(9));
      (void)HOST_c2l(data, l);
      X(11) = l;
      BODY_00_15(10, C, D, E, T, A, B, X(10));
      (void)HOST_c2l(data, l);
      X(12) = l;
      BODY_00_15(11, B, C, D, E, T, A, X(11));
      (void)HOST_c2l(data, l);
      X(13) = l;
      BODY_00_15(12, A, B, C, D, E, T, X(12));
      (void)HOST_c2l(data, l);
      X(14) = l;
      BODY_00_15(13, T, A, B, C, D, E, X(13));
      (void)HOST_c2l(data, l);
      X(15) = l;
      BODY_00_15(14, E, T, A, B, C, D, X(14));
      BODY_00_15(15, D, E, T, A, B, C, X(15));
    }

    /*
     * SHA1 Document£º
     * For t = 16 to 79 let:
         W(t) = S^1(W(t-3) XOR W(t-8) XOR W(t-14) XOR W(t-16)).
         T = S^5(A) + F((B), (C), (D)) + (E) + W(t) + K(t);
         E = D;
         D = C;
         C = S^30(B);
         B = A;
         A = T;
     * description
         when step 0-15 end, register val as follows:
         { C = T; B = E; A = D; T = C; E = B; D = A; }
         so next call as follows:
         (A,C,B,D,E,...) = {C,D,E,T,A,...}
    */
    BODY_16_19(16, C, D, E, T, A, B, X(0), X(0), X(2), X(8), X(13));
    BODY_16_19(17, B, C, D, E, T, A, X(1), X(1), X(3), X(9), X(14));
    BODY_16_19(18, A, B, C, D, E, T, X(2), X(2), X(4), X(10), X(15));
    BODY_16_19(19, T, A, B, C, D, E, X(3), X(3), X(5), X(11), X(0));

    BODY_20_31(20, E, T, A, B, C, D, X(4), X(4), X(6), X(12), X(1));
    BODY_20_31(21, D, E, T, A, B, C, X(5), X(5), X(7), X(13), X(2));
    BODY_20_31(22, C, D, E, T, A, B, X(6), X(6), X(8), X(14), X(3));
    BODY_20_31(23, B, C, D, E, T, A, X(7), X(7), X(9), X(15), X(4));
    BODY_20_31(24, A, B, C, D, E, T, X(8), X(8), X(10), X(0), X(5));
    BODY_20_31(25, T, A, B, C, D, E, X(9), X(9), X(11), X(1), X(6));
    BODY_20_31(26, E, T, A, B, C, D, X(10), X(10), X(12), X(2), X(7));
    BODY_20_31(27, D, E, T, A, B, C, X(11), X(11), X(13), X(3), X(8));
    BODY_20_31(28, C, D, E, T, A, B, X(12), X(12), X(14), X(4), X(9));
    BODY_20_31(29, B, C, D, E, T, A, X(13), X(13), X(15), X(5), X(10));
    BODY_20_31(30, A, B, C, D, E, T, X(14), X(14), X(0), X(6), X(11));
    BODY_20_31(31, T, A, B, C, D, E, X(15), X(15), X(1), X(7), X(12));

    BODY_32_39(32, E, T, A, B, C, D, X(0), X(2), X(8), X(13));
    BODY_32_39(33, D, E, T, A, B, C, X(1), X(3), X(9), X(14));
    BODY_32_39(34, C, D, E, T, A, B, X(2), X(4), X(10), X(15));
    BODY_32_39(35, B, C, D, E, T, A, X(3), X(5), X(11), X(0));
    BODY_32_39(36, A, B, C, D, E, T, X(4), X(6), X(12), X(1));
    BODY_32_39(37, T, A, B, C, D, E, X(5), X(7), X(13), X(2));
    BODY_32_39(38, E, T, A, B, C, D, X(6), X(8), X(14), X(3));
    BODY_32_39(39, D, E, T, A, B, C, X(7), X(9), X(15), X(4));

    BODY_40_59(40, C, D, E, T, A, B, X(8), X(10), X(0), X(5));
    BODY_40_59(41, B, C, D, E, T, A, X(9), X(11), X(1), X(6));
    BODY_40_59(42, A, B, C, D, E, T, X(10), X(12), X(2), X(7));
    BODY_40_59(43, T, A, B, C, D, E, X(11), X(13), X(3), X(8));
    BODY_40_59(44, E, T, A, B, C, D, X(12), X(14), X(4), X(9));
    BODY_40_59(45, D, E, T, A, B, C, X(13), X(15), X(5), X(10));
    BODY_40_59(46, C, D, E, T, A, B, X(14), X(0), X(6), X(11));
    BODY_40_59(47, B, C, D, E, T, A, X(15), X(1), X(7), X(12));
    BODY_40_59(48, A, B, C, D, E, T, X(0), X(2), X(8), X(13));
    BODY_40_59(49, T, A, B, C, D, E, X(1), X(3), X(9), X(14));
    BODY_40_59(50, E, T, A, B, C, D, X(2), X(4), X(10), X(15));
    BODY_40_59(51, D, E, T, A, B, C, X(3), X(5), X(11), X(0));
    BODY_40_59(52, C, D, E, T, A, B, X(4), X(6), X(12), X(1));
    BODY_40_59(53, B, C, D, E, T, A, X(5), X(7), X(13), X(2));
    BODY_40_59(54, A, B, C, D, E, T, X(6), X(8), X(14), X(3));
    BODY_40_59(55, T, A, B, C, D, E, X(7), X(9), X(15), X(4));
    BODY_40_59(56, E, T, A, B, C, D, X(8), X(10), X(0), X(5));
    BODY_40_59(57, D, E, T, A, B, C, X(9), X(11), X(1), X(6));
    BODY_40_59(58, C, D, E, T, A, B, X(10), X(12), X(2), X(7));
    BODY_40_59(59, B, C, D, E, T, A, X(11), X(13), X(3), X(8));

    BODY_60_79(60, A, B, C, D, E, T, X(12), X(14), X(4), X(9));
    BODY_60_79(61, T, A, B, C, D, E, X(13), X(15), X(5), X(10));
    BODY_60_79(62, E, T, A, B, C, D, X(14), X(0), X(6), X(11));
    BODY_60_79(63, D, E, T, A, B, C, X(15), X(1), X(7), X(12));
    BODY_60_79(64, C, D, E, T, A, B, X(0), X(2), X(8), X(13));
    BODY_60_79(65, B, C, D, E, T, A, X(1), X(3), X(9), X(14));
    BODY_60_79(66, A, B, C, D, E, T, X(2), X(4), X(10), X(15));
    BODY_60_79(67, T, A, B, C, D, E, X(3), X(5), X(11), X(0));
    BODY_60_79(68, E, T, A, B, C, D, X(4), X(6), X(12), X(1));
    BODY_60_79(69, D, E, T, A, B, C, X(5), X(7), X(13), X(2));
    BODY_60_79(70, C, D, E, T, A, B, X(6), X(8), X(14), X(3));
    BODY_60_79(71, B, C, D, E, T, A, X(7), X(9), X(15), X(4));
    BODY_60_79(72, A, B, C, D, E, T, X(8), X(10), X(0), X(5));
    BODY_60_79(73, T, A, B, C, D, E, X(9), X(11), X(1), X(6));
    BODY_60_79(74, E, T, A, B, C, D, X(10), X(12), X(2), X(7));
    BODY_60_79(75, D, E, T, A, B, C, X(11), X(13), X(3), X(8));
    BODY_60_79(76, C, D, E, T, A, B, X(12), X(14), X(4), X(9));
    BODY_60_79(77, B, C, D, E, T, A, X(13), X(15), X(5), X(10));
    BODY_60_79(78, A, B, C, D, E, T, X(14), X(0), X(6), X(11));
    BODY_60_79(79, T, A, B, C, D, E, X(15), X(1), X(7), X(12));

    /*
     * SHA1 Document£º
     * Let H0 = H0 + A, H1 = H1 + B, H2 = H2 + C, H3 = H3 + D, H4 = H4 + E.
     */
    ctx_.h0 = (ctx_.h0 + E) & 0xffffffffL;
    ctx_.h1 = (ctx_.h1 + T) & 0xffffffffL;
    ctx_.h2 = (ctx_.h2 + A) & 0xffffffffL;
    ctx_.h3 = (ctx_.h3 + B) & 0xffffffffL;
    ctx_.h4 = (ctx_.h4 + C) & 0xffffffffL;

    if (--num_block == 0) break;

    /*
     * SHA1 Document£º
     * Let A = H0, B = H1, C = H2, D = H3, E = H4.
     */
    A = ctx_.h0;
    B = ctx_.h1;
    C = ctx_.h2;
    D = ctx_.h3;
    E = ctx_.h4;
  }
}

}  // namespace digest