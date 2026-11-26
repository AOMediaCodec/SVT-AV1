#!/usr/bin/env python3
#
# Copyright(c) 2026 eloy-ms.
#
# This source code is subject to the terms of the BSD 2 Clause License and
# the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
# was not distributed with this source code in the LICENSE file, you can
# obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
# Media Patent License 1.0 was not distributed with this source code in the
# PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
#
"""Generate canonical SMPTE ST 2084 (PQ) EOTF and derivative lookup tables for SVT-AV1 HDR.

Computes two 1024-entry lookup tables for 10-bit canonical PQ codewords [0..1023]:
1. svt_pq_eotf_table: absolute optical luminance L(V) in cd/m^2 (nits)
2. svt_pq_dldv_table: analytical derivative dL/dV in nits per unit normalized V

Used by SVT-AV1 Adaptive Barten Variance Boost to compute:
- Capped linear Stiles-Holladay veiling glare floor L_amb = L_0 + alpha_glare * min(L_frame, L_cap)
- Residual codeword sensitivity W_res(V) in log-QP space (without double-counting PQ)

Usage:
  python3 Build/gen_pq_eotf_table.py          # Print C tables to stdout
  python3 Build/gen_pq_eotf_table.py --write  # Directly update Source/Lib/Codec/pq_eotf_table.c
"""

import argparse
import os
import shutil
import subprocess

# SMPTE ST 2084 (PQ) Constants
M1 = 2610.0 / 16384.0
M2 = (2523.0 / 4096.0) * 128.0
C1 = 3424.0 / 4096.0
C2 = (2413.0 / 4096.0) * 32.0
C3 = (2392.0 / 4096.0) * 32.0


def pq_eotf(v: float) -> float:
    """Evaluate SMPTE ST 2084 EOTF L(v) in cd/m^2 for normalized signal v in [0.0, 1.0]."""
    if v <= 0.0:
        return 0.0
    vp = v ** (1.0 / M2)
    num = max(vp - C1, 0.0)
    return 0.0 if num == 0.0 else 10000.0 * (num / (C2 - C3 * vp)) ** (1.0 / M1)


def pq_dldv(v: float) -> float:
    """Evaluate analytical derivative dL/dv (nits per unit v) of SMPTE ST 2084 EOTF."""
    if v <= 0.0:
        return 0.0
    vp = v ** (1.0 / M2)
    num = vp - C1
    den = C2 - C3 * vp
    if num <= 0.0 or den <= 0.0:
        return 0.0
    return (
        (10000.0 / (M1 * M2))
        * ((C2 - C1 * C3) / (den * den))
        * (v ** (1.0 / M2 - 1.0))
        * ((num / den) ** (1.0 / M1 - 1.0))
    )


def generate_tables(size: int = 1024) -> tuple[list[float], list[float]]:
    """Generate 10-bit canonical codeword -> (L_pq, dL/dV) lookup tables."""
    eotf_table = []
    dldv_table = []
    for code in range(size):
        v = code / 1023.0
        eotf_table.append(pq_eotf(v))
        dldv_table.append(pq_dldv(v))
    return eotf_table, dldv_table


def format_c_table(comment: str, name: str, table: list[float]) -> str:
    """Format a lookup table as a C const double array."""
    lines = [
        f"/* {comment} */",
        f"const double {name}[{len(table)}] = {{",
    ]
    for i in range(0, len(table), 8):
        chunk = ", ".join(f"{table[j]:.6f}" for j in range(i, i + 8))
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def update_pq_eotf_source(repo_root: str, formatted_eotf: str, formatted_dldv: str):
    """Write/update svt_pq_eotf_table and svt_pq_dldv_table in Source/Lib/Codec/pq_eotf_table.c."""
    source_path = os.path.join(repo_root, "Source", "Lib", "Codec", "pq_eotf_table.c")
    content = f"""/*
* Copyright(c) 2026 eloy-ms.
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/

#include "pq_eotf_table.h"

{formatted_eotf}

{formatted_dldv}
"""
    with open(source_path, "w", encoding="utf-8") as f:
        f.write(content)
    clang_fmt = shutil.which("clang-format")
    if clang_fmt:
        subprocess.run([clang_fmt, "-i", source_path], check=False)
    print(f"Updated {source_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write",
        action="store_true",
        help="Write the generated tables directly into Source/Lib/Codec/pq_eotf_table.c",
    )
    args = parser.parse_args()

    eotf_table, dldv_table = generate_tables(1024)
    formatted_eotf = format_c_table(
        "Canonical SMPTE ST 2084 (PQ) EOTF lookup table mapping 10-bit codeword [0..1023] to cd/m^2 (nits)",
        "svt_pq_eotf_table",
        eotf_table,
    )
    formatted_dldv = format_c_table(
        "Analytical SMPTE ST 2084 derivative dL/dV (nits per unit normalized V) for 10-bit codeword [0..1023]",
        "svt_pq_dldv_table",
        dldv_table,
    )

    if args.write:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.normpath(os.path.join(script_dir, ".."))
        update_pq_eotf_source(repo_root, formatted_eotf, formatted_dldv)
    else:
        print(formatted_eotf)
        print()
        print(formatted_dldv)


if __name__ == "__main__":
    main()
