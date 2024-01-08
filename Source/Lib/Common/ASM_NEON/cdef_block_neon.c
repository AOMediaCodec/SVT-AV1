/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */
#include "EbDefinitions.h"
#include <arm_neon.h>
#include "EbCdef.h"
#include "EbBitstreamUnit.h"

typedef struct {
    int64x2_t val[2];
} v256;

SIMD_INLINE v256 v256_from_v128_s64(int64x2_t hi, int64x2_t lo) {
    v256 t = {{lo, hi}};
    return t;
}

SIMD_INLINE v256 v256_from_v128_s16(int16x8_t hi, int16x8_t lo) {
    v256 t = {{vreinterpretq_s64_s16(lo), vreinterpretq_s64_s16(hi)}};
    return t;
}

SIMD_INLINE v256 v256_from_v128_u16(uint16x8_t hi, uint16x8_t lo) {
    v256 t = {{vreinterpretq_s64_u16(lo), vreinterpretq_s64_u16(hi)}};
    return t;
}

SIMD_INLINE v256 v256_from_v128_s8(int8x16_t hi, int8x16_t lo) {
    v256 t = {{vreinterpretq_s64_s8(lo), vreinterpretq_s64_s8(hi)}};
    return t;
}

SIMD_INLINE v256 v256_from_v128_u8(uint8x16_t hi, uint8x16_t lo) {
    v256 t = {{vreinterpretq_s64_u8(lo), vreinterpretq_s64_u8(hi)}};
    return t;
}

SIMD_INLINE v256 v256_dup_s16(int16_t x) {
    int64x2_t t = vreinterpretq_s64_s16(vdupq_n_s16(x));
    return v256_from_v128_s64(t, t);
}

SIMD_INLINE v256 v256_zero(void) {
    return v256_from_v128_s64(vdupq_n_s64(0), vdupq_n_s64(0));
}

SIMD_INLINE v256 v256_max_s16(v256 a, v256 b) {
    return v256_from_v128_s16(
        vmaxq_s16(vreinterpretq_s16_s64(a.val[1]), vreinterpretq_s16_s64(b.val[1])), 
        vmaxq_s16(vreinterpretq_s16_s64(a.val[0]), vreinterpretq_s16_s64(b.val[0])));
}

SIMD_INLINE v256 v256_andn(v256 a, v256 b) {
    return v256_from_v128_s64(vbicq_s64(a.val[1], b.val[1]), vbicq_s64(a.val[0], b.val[0]));
}

SIMD_INLINE v256 v256_cmpeq_16(v256 a, v256 b) {
    return v256_from_v128_u16(
        vceqq_s16(vreinterpretq_s16_s64(b.val[1]), vreinterpretq_s16_s64(a.val[1])), 
        vceqq_s16(vreinterpretq_s16_s64(b.val[0]), vreinterpretq_s16_s64(a.val[0])));
}

SIMD_INLINE v256 v256_min_s16(v256 a, v256 b) {
    return v256_from_v128_s16(
        vminq_s16(vreinterpretq_s16_s64(a.val[1]), vreinterpretq_s16_s64(b.val[1])), 
        vminq_s16(vreinterpretq_s16_s64(a.val[0]), vreinterpretq_s16_s64(b.val[0])));
}

SIMD_INLINE v256 v256_sub_u16(v256 a, v256 b) {
    return v256_from_v128_u16(
        vsubq_u16(vreinterpretq_u16_s64(a.val[1]), vreinterpretq_u16_s64(b.val[1])), 
        vsubq_u16(vreinterpretq_u16_s64(a.val[0]), vreinterpretq_u16_s64(b.val[0])));
}

SIMD_INLINE v256 v256_add_16(v256 a, v256 b) {
    return v256_from_v128_s16(
        vaddq_s16(vreinterpretq_s16_s64(a.val[1]), vreinterpretq_s16_s64(b.val[1])), 
        vaddq_s16(vreinterpretq_s16_s64(a.val[0]), vreinterpretq_s16_s64(b.val[0])));
}

SIMD_INLINE int64x2_t maddubs_s16(int64x2_t a, int64x2_t b) {
    int16x8_t a_odd = vshrq_n_s16(vreinterpretq_s16_s64(a), 8);
    int16x8_t a_even = vbicq_s16(vreinterpretq_s16_s64(a), vdupq_n_s16(0xff00));
    int16x8_t b_even = vshrq_n_s16(vshlq_n_s16(vreinterpretq_s16_s64(b), 8), 8);
    int16x8_t b_odd = vshrq_n_s16(vreinterpretq_s16_s64(b), 8);
    int16x8_t prod1 = vmulq_s16(a_even, b_even);
    int16x8_t prod2 = vmulq_s16(a_odd, b_odd);
    return vreinterpretq_s64_s16(vqaddq_s16(prod1, prod2));
}

SIMD_INLINE v256 v256_madd_s8(v256 a, v256 b) {
    return v256_from_v128_s64(maddubs_s16(a.val[1], b.val[1]), maddubs_s16(a.val[0], b.val[0]));
}

SIMD_INLINE v256 v256_dup_s8(int8_t x) {
    int8x16_t t = vdupq_n_s8(x);
    return v256_from_v128_s8(t, t);
}

SIMD_INLINE v256 v256_cmplt_s16(v256 a, v256 b) {
    return v256_from_v128_u16(
        vcltq_s16(vreinterpretq_s16_s64(a.val[1]), vreinterpretq_s16_s64(b.val[1])), 
        vcltq_s16(vreinterpretq_s16_s64(a.val[0]), vreinterpretq_s16_s64(b.val[0])));
}

SIMD_INLINE v256 v256_pack_s16_u8(v256 a, v256 b) {
    return v256_from_v128_u8(
        vqmovun_high_s16(vqmovun_s16(vreinterpretq_s16_s64(a.val[0])), vreinterpretq_s16_s64(a.val[1])), 
        vqmovun_high_s16(vqmovun_s16(vreinterpretq_s16_s64(b.val[0])), vreinterpretq_s16_s64(b.val[1])));
}

SIMD_INLINE int64x2_t unpacklo(int64x2_t a, int64x2_t b) {
    int64x1_t a_l = vget_low_s64(a);
    int64x1_t b_l = vget_low_s64(b);
    return vcombine_s64(a_l, b_l);
}

SIMD_INLINE v256 v256_from_v64(int64x2_t a, int64x2_t b, int64x2_t c, int64x2_t d) {
    return v256_from_v128_s64(unpacklo(b, a), unpacklo(d, c));
}

#define v128_shr_n_s16(a, c) vshlq_s16(a, vdupq_n_s16((int16_t) -((c & ~15) ? 15 : c)))
#define v256_shr_n_s16(a, n) v256_from_v128_s16(v128_shr_n_s16(vreinterpretq_s16_s64(a.val[1]), n), v128_shr_n_s16(vreinterpretq_s16_s64(a.val[0]), n))

SIMD_INLINE int64x2_t constrain(v256 a, v256 b, unsigned int strength, unsigned int adjdamp) {
    const v256    diff16 = v256_sub_u16(a, b);
    int8x16_t       diff = vqmovn_high_s16(vqmovn_s16(vreinterpretq_s16_s64(diff16.val[0])),
                                           vreinterpretq_s16_s64(diff16.val[1]));
    const int8x16_t sign = vreinterpretq_s8_u8(vcltq_s8(diff, vdupq_n_s8(0)));
    diff                 = vabsq_s8(diff);
    return vreinterpretq_s64_s8(veorq_s8(vaddq_s8(sign, vreinterpretq_s8_u8(vminq_u8(vreinterpretq_u8_s8(diff), vqsubq_u8(vdupq_n_u8(strength), vshrq_n_u8(vreinterpretq_u8_s8(diff), adjdamp))))), sign));
}

SIMD_INLINE v256 v256_ssub_u16(v256 a, v256 b) {
    return v256_from_v128_u16(
        vqsubq_u16(vreinterpretq_u16_s64(a.val[1]), vreinterpretq_u16_s64(b.val[1])), 
        vqsubq_u16(vreinterpretq_u16_s64(a.val[0]), vreinterpretq_u16_s64(b.val[0])));
}

SIMD_INLINE v256 v256_abs_s16(v256 a) { 
    return v256_from_v128_s16(
        vabsq_s16(vreinterpretq_s16_s64(a.val[1])), 
        vabsq_s16(vreinterpretq_s16_s64(a.val[0]))); 
}

SIMD_INLINE int64x2_t srli_s16(int16x8_t a, const unsigned int imm8) {
    if (__builtin_expect(!!(imm8 > 15), 0)) {
        return vdupq_n_s64(0);
    }
    return vreinterpretq_s64_s16(vshrq_n_s16(a, (int8_t)imm8));
}

SIMD_INLINE v256 v256_shr_s16(v256 a, const unsigned int c) {
    return v256_from_v128_s64(
        srli_s16(vreinterpretq_s16_s64(a.val[1]), c), 
        srli_s16(vreinterpretq_s16_s64(a.val[0]), c));
}

SIMD_INLINE v256 v256_xor(v256 a, v256 b) {
    return v256_from_v128_s64(veorq_s64(a.val[1], b.val[1]), veorq_s64(a.val[0], b.val[0]));
}

SIMD_INLINE v256 v256_mullo_s16(v256 a, v256 b) {
    return v256_from_v128_s16(
        vmulq_s16(vreinterpretq_s16_s64(a.val[1]), vreinterpretq_s16_s64(b.val[1])), 
        vmulq_s16(vreinterpretq_s16_s64(a.val[0]), vreinterpretq_s16_s64(b.val[0])));
}

SIMD_INLINE v256 constrain16(v256 a, v256 b, unsigned int threshold, unsigned int adjdamp) {
    v256       diff = v256_sub_u16(a, b);
    const v256 sign = v256_shr_n_s16(diff, 15);
    diff            = v256_abs_s16(diff);
    const v256 s    = v256_ssub_u16(v256_dup_s16((int) threshold), v256_shr_s16(diff, adjdamp));
    return v256_xor(v256_add_16(sign, v256_min_s16(diff, s)), sign);
}

SIMD_INLINE int64x2_t loadl_s64(int64x2_t const* mem_addr){
    return vcombine_s64(vld1_s64((int64_t const *)(mem_addr)), vdup_n_s64(0));
}

SIMD_INLINE int8x16_t loadu_s8(void const * mem_addr) {
    return vld1q_s8((int8_t const*) (mem_addr));
}

SIMD_INLINE void storel_s64(int64x2_t* mem_addr, int64x2_t a) {
    __builtin_memcpy(mem_addr, &a, sizeof(int64_t));
}

SIMD_INLINE void storeu_s64(void* mem_addr, int64x2_t a) {
    __builtin_memcpy(mem_addr, &a, sizeof(a));
}

#define srli_s8(a, imm8) ((imm8 < 0) || (imm8 > 15)) ? vdupq_n_s8(0) : (vextq_s8(a, vdupq_n_s8(0), ((imm8 & 15) != 0) ? imm8 : (imm8 & 15)))

void svt_av1_cdef_filter_block_8xn_8_neon(uint8_t *dst, int dstride, const uint16_t *in, int pri_strength,
                                          int sec_strength, int dir, int pri_damping, int sec_damping,
                                          int coeff_shift, uint8_t height, uint8_t subsampling_factor) {
    int           i;
    int64x2_t     p0, p1, p2, p3;
    v256          sum, row, res, tap;
    v256          max, min, large = v256_dup_s16(CDEF_VERY_LARGE);
    const int32_t po1  = svt_aom_eb_cdef_directions[dir][0];
    const int32_t po2  = svt_aom_eb_cdef_directions[dir][1];
    const int32_t s1o1 = svt_aom_eb_cdef_directions[(dir + 2)][0];
    const int32_t s1o2 = svt_aom_eb_cdef_directions[(dir + 2)][1];
    const int32_t s2o1 = svt_aom_eb_cdef_directions[(dir - 2)][0];
    const int32_t s2o2 = svt_aom_eb_cdef_directions[(dir - 2)][1];

    const int *pri_taps = svt_aom_eb_cdef_pri_taps[(pri_strength >> coeff_shift) & 1];
    const int *sec_taps = svt_aom_eb_cdef_sec_taps[0];

    if (pri_strength)
        pri_damping = AOMMAX(0, pri_damping - get_msb(pri_strength));
    if (sec_strength)
        sec_damping = AOMMAX(0, sec_damping - get_msb(sec_strength));
    for (i = 0; i < height; i += (2 * subsampling_factor)) {
        sum = v256_zero();
        row = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE)));

        max = min = row;
        // Primary near taps
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + po1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + po1)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p0  = constrain(tap, row, pri_strength, pri_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - po1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - po1)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p1  = constrain(tap, row, pri_strength, pri_damping);

        // sum += pri_taps[0] * (p0 + p1)
        sum = v256_add_16(sum,
                          v256_madd_s8(v256_dup_s8(pri_taps[0]),
                                       v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)))));

        // Primary far taps
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + po2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + po2)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p0  = constrain(tap, row, pri_strength, pri_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - po2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - po2)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p1  = constrain(tap, row, pri_strength, pri_damping);

        // sum += pri_taps[1] * (p0 + p1)
        sum = v256_add_16(sum,
                          v256_madd_s8(v256_dup_s8(pri_taps[1]),
                                       v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)))));

        // Secondary near taps
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s1o1)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p0  = constrain(tap, row, sec_strength, sec_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s1o1)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p1  = constrain(tap, row, sec_strength, sec_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s2o1)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p2  = constrain(tap, row, sec_strength, sec_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s2o1)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p3  = constrain(tap, row, sec_strength, sec_damping);

        // sum += sec_taps[0] * (p0 + p1 + p2 + p3)
        p0  = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p0), vreinterpretq_s8_s64(p1)));
        p2  = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p3)));
        sum = v256_add_16(sum,
                          v256_madd_s8(v256_dup_s8(sec_taps[0]),
                                       v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)))));

        // Secondary far taps
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s1o2)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p0  = constrain(tap, row, sec_strength, sec_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s1o2)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p1  = constrain(tap, row, sec_strength, sec_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s2o2)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p2  = constrain(tap, row, sec_strength, sec_damping);
        tap = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s2o2)));
        max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
        min = v256_min_s16(min, tap);
        p3  = constrain(tap, row, sec_strength, sec_damping);

        // sum += sec_taps[1] * (p0 + p1 + p2 + p3)
        p0  = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p0), vreinterpretq_s8_s64(p1)));
        p2  = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p3)));
        sum = v256_add_16(sum,
                          v256_madd_s8(v256_dup_s8(sec_taps[1]),
                                       v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)))));

        // res = row + ((sum - (sum < 0) + 8) >> 4)
        sum = v256_add_16(sum, v256_cmplt_s16(sum, v256_zero()));
        res = v256_add_16(sum, v256_dup_s16(8));
        res = v256_shr_n_s16(res, 4);
        res = v256_add_16(row, res);
        res = v256_min_s16(v256_max_s16(res, min), max);
        res = v256_pack_s16_u8(res, res);

        p0 = res.val[0];
        storel_s64((int64x2_t *)(dst + i * dstride), vreinterpretq_s64_s8(srli_s8(vreinterpretq_s8_s64(p0), 8)));
        storel_s64((int64x2_t *)(dst + (i + subsampling_factor) * dstride), p0);
    }
}

void svt_av1_cdef_filter_block_4xn_8_neon(uint8_t *dst, int dstride, const uint16_t *in, int pri_strength,
                                          int sec_strength, int dir, int pri_damping, int sec_damping,
                                          int coeff_shift, uint8_t height, uint8_t subsampling_factor) {
    int64x2_t       p0, p1, p2, p3;
    v256          sum, row, tap, res;
    v256          max, min, large = v256_dup_s16(CDEF_VERY_LARGE);
    const int32_t po1  = svt_aom_eb_cdef_directions[dir][0];
    const int32_t po2  = svt_aom_eb_cdef_directions[dir][1];
    const int32_t s1o1 = svt_aom_eb_cdef_directions[(dir + 2)][0];
    const int32_t s1o2 = svt_aom_eb_cdef_directions[(dir + 2)][1];
    const int32_t s2o1 = svt_aom_eb_cdef_directions[(dir - 2)][0];
    const int32_t s2o2 = svt_aom_eb_cdef_directions[(dir - 2)][1];

    const int *pri_taps = svt_aom_eb_cdef_pri_taps[(pri_strength >> coeff_shift) & 1];
    const int *sec_taps = svt_aom_eb_cdef_sec_taps[(pri_strength >> coeff_shift) & 1];

    if (pri_strength)
        pri_damping = AOMMAX(0, pri_damping - get_msb(pri_strength));
    if (sec_strength)
        sec_damping = AOMMAX(0, sec_damping - get_msb(sec_strength));

    for (uint32_t i = 0; i < height; i += (4 * subsampling_factor)) {
        sum = v256_zero();
        row = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE)),
                            loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE)),
                            loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE)),
                            loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE)));
        max = min = row;

        if (pri_strength) {
            // Primary near taps
            tap = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + po1)),
                                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE + po1)),
                                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE + po1)),
                                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE + po1)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p0  = constrain(tap, row, pri_strength, pri_damping);
            tap = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - po1)),
                                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE - po1)),
                                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE - po1)),
                                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE - po1)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p1  = constrain(tap, row, pri_strength, pri_damping);

            // sum += pri_taps[0] * (p0 + p1)
            sum = v256_add_16(sum,
                              v256_madd_s8(v256_dup_s8(pri_taps[0]),
                                           v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)))));

            // Primary far taps
            tap = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + po2)),
                                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE + po2)),
                                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE + po2)),
                                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE + po2)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p0  = constrain(tap, row, pri_strength, pri_damping);
            tap = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - po2)),
                                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE - po2)),
                                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE - po2)),
                                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE - po2)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p1  = constrain(tap, row, pri_strength, pri_damping);

            // sum += pri_taps[1] * (p0 + p1)
            sum = v256_add_16(sum,
                              v256_madd_s8(v256_dup_s8(pri_taps[1]),
                                           v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p1), vreinterpretq_s8_s64(p0)))));
        }

        if (sec_strength) {
            // Secondary near taps
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o1)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE + s1o1)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE + s1o1)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE + s1o1)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p0  = constrain(tap, row, sec_strength, sec_damping);
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o1)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE - s1o1)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE - s1o1)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE - s1o1)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p1  = constrain(tap, row, sec_strength, sec_damping);
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o1)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE + s2o1)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE + s2o1)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE + s2o1)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p2  = constrain(tap, row, sec_strength, sec_damping);
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o1)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE - s2o1)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE - s2o1)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE - s2o1)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p3  = constrain(tap, row, sec_strength, sec_damping);

            // sum += sec_taps[0] * (p0 + p1 + p2 + p3)
            p0  = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p0), vreinterpretq_s8_s64(p1)));
            p2  = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p3)));
            sum = v256_add_16(sum,
                              v256_madd_s8(v256_dup_s8(sec_taps[0]),
                                           v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)))));

            // Secondary far taps
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o2)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE + s1o2)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE + s1o2)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE + s1o2)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p0  = constrain(tap, row, sec_strength, sec_damping);
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o2)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE - s1o2)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE - s1o2)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE - s1o2)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p1  = constrain(tap, row, sec_strength, sec_damping);
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o2)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE + s2o2)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE + s2o2)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE + s2o2)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p2  = constrain(tap, row, sec_strength, sec_damping);
            tap = v256_from_v64(
                loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o2)),
                loadl_s64((int64x2_t *)(in + (i + (1 * subsampling_factor)) * CDEF_BSTRIDE - s2o2)),
                loadl_s64((int64x2_t *)(in + (i + (2 * subsampling_factor)) * CDEF_BSTRIDE - s2o2)),
                loadl_s64((int64x2_t *)(in + (i + (3 * subsampling_factor)) * CDEF_BSTRIDE - s2o2)));
            max = v256_max_s16(max, v256_andn(tap, v256_cmpeq_16(tap, large)));
            min = v256_min_s16(min, tap);
            p3  = constrain(tap, row, sec_strength, sec_damping);

            // sum += sec_taps[1] * (p0 + p1 + p2 + p3)
            p0 = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p0), vreinterpretq_s8_s64(p1)));
            p2 = vreinterpretq_s64_s8(vaddq_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p3)));

            sum = v256_add_16(sum,
                              v256_madd_s8(v256_dup_s8(sec_taps[1]),
                                           v256_from_v128_s8(vzip2q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)), vzip1q_s8(vreinterpretq_s8_s64(p2), vreinterpretq_s8_s64(p0)))));
        }

        // res = row + ((sum - (sum < 0) + 8) >> 4)
        sum = v256_add_16(sum, v256_cmplt_s16(sum, v256_zero()));
        res = v256_add_16(sum, v256_dup_s16(8));
        res = v256_shr_n_s16(res, 4);
        res = v256_add_16(row, res);
        res = v256_min_s16(v256_max_s16(res, min), max);
        res = v256_pack_s16_u8(res, res);

        p0                                                              = res.val[0];
        *((uint32_t *)(dst + i * dstride))                              = vgetq_lane_s32(vreinterpretq_s32_s8(srli_s8(vreinterpretq_s8_s64(p0), 12)), 0);
        *((uint32_t *)(dst + (i + (1 * subsampling_factor)) * dstride)) = vgetq_lane_s32(vreinterpretq_s32_s8(srli_s8(vreinterpretq_s8_s64(p0), 8)), 0);
        *((uint32_t *)(dst + (i + (2 * subsampling_factor)) * dstride)) = vgetq_lane_s32(vreinterpretq_s32_s8(srli_s8(vreinterpretq_s8_s64(p0), 4)), 0);
        *((uint32_t *)(dst + (i + (3 * subsampling_factor)) * dstride)) = vgetq_lane_s32(vreinterpretq_s32_s64(p0), 0);
    }
}

void svt_av1_cdef_filter_block_8xn_16_neon(uint16_t *dst, int dstride, const uint16_t *in, int pri_strength,
                                           int sec_strength, int dir, int pri_damping, int sec_damping,
                                           int coeff_shift, uint8_t height, uint8_t subsampling_factor) {
    int           i;
    v256          sum, p0, p1, p2, p3, row, res;
    v256          max, min, large = v256_dup_s16(CDEF_VERY_LARGE);
    const int32_t po1  = svt_aom_eb_cdef_directions[dir][0];
    const int32_t po2  = svt_aom_eb_cdef_directions[dir][1];
    const int32_t s1o1 = svt_aom_eb_cdef_directions[(dir + 2)][0];
    const int32_t s1o2 = svt_aom_eb_cdef_directions[(dir + 2)][1];
    const int32_t s2o1 = svt_aom_eb_cdef_directions[(dir - 2)][0];
    const int32_t s2o2 = svt_aom_eb_cdef_directions[(dir - 2)][1];

    const int *pri_taps = svt_aom_eb_cdef_pri_taps[(pri_strength >> coeff_shift) & 1];
    const int *sec_taps = svt_aom_eb_cdef_sec_taps[(pri_strength >> coeff_shift) & 1];

    if (pri_strength)
        pri_damping = AOMMAX(0, pri_damping - get_msb(pri_strength));
    if (sec_strength)
        sec_damping = AOMMAX(0, sec_damping - get_msb(sec_strength));

    for (i = 0; i < height; i += (2 * subsampling_factor)) {
        sum = v256_zero();
        row = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE)));

        min = max = row;
        // Primary near taps
        p0  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + po1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + po1)));
        p1  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - po1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - po1)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        min = v256_min_s16(v256_min_s16(min, p0), p1);
        p0  = constrain16(p0, row, pri_strength, pri_damping);
        p1  = constrain16(p1, row, pri_strength, pri_damping);

        // sum += pri_taps[0] * (p0 + p1)
        sum = v256_add_16(sum, v256_mullo_s16(v256_dup_s16(pri_taps[0]), v256_add_16(p0, p1)));

        // Primary far taps
        p0  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + po2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + po2)));
        p1  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - po2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - po2)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        min = v256_min_s16(v256_min_s16(min, p0), p1);
        p0  = constrain16(p0, row, pri_strength, pri_damping);
        p1  = constrain16(p1, row, pri_strength, pri_damping);

        // sum += pri_taps[1] * (p0 + p1)
        sum = v256_add_16(sum, v256_mullo_s16(v256_dup_s16(pri_taps[1]), v256_add_16(p0, p1)));

        // Secondary near taps
        p0  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s1o1)));
        p1  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s1o1)));
        p2  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s2o1)));
        p3  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o1)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s2o1)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p2, v256_cmpeq_16(p2, large))),
                           v256_andn(p3, v256_cmpeq_16(p3, large)));
        min = v256_min_s16(v256_min_s16(v256_min_s16(v256_min_s16(min, p0), p1), p2), p3);
        p0  = constrain16(p0, row, sec_strength, sec_damping);
        p1  = constrain16(p1, row, sec_strength, sec_damping);
        p2  = constrain16(p2, row, sec_strength, sec_damping);
        p3  = constrain16(p3, row, sec_strength, sec_damping);

        // sum += sec_taps[0] * (p0 + p1 + p2 + p3)
        sum = v256_add_16(
            sum, v256_mullo_s16(v256_dup_s16(sec_taps[0]), v256_add_16(v256_add_16(p0, p1), v256_add_16(p2, p3))));

        // Secondary far taps
        p0  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s1o2)));
        p1  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s1o2)));
        p2  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE + s2o2)));
        p3  = v256_from_v128_s8(loadu_s8((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o2)),
                                loadu_s8((int64x2_t *)(in + (i + subsampling_factor) * CDEF_BSTRIDE - s2o2)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p2, v256_cmpeq_16(p2, large))),
                           v256_andn(p3, v256_cmpeq_16(p3, large)));
        min = v256_min_s16(v256_min_s16(v256_min_s16(v256_min_s16(min, p0), p1), p2), p3);
        p0  = constrain16(p0, row, sec_strength, sec_damping);
        p1  = constrain16(p1, row, sec_strength, sec_damping);
        p2  = constrain16(p2, row, sec_strength, sec_damping);
        p3  = constrain16(p3, row, sec_strength, sec_damping);

        // sum += sec_taps[1] * (p0 + p1 + p2 + p3)
        sum = v256_add_16(
            sum, v256_mullo_s16(v256_dup_s16(sec_taps[1]), v256_add_16(v256_add_16(p0, p1), v256_add_16(p2, p3))));

        // res = row + ((sum - (sum < 0) + 8) >> 4)
        sum = v256_add_16(sum, v256_cmplt_s16(sum, v256_zero()));
        res = v256_add_16(sum, v256_dup_s16(8));
        res = v256_shr_n_s16(res, 4);
        res = v256_add_16(row, res);
        res = v256_min_s16(v256_max_s16(res, min), max);

        storeu_s64((int64x2_t *)(dst + i * dstride), res.val[1]);
        storeu_s64((int64x2_t *)(dst + (i + subsampling_factor) * dstride), res.val[0]);
    }
}

void svt_av1_cdef_filter_block_4xn_16_neon(uint16_t *dst, int dstride, const uint16_t *in, int pri_strength,
                                           int sec_strength, int dir, int pri_damping, int sec_damping,
                                           int coeff_shift, uint8_t height, uint8_t subsampling_factor) {
    int           i;
    v256          p0, p1, p2, p3, sum, row, res;
    v256          max, min, large = v256_dup_s16(CDEF_VERY_LARGE);
    const int32_t po1  = svt_aom_eb_cdef_directions[dir][0];
    const int32_t po2  = svt_aom_eb_cdef_directions[dir][1];
    const int32_t s1o1 = svt_aom_eb_cdef_directions[(dir + 2)][0];
    const int32_t s1o2 = svt_aom_eb_cdef_directions[(dir + 2)][1];
    const int32_t s2o1 = svt_aom_eb_cdef_directions[(dir - 2)][0];
    const int32_t s2o2 = svt_aom_eb_cdef_directions[(dir - 2)][1];

    const int *pri_taps = svt_aom_eb_cdef_pri_taps[(pri_strength >> coeff_shift) & 1];
    const int *sec_taps = svt_aom_eb_cdef_sec_taps[(pri_strength >> coeff_shift) & 1];

    if (pri_strength)
        pri_damping = AOMMAX(0, pri_damping - get_msb(pri_strength));
    if (sec_strength)
        sec_damping = AOMMAX(0, sec_damping - get_msb(sec_strength));
    for (i = 0; i < height; i += (4 * subsampling_factor)) {
        sum = v256_zero();
        row = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE)));
        min = max = row;

        // Primary near taps
        p0  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + po1)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE + po1)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE + po1)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE + po1)));
        p1  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - po1)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE - po1)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE - po1)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE - po1)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        min = v256_min_s16(v256_min_s16(min, p0), p1);
        p0  = constrain16(p0, row, pri_strength, pri_damping);
        p1  = constrain16(p1, row, pri_strength, pri_damping);

        // sum += pri_taps[0] * (p0 + p1)
        sum = v256_add_16(sum, v256_mullo_s16(v256_dup_s16(pri_taps[0]), v256_add_16(p0, p1)));

        // Primary far taps
        p0  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + po2)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE + po2)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE + po2)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE + po2)));
        p1  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - po2)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE - po2)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE - po2)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE - po2)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        min = v256_min_s16(v256_min_s16(min, p0), p1);
        p0  = constrain16(p0, row, pri_strength, pri_damping);
        p1  = constrain16(p1, row, pri_strength, pri_damping);

        // sum += pri_taps[1] * (p0 + p1)
        sum = v256_add_16(sum, v256_mullo_s16(v256_dup_s16(pri_taps[1]), v256_add_16(p0, p1)));

        // Secondary near taps
        p0  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o1)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE + s1o1)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE + s1o1)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE + s1o1)));
        p1  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o1)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE - s1o1)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE - s1o1)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE - s1o1)));
        p2  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o1)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE + s2o1)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE + s2o1)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE + s2o1)));
        p3  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o1)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE - s2o1)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE - s2o1)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE - s2o1)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p2, v256_cmpeq_16(p2, large))),
                           v256_andn(p3, v256_cmpeq_16(p3, large)));
        min = v256_min_s16(v256_min_s16(v256_min_s16(v256_min_s16(min, p0), p1), p2), p3);
        p0  = constrain16(p0, row, sec_strength, sec_damping);
        p1  = constrain16(p1, row, sec_strength, sec_damping);
        p2  = constrain16(p2, row, sec_strength, sec_damping);
        p3  = constrain16(p3, row, sec_strength, sec_damping);

        // sum += sec_taps[0] * (p0 + p1 + p2 + p3)
        sum = v256_add_16(
            sum, v256_mullo_s16(v256_dup_s16(sec_taps[0]), v256_add_16(v256_add_16(p0, p1), v256_add_16(p2, p3))));

        // Secondary far taps
        p0  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s1o2)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE + s1o2)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE + s1o2)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE + s1o2)));
        p1  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s1o2)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE - s1o2)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE - s1o2)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE - s1o2)));
        p2  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE + s2o2)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE + s2o2)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE + s2o2)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE + s2o2)));
        p3  = v256_from_v64(loadl_s64((int64x2_t *)(in + i * CDEF_BSTRIDE - s2o2)),
                            loadl_s64((int64x2_t *)(in + (i + 1 * subsampling_factor) * CDEF_BSTRIDE - s2o2)),
                            loadl_s64((int64x2_t *)(in + (i + 2 * subsampling_factor) * CDEF_BSTRIDE - s2o2)),
                            loadl_s64((int64x2_t *)(in + (i + 3 * subsampling_factor) * CDEF_BSTRIDE - s2o2)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p0, v256_cmpeq_16(p0, large))),
                           v256_andn(p1, v256_cmpeq_16(p1, large)));
        max = v256_max_s16(v256_max_s16(max, v256_andn(p2, v256_cmpeq_16(p2, large))),
                           v256_andn(p3, v256_cmpeq_16(p3, large)));
        min = v256_min_s16(v256_min_s16(v256_min_s16(v256_min_s16(min, p0), p1), p2), p3);
        p0  = constrain16(p0, row, sec_strength, sec_damping);
        p1  = constrain16(p1, row, sec_strength, sec_damping);
        p2  = constrain16(p2, row, sec_strength, sec_damping);
        p3  = constrain16(p3, row, sec_strength, sec_damping);

        // sum += sec_taps[1] * (p0 + p1 + p2 + p3)
        sum = v256_add_16(
            sum, v256_mullo_s16(v256_dup_s16(sec_taps[1]), v256_add_16(v256_add_16(p0, p1), v256_add_16(p2, p3))));

        // res = row + ((sum - (sum < 0) + 8) >> 4)
        sum = v256_add_16(sum, v256_cmplt_s16(sum, v256_zero()));
        res = v256_add_16(sum, v256_dup_s16(8));
        res = v256_shr_n_s16(res, 4);
        res = v256_add_16(row, res);
        res = v256_min_s16(v256_max_s16(res, min), max);

        storel_s64((int64x2_t *)(dst + i * dstride), vreinterpretq_s64_s8(srli_s8(vreinterpretq_s8_s64(res.val[1]), 8)));
        storel_s64((int64x2_t *)(dst + (i + 1 * subsampling_factor) * dstride), res.val[1]);
        storel_s64((int64x2_t *)(dst + (i + 2 * subsampling_factor) * dstride), vreinterpretq_s64_s8(srli_s8(vreinterpretq_s8_s64(res.val[0]), 8)));
        storel_s64((int64x2_t *)(dst + (i + 3 * subsampling_factor) * dstride), res.val[0]);
    }
}

void svt_cdef_filter_block_neon(uint8_t *dst8, uint16_t *dst16, int dstride, const uint16_t *in, int pri_strength,
                                int sec_strength, int dir, int pri_damping, int sec_damping, int bsize,
                                int coeff_shift, uint8_t subsampling_factor) {
    if (dst8) {
        switch(bsize){
            case BLOCK_8X8:
                svt_av1_cdef_filter_block_8xn_8_neon(
                    dst8,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    8,
                    subsampling_factor);
                break;
            case BLOCK_4X8:
                svt_av1_cdef_filter_block_4xn_8_neon(
                    dst8,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    8,
                    subsampling_factor);
                break;
            case BLOCK_8X4:
                svt_av1_cdef_filter_block_8xn_8_neon(
                    dst8,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    4,
                    subsampling_factor);
                break;
            default:
                svt_av1_cdef_filter_block_4xn_8_neon(
                    dst8,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    4,
                    1);
        }
    } else {
        switch(bsize){
            case BLOCK_8X8:
                svt_av1_cdef_filter_block_8xn_16_neon(
                    dst16,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    8,
                    subsampling_factor);
                break;
            case BLOCK_4X8:
                svt_av1_cdef_filter_block_4xn_16_neon(
                    dst16,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    8,
                    subsampling_factor);
                break;
            case BLOCK_8X4:
                svt_av1_cdef_filter_block_8xn_16_neon(
                    dst16,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    4,
                    subsampling_factor);
                break;
            default:
                assert(bsize == BLOCK_4X4);
                svt_av1_cdef_filter_block_4xn_16_neon(
                    dst16,
                    dstride,
                    in,
                    pri_strength,
                    sec_strength,
                    dir,
                    pri_damping,
                    sec_damping,
                    coeff_shift,
                    4,
                    1);
        }
    }
}