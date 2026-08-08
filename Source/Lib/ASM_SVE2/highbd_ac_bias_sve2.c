/*
 * Copyright (c) 2026, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include <stdlib.h>

#include "common_dsp_rtcd.h"
#include "hadamard_path_sve2.h"
#include "mem_neon.h"
#include "sum_neon.h"
#include "transpose_neon.h"

#if CONFIG_ENABLE_HIGH_BIT_DEPTH
static inline int highbd_psy_energy_4x4_sve2(const uint16_t* src, ptrdiff_t src_stride) {
    int16x4_t s0, s1, s2, s3;
    load_s16_4x4((int16_t*)src, src_stride, &s0, &s1, &s2, &s3);

    int16x8_t a0 = vcombine_s16(vhadd_s16(s0, s1), vhsub_s16(s0, s1));
    int16x8_t a1 = vcombine_s16(vhadd_s16(s2, s3), vhsub_s16(s2, s3));

    int16x8_t b0 = vaddq_s16(a0, a1);
    int16x8_t b1 = vsubq_s16(a0, a1);

    a0 = vtrn1q_s16(b0, b1);
    a1 = vtrn2q_s16(b0, b1);

    b0 = vhaddq_s16(a0, a1);
    b1 = vhsubq_s16(a0, a1);

    a0 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(b0), vreinterpretq_s32_s16(b1)));
    a1 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(b0), vreinterpretq_s32_s16(b1)));

    const int dc = vgetq_lane_s16(vaddq_s16(a0, a1), 0);

    const int16x8_t max = vmaxq_s16(vabsq_s16(a0), vabsq_s16(a1));
    const int       sum = vaddlvq_s16(max);

    return (sum << 2) - dc;
}

static inline int32x4_t highbd_psy_energy_8x8_partial_sve2(const uint16_t* src, ptrdiff_t src_stride,
                                                           int32x4_t* dc_coeff) {
    int16x8_t a[8];
    load_s16_8x8((int16_t*)src, src_stride, &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7]);

    hadamard_8x8_cadd_pass_sve2(a);

    const int16x8_t a0 = vaddq_s16(a[0], a[1]);
    const int16x8_t a1 = vsubq_s16(a[0], a[1]);
    const int16x8_t a2 = vaddq_s16(a[2], a[3]);
    const int16x8_t a3 = vsubq_s16(a[2], a[3]);
    const int16x8_t a4 = vaddq_s16(a[4], a[5]);
    const int16x8_t a5 = vsubq_s16(a[4], a[5]);
    const int16x8_t a6 = vaddq_s16(a[6], a[7]);
    const int16x8_t a7 = vsubq_s16(a[6], a[7]);

    const int16x8_t b0 = vabsq_s16(vaddq_s16(a0, a2));
    const int16x8_t b1 = vabdq_s16(a0, a2);
    const int16x8_t b2 = vabsq_s16(vaddq_s16(a1, a3));
    const int16x8_t b3 = vabdq_s16(a1, a3);
    const int16x8_t b4 = vabsq_s16(vaddq_s16(a4, a6));
    const int16x8_t b5 = vabdq_s16(a4, a6);
    const int16x8_t b6 = vabsq_s16(vaddq_s16(a5, a7));
    const int16x8_t b7 = vabdq_s16(a5, a7);

    *dc_coeff = vaddl_high_s16(b0, b4);

    int16x8_t max[4];
    max[0] = vmaxq_s16(b0, b4);
    max[1] = vmaxq_s16(b1, b5);
    max[2] = vmaxq_s16(b2, b6);
    max[3] = vmaxq_s16(b3, b7);

    return horizontal_add_4d_s16x8(max);
}

static inline int highbd_psy_energy_8x8_sve2(const uint16_t* src, ptrdiff_t src_stride) {
    int16x8_t s[8];
    load_s16_8x8((int16_t*)src, src_stride, &s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7]);

    hadamard_8x8_cadd_pass_sve2(s);

    int16x8_t a[8];
    a[0] = vaddq_s16(s[0], s[1]);
    a[1] = vsubq_s16(s[0], s[1]);
    a[2] = vaddq_s16(s[2], s[3]);
    a[3] = vsubq_s16(s[2], s[3]);
    a[4] = vaddq_s16(s[4], s[5]);
    a[5] = vsubq_s16(s[4], s[5]);
    a[6] = vaddq_s16(s[6], s[7]);
    a[7] = vsubq_s16(s[6], s[7]);

    int16x8_t b0 = vabsq_s16(vaddq_s16(a[0], a[2]));
    int16x8_t b2 = vabsq_s16(vaddq_s16(a[1], a[3]));
    int16x8_t b1 = vabdq_s16(a[0], a[2]);
    int16x8_t b3 = vabdq_s16(a[1], a[3]);
    int16x8_t b4 = vabsq_s16(vaddq_s16(a[4], a[6]));
    int16x8_t b6 = vabsq_s16(vaddq_s16(a[5], a[7]));
    int16x8_t b5 = vabdq_s16(a[4], a[6]);
    int16x8_t b7 = vabdq_s16(a[5], a[7]);

    int16x8_t max[4];
    max[0] = vmaxq_s16(b0, b4);
    max[1] = vmaxq_s16(b1, b5);
    max[2] = vmaxq_s16(b2, b6);
    max[3] = vmaxq_s16(b3, b7);

    const int       dc   = vgetq_lane_s32(vaddl_high_s16(b0, b4), 3);
    const int32x4_t sum  = horizontal_add_4d_s16x8(max);
    const int       satd = vaddvq_s32(sum) << 1;

    return ((satd + 2) >> 2) - ((dc + 2) >> 2);
}

static inline int32x4_t highbd_psy_energy_16x16_sve2(const uint16_t* src, ptrdiff_t src_stride) {
    int32x4_t dc_coeff[4];
    int32x4_t sum[4];

    sum[0] = highbd_psy_energy_8x8_partial_sve2(src, src_stride, &dc_coeff[0]);
    sum[1] = highbd_psy_energy_8x8_partial_sve2(src + 8, src_stride, &dc_coeff[1]);
    sum[2] = highbd_psy_energy_8x8_partial_sve2(src + 8 * src_stride, src_stride, &dc_coeff[2]);
    sum[3] = highbd_psy_energy_8x8_partial_sve2(src + 8 * src_stride + 8, src_stride, &dc_coeff[3]);

    dc_coeff[0] = vzip2q_s32(dc_coeff[0], dc_coeff[1]);
    dc_coeff[2] = vzip2q_s32(dc_coeff[2], dc_coeff[3]);
    dc_coeff[0] = vcombine_s32(vget_high_s32(dc_coeff[0]), vget_high_s32(dc_coeff[2]));

    sum[0] = vpaddq_s32(sum[0], sum[1]);
    sum[2] = vpaddq_s32(sum[2], sum[3]);
    sum[0] = vpaddq_s32(sum[0], sum[2]);

    const int32x4_t dc_energy   = vrshrq_n_s32(vabsq_s32(dc_coeff[0]), 2);
    const int32x4_t sa8d_energy = vrshrq_n_s32(sum[0], 1);

    return vsubq_s32(sa8d_energy, dc_energy);
}

uint64_t svt_psy_distortion_hbd_sve2(const uint16_t* input, const uint32_t input_stride, const uint16_t* recon,
                                     const uint32_t recon_stride, const uint32_t width, const uint32_t height) {
    uint64_t energy_gap = 0;

    if (width % 16 == 0 && height % 16 == 0) {
        int32x4_t energy_gap_16x16 = vdupq_n_s32(0);

        for (uint32_t j = 0; j < height; j += 16) {
            for (uint32_t i = 0; i < width; i += 16) {
                const int32x4_t input_energy = highbd_psy_energy_16x16_sve2(input + j * input_stride + i, input_stride);
                const int32x4_t recon_energy = highbd_psy_energy_16x16_sve2(recon + j * recon_stride + i, recon_stride);

                energy_gap_16x16 = vabaq_s32(energy_gap_16x16, input_energy, recon_energy);
            }
        }

        energy_gap += (uint64_t)vaddlvq_s32(energy_gap_16x16);
    } else if (width >= 8 && height >= 8) {
        for (uint32_t j = 0; j < height; j += 8) {
            for (uint32_t i = 0; i < width; i += 8) {
                const int input_energy = highbd_psy_energy_8x8_sve2(input + j * input_stride + i, input_stride);
                const int recon_energy = highbd_psy_energy_8x8_sve2(recon + j * recon_stride + i, recon_stride);

                energy_gap += abs(input_energy - recon_energy);
            }
        }
    } else {
        for (uint32_t j = 0; j < height; j += 4) {
            for (uint32_t i = 0; i < width; i += 4) {
                const int input_energy = highbd_psy_energy_4x4_sve2(input + j * input_stride + i, input_stride);
                const int recon_energy = highbd_psy_energy_4x4_sve2(recon + j * recon_stride + i, recon_stride);

                energy_gap += abs(input_energy - recon_energy);
            }
        }
    }

    return energy_gap << 2;
}
#endif
