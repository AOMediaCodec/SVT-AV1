/*
 * Copyright (c) 2026, Alliance for Open Media. All rights reserved.
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef SVT_AV1_HADAMARD_PATH_NEON_H_
#define SVT_AV1_HADAMARD_PATH_NEON_H_

#include <arm_neon.h>

static inline void hadamard_8x8_one_pass(int16x8_t* a) {
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

    a[0] = vaddq_s16(c0, c4);
    a[1] = vsubq_s16(c2, c6);
    a[2] = vsubq_s16(c0, c4);
    a[3] = vaddq_s16(c2, c6);
    a[4] = vaddq_s16(c3, c7);
    a[5] = vsubq_s16(c3, c7);
    a[6] = vsubq_s16(c1, c5);
    a[7] = vaddq_s16(c1, c5);
}

#endif // SVT_AV1_HADAMARD_PATH_NEON_H_
