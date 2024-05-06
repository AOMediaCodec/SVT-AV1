/*
 * Copyright(c) 2024 Intel Corporation
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
 */

#include "EbDefinitions.h"
#include "EbMotionEstimation.h"

#include <arm_neon.h>

static INLINE void copy_32_bytes_neon(const int32_t *src, int32_t *dst) {
    // const __m256i val = _mm256_loadu_si256((__m256i *)(src + 0 * 8));
    const int32x4_t lo = vld1q_s32(src + 0);
    const int32x4_t hi = vld1q_s32(src + 4);

    // _mm256_storeu_si256((__m256i *)(dst + 0 * 8), val);
    vst1q_s32(dst + 0, lo);
    vst1q_s32(dst + 4, hi);
}

static INLINE uint64_t hadd64_neon(const uint64x2x2_t sum256) {
    // const __m128i sum256_lo = _mm256_castsi256_si128(sum256);
    // const __m128i sum256_hi = _mm256_extracti128_si256(sum256, 1);
    const uint64x2_t sum256_lo = sum256.val[0];
    const uint64x2_t sum256_hi = sum256.val[1];

    // const __m128i sum128 = _mm_add_epi64(sum256_lo, sum256_hi);
    // const __m128i sum128_hi = _mm_srli_si128(sum128, 8);
    // const __m128i sum = _mm_add_epi64(sum128, sum128_hi);
    const uint64x2_t sum128    = vaddq_u64(sum256_lo, sum256_hi);
    const uint64x2_t sum128_hi = vextq_u64(sum128, sum128, 1);
    const uint64x2_t sum       = vaddq_u64(sum128, sum128_hi);

    return vget_lane_u64(sum, 0);
}

static INLINE void energy_computation_kernel_neon(const int32_t *const in, uint64x2x2_t *const sum256) {
    // const __m256i zero = _mm256_setzero_si256();

    // const __m256i input = _mm256_loadu_si256((__m256i *)in);
    // const __m256i in_lo = _mm256_unpacklo_epi32(input, zero);
    // const __m256i in_hi = _mm256_unpackhi_epi32(input, zero);
    const int32x2_t input_0 = vld1_s32(in + 0);
    const int32x2_t input_1 = vld1_s32(in + 2);
    const int32x2_t input_2 = vld1_s32(in + 4);
    const int32x2_t input_3 = vld1_s32(in + 6);

    // const __m256i energy_lo = _mm256_mul_epi32(in_lo, in_lo);
    // const __m256i energy_hi = _mm256_mul_epi32(in_hi, in_hi);
    const int64x2_t energy_input_0 = vreinterpret_u64_s64(vmull_s32(input_0, input_0));
    const int64x2_t energy_input_1 = vreinterpret_u64_s64(vmull_s32(input_1, input_1));
    const int64x2_t energy_input_2 = vreinterpret_u64_s64(vmull_s32(input_2, input_2));
    const int64x2_t energy_input_3 = vreinterpret_u64_s64(vmull_s32(input_3, input_3));

    // *sum256 = _mm256_add_epi64(*sum256, energy_lo);
    // *sum256 = _mm256_add_epi64(*sum256, energy_hi);
    sum256->val[0] = vaddq_u64(sum256->val[0], energy_input_0);
    sum256->val[0] = vaddq_u64(sum256->val[0], energy_input_1);
    sum256->val[1] = vaddq_u64(sum256->val[1], energy_input_2);
    sum256->val[1] = vaddq_u64(sum256->val[1], energy_input_3);
}

static INLINE void copy_256x_bytes_neon(const int32_t *src, int32_t *dst, const uint32_t height) {
    uint32_t h = height;

    do {
        copy_32_bytes_neon(src + 0 * 8, dst + 0 * 8);
        copy_32_bytes_neon(src + 1 * 8, dst + 1 * 8);
        copy_32_bytes_neon(src + 2 * 8, dst + 2 * 8);
        copy_32_bytes_neon(src + 3 * 8, dst + 3 * 8);
        src += 64;
        dst += 32;
    } while (--h);
}

static INLINE uint64_t energy_computation_neon(const int32_t *const in, const uint32_t size) {
    // const __m256i zero = _mm256_setzero_si256();
    // uint32_t i = 0;
    // __m256i sum = zero;

    uint64x2x2_t sum;
    sum.val[0] = vdupq_n_u64(0);
    sum.val[1] = vdupq_n_u64(0);

    uint32_t i = 0;
    do {
        energy_computation_kernel_neon(in + i, &sum);
        i += 8;
    } while (i < size);

    return hadd64_neon(sum);
}

static INLINE uint64_t energy_computation_64_neon(const int32_t *in, const uint32_t height) {
    // const __m256i zero = _mm256_setzero_si256();
    // uint32_t      i    = height;
    // __m256i       sum  = zero;

    uint64x2x2_t sum;
    sum.val[0] = vdupq_n_u64(0);
    sum.val[1] = vdupq_n_u64(0);

    uint32_t i = height;

    do {
        energy_computation_kernel_neon(in + 0 * 8, &sum);
        energy_computation_kernel_neon(in + 1 * 8, &sum);
        energy_computation_kernel_neon(in + 2 * 8, &sum);
        energy_computation_kernel_neon(in + 3 * 8, &sum);
        in += 64;
    } while (--i);

    return hadd64_neon(sum);
}

uint64_t svt_handle_transform64x64_neon(int32_t *output) {
    uint64_t three_quad_energy;

    // top - right 32x32 area.
    three_quad_energy = energy_computation_64_neon(output + 32, 32);
    // bottom 64x32 area.
    three_quad_energy += energy_computation_neon(output + 32 * 64, 64 * 32);
    // Re-pack non-zero coeffs in the first 32x32 indices.
    copy_256x_bytes_neon(output + 64, output + 32, 31);

    return three_quad_energy;
}
