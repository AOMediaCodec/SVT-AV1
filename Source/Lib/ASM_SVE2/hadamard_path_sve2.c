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

#include "common_dsp_rtcd.h"
#include "hadamard_path_neon.h"
#include "hadamard_path_sve2.h"
#include "mem_neon.h"
#include "sum_neon.h"

int svt_av1_hadamard_satd_8x8_sve2(const uint8_t* src, ptrdiff_t src_stride, const uint8_t* pred,
                                   ptrdiff_t pred_stride) {
    uint8x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint8x8_t p0, p1, p2, p3, p4, p5, p6, p7;

    load_u8_8x8(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);
    load_u8_8x8(pred, pred_stride, &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7);

    int16x8_t a[8];
    a[0] = vreinterpretq_s16_u16(vsubl_u8(s0, p0));
    a[1] = vreinterpretq_s16_u16(vsubl_u8(s1, p1));
    a[2] = vreinterpretq_s16_u16(vsubl_u8(s2, p2));
    a[3] = vreinterpretq_s16_u16(vsubl_u8(s3, p3));
    a[4] = vreinterpretq_s16_u16(vsubl_u8(s4, p4));
    a[5] = vreinterpretq_s16_u16(vsubl_u8(s5, p5));
    a[6] = vreinterpretq_s16_u16(vsubl_u8(s6, p6));
    a[7] = vreinterpretq_s16_u16(vsubl_u8(s7, p7));

    hadamard_8x8_cadd_pass_sve2(a);

    int16x8_t b0 = vaddq_s16(a[0], a[1]);
    int16x8_t b1 = vsubq_s16(a[0], a[1]);
    int16x8_t b2 = vaddq_s16(a[2], a[3]);
    int16x8_t b3 = vsubq_s16(a[2], a[3]);
    int16x8_t b4 = vaddq_s16(a[4], a[5]);
    int16x8_t b5 = vsubq_s16(a[4], a[5]);
    int16x8_t b6 = vaddq_s16(a[6], a[7]);
    int16x8_t b7 = vsubq_s16(a[6], a[7]);

    a[0] = vaddq_s16(b0, b2);
    a[1] = vaddq_s16(b1, b3);
    a[2] = vsubq_s16(b1, b3);
    a[3] = vsubq_s16(b0, b2);
    a[4] = vaddq_s16(b4, b6);
    a[5] = vaddq_s16(b5, b7);
    a[6] = vsubq_s16(b5, b7);
    a[7] = vsubq_s16(b4, b6);

    int16x8_t max[4];
    max[0] = vmaxq_s16(vabsq_s16(a[0]), vabsq_s16(a[4]));
    max[1] = vmaxq_s16(vabsq_s16(a[1]), vabsq_s16(a[5]));
    max[2] = vmaxq_s16(vabsq_s16(a[2]), vabsq_s16(a[6]));
    max[3] = vmaxq_s16(vabsq_s16(a[3]), vabsq_s16(a[7]));

    int32x4_t sum = horizontal_add_4d_s16x8(max);
    return vaddlvq_s32(sum) << 1;
}

static inline void hadamard_8x8_sve2(const uint8_t* src, ptrdiff_t src_stride, const uint8_t* pred,
                                     ptrdiff_t pred_stride, int16x8_t coeff[8]) {
    uint8x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint8x8_t p0, p1, p2, p3, p4, p5, p6, p7;

    load_u8_8x8(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);
    load_u8_8x8(pred, pred_stride, &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7);

    coeff[0] = vreinterpretq_s16_u16(vsubl_u8(s0, p0));
    coeff[1] = vreinterpretq_s16_u16(vsubl_u8(s1, p1));
    coeff[2] = vreinterpretq_s16_u16(vsubl_u8(s2, p2));
    coeff[3] = vreinterpretq_s16_u16(vsubl_u8(s3, p3));
    coeff[4] = vreinterpretq_s16_u16(vsubl_u8(s4, p4));
    coeff[5] = vreinterpretq_s16_u16(vsubl_u8(s5, p5));
    coeff[6] = vreinterpretq_s16_u16(vsubl_u8(s6, p6));
    coeff[7] = vreinterpretq_s16_u16(vsubl_u8(s7, p7));

    hadamard_8x8_cadd_pass_sve2(coeff);
    hadamard_8x8_one_pass(coeff);
}

int svt_av1_hadamard_satd_16x16_sve2(const uint8_t* src, ptrdiff_t src_stride, const uint8_t* pred,
                                     ptrdiff_t pred_stride) {
    const int16x8_t one = vdupq_n_s16(1);
    int16x8_t       q0[8], q1[8], q2[8], q3[8];

    hadamard_8x8_sve2(src + 0 + 0 * src_stride, src_stride, pred + 0 + 0 * pred_stride, pred_stride, q0);
    hadamard_8x8_sve2(src + 8 + 0 * src_stride, src_stride, pred + 8 + 0 * pred_stride, pred_stride, q1);
    hadamard_8x8_sve2(src + 0 + 8 * src_stride, src_stride, pred + 0 + 8 * pred_stride, pred_stride, q2);
    hadamard_8x8_sve2(src + 8 + 8 * src_stride, src_stride, pred + 8 + 8 * pred_stride, pred_stride, q3);

    int64x2_t acc0 = vdupq_n_s64(0);
    int64x2_t acc1 = vdupq_n_s64(0);
    for (int i = 0; i < 8; ++i) {
        const int16x8_t b0 = vabsq_s16(vhaddq_s16(q0[i], q1[i]));
        const int16x8_t b1 = vabsq_s16(vhsubq_s16(q0[i], q1[i]));
        const int16x8_t b2 = vabsq_s16(vhaddq_s16(q2[i], q3[i]));
        const int16x8_t b3 = vabsq_s16(vhsubq_s16(q2[i], q3[i]));

        acc0 = svt_sdotq_s16(acc0, vmaxq_s16(b0, b2), one);
        acc1 = svt_sdotq_s16(acc1, vmaxq_s16(b1, b3), one);
    }

    return (int)vaddvq_s64(vaddq_s64(acc0, acc1)) << 1;
}

static inline void hadamard_16x16_sve2(const uint8_t* src, ptrdiff_t src_stride, const uint8_t* pred,
                                       ptrdiff_t pred_stride, int16x8_t coeff[32]) {
    int16x8_t q0[8], q1[8], q2[8], q3[8];

    hadamard_8x8_sve2(src + 0 + 0 * src_stride, src_stride, pred + 0 + 0 * pred_stride, pred_stride, q0);
    hadamard_8x8_sve2(src + 8 + 0 * src_stride, src_stride, pred + 8 + 0 * pred_stride, pred_stride, q1);
    hadamard_8x8_sve2(src + 0 + 8 * src_stride, src_stride, pred + 0 + 8 * pred_stride, pred_stride, q2);
    hadamard_8x8_sve2(src + 8 + 8 * src_stride, src_stride, pred + 8 + 8 * pred_stride, pred_stride, q3);

    for (int i = 0; i < 8; ++i) {
        const int16x8_t b0 = vhaddq_s16(q0[i], q1[i]);
        const int16x8_t b1 = vhsubq_s16(q0[i], q1[i]);
        const int16x8_t b2 = vhaddq_s16(q2[i], q3[i]);
        const int16x8_t b3 = vhsubq_s16(q2[i], q3[i]);

        coeff[4 * i + 0] = vaddq_s16(b0, b2);
        coeff[4 * i + 1] = vaddq_s16(b1, b3);
        coeff[4 * i + 2] = vsubq_s16(b0, b2);
        coeff[4 * i + 3] = vsubq_s16(b1, b3);
    }
}

int svt_av1_hadamard_satd_32x32_sve2(const uint8_t* src, ptrdiff_t src_stride, const uint8_t* pred,
                                     ptrdiff_t pred_stride) {
    const int16x8_t one = vdupq_n_s16(1);
    int16x8_t       q0[32], q1[32], q2[32], q3[32];

    hadamard_16x16_sve2(src + 0 + 0 * src_stride, src_stride, pred + 0 + 0 * pred_stride, pred_stride, q0);
    hadamard_16x16_sve2(src + 16 + 0 * src_stride, src_stride, pred + 16 + 0 * pred_stride, pred_stride, q1);
    hadamard_16x16_sve2(src + 0 + 16 * src_stride, src_stride, pred + 0 + 16 * pred_stride, pred_stride, q2);
    hadamard_16x16_sve2(src + 16 + 16 * src_stride, src_stride, pred + 16 + 16 * pred_stride, pred_stride, q3);

    int64x2_t acc0 = vdupq_n_s64(0);
    int64x2_t acc1 = vdupq_n_s64(0);
    for (int i = 0; i < 32; ++i) {
        const int16x8_t a0 = vhaddq_s16(q0[i], q1[i]);
        const int16x8_t a1 = vhsubq_s16(q0[i], q1[i]);
        const int16x8_t a2 = vhaddq_s16(q2[i], q3[i]);
        const int16x8_t a3 = vhsubq_s16(q2[i], q3[i]);

        const int16x8_t b0 = vshrq_n_s16(a0, 1);
        const int16x8_t b1 = vshrq_n_s16(a1, 1);
        const int16x8_t b2 = vshrq_n_s16(a2, 1);
        const int16x8_t b3 = vshrq_n_s16(a3, 1);

        acc0 = svt_sdotq_s16(acc0, vmaxq_s16(vabsq_s16(b0), vabsq_s16(b2)), one);
        acc1 = svt_sdotq_s16(acc1, vmaxq_s16(vabsq_s16(b1), vabsq_s16(b3)), one);
    }

    return (int)vaddvq_s64(vaddq_s64(acc0, acc1)) << 1;
}
