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

#if CONFIG_ENABLE_HIGH_BIT_DEPTH
static inline void highbd_hadamard_8x8_sve2(const uint16_t* src, ptrdiff_t src_stride, const uint16_t* pred,
                                            ptrdiff_t pred_stride, int32x4_t coeff[16]) {
    uint16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t p0, p1, p2, p3, p4, p5, p6, p7;

    load_u16_8x8(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);
    load_u16_8x8(pred, pred_stride, &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7);

    int16x8_t a[8];
    a[0] = vreinterpretq_s16_u16(vsubq_u16(s0, p0));
    a[1] = vreinterpretq_s16_u16(vsubq_u16(s1, p1));
    a[2] = vreinterpretq_s16_u16(vsubq_u16(s2, p2));
    a[3] = vreinterpretq_s16_u16(vsubq_u16(s3, p3));
    a[4] = vreinterpretq_s16_u16(vsubq_u16(s4, p4));
    a[5] = vreinterpretq_s16_u16(vsubq_u16(s5, p5));
    a[6] = vreinterpretq_s16_u16(vsubq_u16(s6, p6));
    a[7] = vreinterpretq_s16_u16(vsubq_u16(s7, p7));

    hadamard_8x8_cadd_pass_sve2(a);

    const int16x8_t b0 = vaddq_s16(a[0], a[1]);
    const int16x8_t b1 = vsubq_s16(a[0], a[1]);
    const int16x8_t b2 = vaddq_s16(a[2], a[3]);
    const int16x8_t b3 = vsubq_s16(a[2], a[3]);
    const int16x8_t b4 = vaddq_s16(a[4], a[5]);
    const int16x8_t b5 = vsubq_s16(a[4], a[5]);
    const int16x8_t b6 = vaddq_s16(a[6], a[7]);
    const int16x8_t b7 = vsubq_s16(a[6], a[7]);

    const int16x8_t c0 = vaddq_s16(b0, b2);
    const int16x8_t c1 = vaddq_s16(b1, b3);
    const int16x8_t c2 = vsubq_s16(b0, b2);
    const int16x8_t c3 = vsubq_s16(b1, b3);
    const int16x8_t c4 = vaddq_s16(b4, b6);
    const int16x8_t c5 = vaddq_s16(b5, b7);
    const int16x8_t c6 = vsubq_s16(b4, b6);
    const int16x8_t c7 = vsubq_s16(b5, b7);

    coeff[0] = vaddl_s16(vget_low_s16(c0), vget_low_s16(c4));
    coeff[1] = vsubl_s16(vget_low_s16(c2), vget_low_s16(c6));
    coeff[2] = vsubl_s16(vget_low_s16(c0), vget_low_s16(c4));
    coeff[3] = vaddl_s16(vget_low_s16(c2), vget_low_s16(c6));
    coeff[4] = vaddl_s16(vget_low_s16(c3), vget_low_s16(c7));
    coeff[5] = vsubl_s16(vget_low_s16(c3), vget_low_s16(c7));
    coeff[6] = vsubl_s16(vget_low_s16(c1), vget_low_s16(c5));
    coeff[7] = vaddl_s16(vget_low_s16(c1), vget_low_s16(c5));

    coeff[8]  = vaddl_s16(vget_high_s16(c0), vget_high_s16(c4));
    coeff[9]  = vsubl_s16(vget_high_s16(c2), vget_high_s16(c6));
    coeff[10] = vsubl_s16(vget_high_s16(c0), vget_high_s16(c4));
    coeff[11] = vaddl_s16(vget_high_s16(c2), vget_high_s16(c6));
    coeff[12] = vaddl_s16(vget_high_s16(c3), vget_high_s16(c7));
    coeff[13] = vsubl_s16(vget_high_s16(c3), vget_high_s16(c7));
    coeff[14] = vsubl_s16(vget_high_s16(c1), vget_high_s16(c5));
    coeff[15] = vaddl_s16(vget_high_s16(c1), vget_high_s16(c5));
}

int svt_av1_highbd_hadamard_satd_8x8_sve2(const uint16_t* src, ptrdiff_t src_stride, const uint16_t* pred,
                                          ptrdiff_t pred_stride) {
    uint16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t p0, p1, p2, p3, p4, p5, p6, p7;

    load_u16_8x8(src, src_stride, &s0, &s1, &s2, &s3, &s4, &s5, &s6, &s7);
    load_u16_8x8(pred, pred_stride, &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7);

    int16x8_t a[8];
    a[0] = vreinterpretq_s16_u16(vsubq_u16(s0, p0));
    a[1] = vreinterpretq_s16_u16(vsubq_u16(s1, p1));
    a[2] = vreinterpretq_s16_u16(vsubq_u16(s2, p2));
    a[3] = vreinterpretq_s16_u16(vsubq_u16(s3, p3));
    a[4] = vreinterpretq_s16_u16(vsubq_u16(s4, p4));
    a[5] = vreinterpretq_s16_u16(vsubq_u16(s5, p5));
    a[6] = vreinterpretq_s16_u16(vsubq_u16(s6, p6));
    a[7] = vreinterpretq_s16_u16(vsubq_u16(s7, p7));

    hadamard_8x8_cadd_pass_sve2(a);

    const int16x8_t b0 = vaddq_s16(a[0], a[1]);
    const int16x8_t b1 = vsubq_s16(a[0], a[1]);
    const int16x8_t b2 = vaddq_s16(a[2], a[3]);
    const int16x8_t b3 = vsubq_s16(a[2], a[3]);
    const int16x8_t b4 = vaddq_s16(a[4], a[5]);
    const int16x8_t b5 = vsubq_s16(a[4], a[5]);
    const int16x8_t b6 = vaddq_s16(a[6], a[7]);
    const int16x8_t b7 = vsubq_s16(a[6], a[7]);

    a[0] = vabsq_s16(vaddq_s16(b0, b2));
    a[1] = vabsq_s16(vaddq_s16(b1, b3));
    a[2] = vabdq_s16(b1, b3);
    a[3] = vabdq_s16(b0, b2);
    a[4] = vabsq_s16(vaddq_s16(b4, b6));
    a[5] = vabsq_s16(vaddq_s16(b5, b7));
    a[6] = vabdq_s16(b5, b7);
    a[7] = vabdq_s16(b4, b6);

    int16x8_t max[4];
    max[0] = vmaxq_s16(a[0], a[4]);
    max[1] = vmaxq_s16(a[1], a[5]);
    max[2] = vmaxq_s16(a[2], a[6]);
    max[3] = vmaxq_s16(a[3], a[7]);

    return vaddlvq_s32(horizontal_add_4d_s16x8(max)) << 1;
}

int svt_av1_highbd_hadamard_satd_16x16_sve2(const uint16_t* src, ptrdiff_t src_stride, const uint16_t* pred,
                                            ptrdiff_t pred_stride) {
    int32x4_t q0[16], q1[16], q2[16], q3[16];

    highbd_hadamard_8x8_sve2(src + 0 + 0 * src_stride, src_stride, pred + 0 + 0 * pred_stride, pred_stride, q0);
    highbd_hadamard_8x8_sve2(src + 8 + 0 * src_stride, src_stride, pred + 8 + 0 * pred_stride, pred_stride, q1);
    highbd_hadamard_8x8_sve2(src + 0 + 8 * src_stride, src_stride, pred + 0 + 8 * pred_stride, pred_stride, q2);
    highbd_hadamard_8x8_sve2(src + 8 + 8 * src_stride, src_stride, pred + 8 + 8 * pred_stride, pred_stride, q3);

    int32x4_t acc0 = vdupq_n_s32(0);
    int32x4_t acc1 = vdupq_n_s32(0);
    for (int i = 0; i < 16; ++i) {
        const int32x4_t a0 = vabsq_s32(vhaddq_s32(q0[i], q1[i]));
        const int32x4_t a1 = vabsq_s32(vhsubq_s32(q0[i], q1[i]));
        const int32x4_t a2 = vabsq_s32(vhaddq_s32(q2[i], q3[i]));
        const int32x4_t a3 = vabsq_s32(vhsubq_s32(q2[i], q3[i]));

        acc0 = vaddq_s32(acc0, vmaxq_s32(a0, a2));
        acc1 = vaddq_s32(acc1, vmaxq_s32(a1, a3));
    }

    return vaddvq_s32(vaddq_s32(acc0, acc1)) << 1;
}

static inline void highbd_hadamard_16x16_sve2(const uint16_t* src, ptrdiff_t src_stride, const uint16_t* pred,
                                              ptrdiff_t pred_stride, int32x4_t coeff[64]) {
    int32x4_t q0[16], q1[16], q2[16], q3[16];

    highbd_hadamard_8x8_sve2(src + 0 + 0 * src_stride, src_stride, pred + 0 + 0 * pred_stride, pred_stride, q0);
    highbd_hadamard_8x8_sve2(src + 8 + 0 * src_stride, src_stride, pred + 8 + 0 * pred_stride, pred_stride, q1);
    highbd_hadamard_8x8_sve2(src + 0 + 8 * src_stride, src_stride, pred + 0 + 8 * pred_stride, pred_stride, q2);
    highbd_hadamard_8x8_sve2(src + 8 + 8 * src_stride, src_stride, pred + 8 + 8 * pred_stride, pred_stride, q3);

    for (int i = 0; i < 16; ++i) {
        const int32x4_t b0 = vhaddq_s32(q0[i], q1[i]);
        const int32x4_t b1 = vhsubq_s32(q0[i], q1[i]);
        const int32x4_t b2 = vhaddq_s32(q2[i], q3[i]);
        const int32x4_t b3 = vhsubq_s32(q2[i], q3[i]);

        coeff[4 * i + 0] = vaddq_s32(b0, b2);
        coeff[4 * i + 1] = vaddq_s32(b1, b3);
        coeff[4 * i + 2] = vsubq_s32(b0, b2);
        coeff[4 * i + 3] = vsubq_s32(b1, b3);
    }
}

int svt_av1_highbd_hadamard_satd_32x32_sve2(const uint16_t* src, ptrdiff_t src_stride, const uint16_t* pred,
                                            ptrdiff_t pred_stride) {
    int32x4_t q0[64], q1[64], q2[64], q3[64];

    highbd_hadamard_16x16_sve2(src + 0 + 0 * src_stride, src_stride, pred + 0 + 0 * pred_stride, pred_stride, q0);
    highbd_hadamard_16x16_sve2(src + 16 + 0 * src_stride, src_stride, pred + 16 + 0 * pred_stride, pred_stride, q1);
    highbd_hadamard_16x16_sve2(src + 0 + 16 * src_stride, src_stride, pred + 0 + 16 * pred_stride, pred_stride, q2);
    highbd_hadamard_16x16_sve2(src + 16 + 16 * src_stride, src_stride, pred + 16 + 16 * pred_stride, pred_stride, q3);

    int32x4_t acc0 = vdupq_n_s32(0);
    int32x4_t acc1 = vdupq_n_s32(0);
    for (int i = 0; i < 64; ++i) {
        const int32x4_t a0 = vabsq_s32(vshrq_n_s32(vaddq_s32(q0[i], q1[i]), 2));
        const int32x4_t a1 = vabsq_s32(vshrq_n_s32(vsubq_s32(q0[i], q1[i]), 2));
        const int32x4_t a2 = vabsq_s32(vshrq_n_s32(vaddq_s32(q2[i], q3[i]), 2));
        const int32x4_t a3 = vabsq_s32(vshrq_n_s32(vsubq_s32(q2[i], q3[i]), 2));

        acc0 = vaddq_s32(acc0, vmaxq_s32(a0, a2));
        acc1 = vaddq_s32(acc1, vmaxq_s32(a1, a3));
    }

    return vaddvq_s32(vaddq_s32(acc0, acc1)) << 1;
}
#endif
