/*
* Copyright(c) 2026 eloy-ms.
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/

#ifndef _PQ_EOTF_TABLE_H_
#define _PQ_EOTF_TABLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical SMPTE ST 2084 (PQ) EOTF lookup table mapping 10-bit codeword [0..1023] to cd/m^2 (nits) */
extern const double svt_pq_eotf_table[1024];

/* Analytical SMPTE ST 2084 derivative dL/dV (nits per unit normalized V) for 10-bit codeword [0..1023] */
extern const double svt_pq_dldv_table[1024];

#ifdef __cplusplus
}
#endif

#endif // _PQ_EOTF_TABLE_H_
