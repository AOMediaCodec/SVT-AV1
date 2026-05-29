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

#ifndef SVT_AV1_HADAMARD_PATH_SVE2_H_
#define SVT_AV1_HADAMARD_PATH_SVE2_H_

#include <arm_neon.h>

#include "neon_sve_bridge.h"
#include "neon_sve2_bridge.h"

static const uint16_t kHadamard8x8CADDTbl[8] = {0, 2, 4, 6, 1, 3, 5, 7};

static inline void hadamard_8x8_cadd_pass_sve2(int16x8_t a[8]) {
    const uint16x8_t idx = vld1q_u16(kHadamard8x8CADDTbl);

    a[0] = svt_tbl_s16(svt_caddq_s16_90(a[0], a[0]), idx);
    a[1] = svt_tbl_s16(svt_caddq_s16_90(a[1], a[1]), idx);
    a[2] = svt_tbl_s16(svt_caddq_s16_90(a[2], a[2]), idx);
    a[3] = svt_tbl_s16(svt_caddq_s16_90(a[3], a[3]), idx);
    a[4] = svt_tbl_s16(svt_caddq_s16_90(a[4], a[4]), idx);
    a[5] = svt_tbl_s16(svt_caddq_s16_90(a[5], a[5]), idx);
    a[6] = svt_tbl_s16(svt_caddq_s16_90(a[6], a[6]), idx);
    a[7] = svt_tbl_s16(svt_caddq_s16_90(a[7], a[7]), idx);

    a[0] = svt_tbl_s16(svt_caddq_s16_90(a[0], a[0]), idx);
    a[1] = svt_tbl_s16(svt_caddq_s16_90(a[1], a[1]), idx);
    a[2] = svt_tbl_s16(svt_caddq_s16_90(a[2], a[2]), idx);
    a[3] = svt_tbl_s16(svt_caddq_s16_90(a[3], a[3]), idx);
    a[4] = svt_tbl_s16(svt_caddq_s16_90(a[4], a[4]), idx);
    a[5] = svt_tbl_s16(svt_caddq_s16_90(a[5], a[5]), idx);
    a[6] = svt_tbl_s16(svt_caddq_s16_90(a[6], a[6]), idx);
    a[7] = svt_tbl_s16(svt_caddq_s16_90(a[7], a[7]), idx);

    a[0] = svt_caddq_s16_90(a[0], a[0]);
    a[1] = svt_caddq_s16_90(a[1], a[1]);
    a[2] = svt_caddq_s16_90(a[2], a[2]);
    a[3] = svt_caddq_s16_90(a[3], a[3]);
    a[4] = svt_caddq_s16_90(a[4], a[4]);
    a[5] = svt_caddq_s16_90(a[5], a[5]);
    a[6] = svt_caddq_s16_90(a[6], a[6]);
    a[7] = svt_caddq_s16_90(a[7], a[7]);
}

#endif // SVT_AV1_HADAMARD_PATH_SVE2_H_
