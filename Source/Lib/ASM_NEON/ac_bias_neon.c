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
#include "hadamard_path_neon.h"
#include "mem_neon.h"
#include "sum_neon.h"
#include "transpose_neon.h"

static inline int psy_energy_4x4_neon(const uint8_t* src, ptrdiff_t src_stride) {
    const uint8x8_t s02_u8 = load_u8x4_strided_x2((uint8_t*)src + 0 * src_stride, 2 * src_stride);
    const uint8x8_t s13_u8 = load_u8x4_strided_x2((uint8_t*)src + 1 * src_stride, 2 * src_stride);

    const int16x8_t s02 = vreinterpretq_s16_u16(vmovl_u8(s02_u8));
    const int16x8_t s13 = vreinterpretq_s16_u16(vmovl_u8(s13_u8));

    int16x8_t a0 = vhaddq_s16(s02, s13);
    int16x8_t a1 = vhsubq_s16(s02, s13);

    int16x8_t b0 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a0), vreinterpretq_s64_s16(a1)));
    int16x8_t b1 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a0), vreinterpretq_s64_s16(a1)));

    a0 = vaddq_s16(b0, b1);
    a1 = vsubq_s16(b0, b1);

    b0 = vtrn1q_s16(a0, a1);
    b1 = vtrn2q_s16(a0, a1);

    a0 = vhaddq_s16(b0, b1);
    a1 = vhsubq_s16(b0, b1);

    b0 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a0), vreinterpretq_s32_s16(a1)));
    b1 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a0), vreinterpretq_s32_s16(a1)));

    const int dc = vgetq_lane_s16(vaddq_s16(b0, b1), 0);

    a0 = vabsq_s16(b0);
    a1 = vabsq_s16(b1);

    const int satd = vaddlvq_s16(vmaxq_s16(a0, a1)) << 1;
    return (satd << 1) - dc;
}

static inline uint32x4_t hadamard_8x8_neon(const uint8_t* src, ptrdiff_t src_stride, int32x4_t* dc) {
    uint8x8_t s0, s1, s2, s3, s4, s5, s6, s7;

    load_u8_8x8(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);

    int16x8_t a[8];
    a[0] = vreinterpretq_s16_u16(vmovl_u8(s0));
    a[1] = vreinterpretq_s16_u16(vmovl_u8(s1));
    a[2] = vreinterpretq_s16_u16(vmovl_u8(s2));
    a[3] = vreinterpretq_s16_u16(vmovl_u8(s3));
    a[4] = vreinterpretq_s16_u16(vmovl_u8(s4));
    a[5] = vreinterpretq_s16_u16(vmovl_u8(s5));
    a[6] = vreinterpretq_s16_u16(vmovl_u8(s6));
    a[7] = vreinterpretq_s16_u16(vmovl_u8(s7));

    hadamard_8x8_one_pass(a);
    *dc = vpaddlq_s16(a[0]);

    int16x8_t b0 = vtrn1q_s16(a[0], a[1]);
    int16x8_t b1 = vtrn2q_s16(a[0], a[1]);
    int16x8_t b2 = vtrn1q_s16(a[2], a[3]);
    int16x8_t b3 = vtrn2q_s16(a[2], a[3]);
    int16x8_t b4 = vtrn1q_s16(a[4], a[5]);
    int16x8_t b5 = vtrn2q_s16(a[4], a[5]);
    int16x8_t b6 = vtrn1q_s16(a[6], a[7]);
    int16x8_t b7 = vtrn2q_s16(a[6], a[7]);

    a[0] = vaddq_s16(b0, b1);
    a[1] = vsubq_s16(b0, b1);
    a[2] = vaddq_s16(b2, b3);
    a[3] = vsubq_s16(b2, b3);
    a[4] = vaddq_s16(b4, b5);
    a[5] = vsubq_s16(b4, b5);
    a[6] = vaddq_s16(b6, b7);
    a[7] = vsubq_s16(b6, b7);

    b0 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[0]), vreinterpretq_s32_s16(a[1])));
    b1 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[0]), vreinterpretq_s32_s16(a[1])));
    b2 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[2]), vreinterpretq_s32_s16(a[3])));
    b3 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[2]), vreinterpretq_s32_s16(a[3])));
    b4 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[4]), vreinterpretq_s32_s16(a[5])));
    b5 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[4]), vreinterpretq_s32_s16(a[5])));
    b6 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[6]), vreinterpretq_s32_s16(a[7])));
    b7 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[6]), vreinterpretq_s32_s16(a[7])));

    a[0] = vabsq_s16(vaddq_s16(b0, b1));
    a[1] = vabdq_s16(b0, b1);
    a[2] = vabsq_s16(vaddq_s16(b2, b3));
    a[3] = vabdq_s16(b2, b3);
    a[4] = vabsq_s16(vaddq_s16(b4, b5));
    a[5] = vabdq_s16(b4, b5);
    a[6] = vabsq_s16(vaddq_s16(b6, b7));
    a[7] = vabdq_s16(b6, b7);

    b0 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[0]), vreinterpretq_s64_s16(a[1])));
    b1 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[0]), vreinterpretq_s64_s16(a[1])));
    b2 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[2]), vreinterpretq_s64_s16(a[3])));
    b3 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[2]), vreinterpretq_s64_s16(a[3])));
    b4 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[4]), vreinterpretq_s64_s16(a[5])));
    b5 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[4]), vreinterpretq_s64_s16(a[5])));
    b6 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[6]), vreinterpretq_s64_s16(a[7])));
    b7 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[6]), vreinterpretq_s64_s16(a[7])));

    int16x8_t max[4];
    max[0] = vmaxq_s16(b0, b1);
    max[1] = vmaxq_s16(b2, b3);
    max[2] = vmaxq_s16(b4, b5);
    max[3] = vmaxq_s16(b6, b7);

    return vreinterpretq_u32_s32(horizontal_add_4d_s16x8(max));
}

static inline int psy_energy_8x8_neon(const uint8_t* src, ptrdiff_t src_stride) {
    uint8x8_t s0, s1, s2, s3, s4, s5, s6, s7;

    load_u8_8x8(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);

    int16x8_t a[8];
    a[0] = vreinterpretq_s16_u16(vmovl_u8(s0));
    a[1] = vreinterpretq_s16_u16(vmovl_u8(s1));
    a[2] = vreinterpretq_s16_u16(vmovl_u8(s2));
    a[3] = vreinterpretq_s16_u16(vmovl_u8(s3));
    a[4] = vreinterpretq_s16_u16(vmovl_u8(s4));
    a[5] = vreinterpretq_s16_u16(vmovl_u8(s5));
    a[6] = vreinterpretq_s16_u16(vmovl_u8(s6));
    a[7] = vreinterpretq_s16_u16(vmovl_u8(s7));

    hadamard_8x8_one_pass(a);
    const int dc = vaddlvq_s16(a[0]);

    int16x8_t b0 = vtrn1q_s16(a[0], a[1]);
    int16x8_t b1 = vtrn2q_s16(a[0], a[1]);
    int16x8_t b2 = vtrn1q_s16(a[2], a[3]);
    int16x8_t b3 = vtrn2q_s16(a[2], a[3]);
    int16x8_t b4 = vtrn1q_s16(a[4], a[5]);
    int16x8_t b5 = vtrn2q_s16(a[4], a[5]);
    int16x8_t b6 = vtrn1q_s16(a[6], a[7]);
    int16x8_t b7 = vtrn2q_s16(a[6], a[7]);

    a[0] = vaddq_s16(b0, b1);
    a[1] = vsubq_s16(b0, b1);
    a[2] = vaddq_s16(b2, b3);
    a[3] = vsubq_s16(b2, b3);
    a[4] = vaddq_s16(b4, b5);
    a[5] = vsubq_s16(b4, b5);
    a[6] = vaddq_s16(b6, b7);
    a[7] = vsubq_s16(b6, b7);

    b0 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[0]), vreinterpretq_s32_s16(a[1])));
    b1 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[0]), vreinterpretq_s32_s16(a[1])));
    b2 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[2]), vreinterpretq_s32_s16(a[3])));
    b3 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[2]), vreinterpretq_s32_s16(a[3])));
    b4 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[4]), vreinterpretq_s32_s16(a[5])));
    b5 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[4]), vreinterpretq_s32_s16(a[5])));
    b6 = vreinterpretq_s16_s32(vtrn1q_s32(vreinterpretq_s32_s16(a[6]), vreinterpretq_s32_s16(a[7])));
    b7 = vreinterpretq_s16_s32(vtrn2q_s32(vreinterpretq_s32_s16(a[6]), vreinterpretq_s32_s16(a[7])));

    a[0] = vabsq_s16(vaddq_s16(b0, b1));
    a[1] = vabdq_s16(b0, b1);
    a[2] = vabsq_s16(vaddq_s16(b2, b3));
    a[3] = vabdq_s16(b2, b3);
    a[4] = vabsq_s16(vaddq_s16(b4, b5));
    a[5] = vabdq_s16(b4, b5);
    a[6] = vabsq_s16(vaddq_s16(b6, b7));
    a[7] = vabdq_s16(b6, b7);

    b0 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[0]), vreinterpretq_s64_s16(a[1])));
    b1 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[0]), vreinterpretq_s64_s16(a[1])));
    b2 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[2]), vreinterpretq_s64_s16(a[3])));
    b3 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[2]), vreinterpretq_s64_s16(a[3])));
    b4 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[4]), vreinterpretq_s64_s16(a[5])));
    b5 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[4]), vreinterpretq_s64_s16(a[5])));
    b6 = vreinterpretq_s16_s64(vtrn1q_s64(vreinterpretq_s64_s16(a[6]), vreinterpretq_s64_s16(a[7])));
    b7 = vreinterpretq_s16_s64(vtrn2q_s64(vreinterpretq_s64_s16(a[6]), vreinterpretq_s64_s16(a[7])));

    int16x8_t max[4];
    max[0] = vmaxq_s16(b0, b1);
    max[1] = vmaxq_s16(b2, b3);
    max[2] = vmaxq_s16(b4, b5);
    max[3] = vmaxq_s16(b6, b7);

    const int32x4_t sum  = horizontal_add_4d_s16x8(max);
    const int       satd = vaddvq_s32(sum) << 1;

    return ((satd + 2) >> 2) - ((dc + 2) >> 2);
}

static inline int32x4_t psy_energy_16x16_neon(const uint8_t* src, ptrdiff_t src_stride) {
    int32x4_t  dc[4];
    uint32x4_t sum[4];

    sum[0] = hadamard_8x8_neon(src, src_stride, &dc[0]);
    sum[1] = hadamard_8x8_neon(src + 8, src_stride, &dc[1]);
    sum[2] = hadamard_8x8_neon(src + 8 * src_stride, src_stride, &dc[2]);
    sum[3] = hadamard_8x8_neon(src + 8 * src_stride + 8, src_stride, &dc[3]);

    dc[0] = vpaddq_s32(dc[0], dc[1]);
    dc[2] = vpaddq_s32(dc[2], dc[3]);
    dc[0] = vpaddq_s32(dc[0], dc[2]);

    sum[0] = vpaddq_u32(sum[0], sum[1]);
    sum[2] = vpaddq_u32(sum[2], sum[3]);
    sum[0] = vpaddq_u32(sum[0], sum[2]);

    const int32x4_t dc_energy   = vrshrq_n_s32(dc[0], 2);
    const int32x4_t satd_energy = vreinterpretq_s32_u32(vrshrq_n_u32(sum[0], 1));

    return vsubq_s32(satd_energy, dc_energy);
}

uint64_t svt_psy_distortion_neon(const uint8_t* input, const uint32_t input_stride, const uint8_t* recon,
                                 const uint32_t recon_stride, const uint32_t width, const uint32_t height) {
    uint64_t energy_gap = 0;

    if (width % 16 == 0 && height % 16 == 0) {
        int32x4_t energy_gap_16x16 = vdupq_n_s32(0);

        for (uint32_t j = 0; j < height; j += 16) {
            for (uint32_t i = 0; i < width; i += 16) {
                const int32x4_t input_energy = psy_energy_16x16_neon(input + j * input_stride + i, input_stride);
                const int32x4_t recon_energy = psy_energy_16x16_neon(recon + j * recon_stride + i, recon_stride);

                energy_gap_16x16 = vabaq_s32(energy_gap_16x16, input_energy, recon_energy);
            }
        }
        energy_gap += (uint64_t)vaddlvq_s32(energy_gap_16x16);
    } else if (width >= 8 && height >= 8) {
        for (uint32_t j = 0; j < height; j += 8) {
            for (uint32_t i = 0; i < width; i += 8) {
                const int input_energy = psy_energy_8x8_neon(input + j * input_stride + i, input_stride);
                const int recon_energy = psy_energy_8x8_neon(recon + j * recon_stride + i, recon_stride);

                energy_gap += abs(input_energy - recon_energy);
            }
        }
    } else {
        for (uint32_t j = 0; j < height; j += 4) {
            for (uint32_t i = 0; i < width; i += 4) {
                const int input_energy = psy_energy_4x4_neon(input + j * input_stride + i, input_stride);
                const int recon_energy = psy_energy_4x4_neon(recon + j * recon_stride + i, recon_stride);

                energy_gap += abs(input_energy - recon_energy);
            }
        }
    }

    return energy_gap;
}
