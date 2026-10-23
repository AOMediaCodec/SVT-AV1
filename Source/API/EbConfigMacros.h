/*
* Copyright(c) 2025 Meta Platforms, Inc. and affiliates.
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/

/*
* This file contains configuration macros that control which parts of code are used
* Macros could be fed via command line, so all macros here must check if they are
* already defined!
* All macros must have the following format:
* - all macros must be prefixed with CONFIG_
*/

#ifndef EbConfigMacros_h
#define EbConfigMacros_h

// clang-format off

#ifndef RTC_BUILD
#define RTC_BUILD 0
#endif

#ifndef MINIMAL_BUILD
#define MINIMAL_BUILD 0
#endif

#ifndef MOBILE_BUILD
#define MOBILE_BUILD 0
#endif

#if RTC_BUILD && MOBILE_BUILD
#error "RTC_BUILD and MOBILE_BUILD are mutually exclusive"
#endif

#if RTC_BUILD

#if MINIMAL_BUILD
#define MIN_ENC_PRESET                      ENC_M9

#ifndef CONFIG_LOG_QUIET
#define CONFIG_LOG_QUIET                    1
#endif

// Below features are disabled based on 2 assumptions:
// 1. 8-bit input/processing only
// 2. MIN_ENC_PRESET=ENC_M9

#ifndef CONFIG_ENABLE_QUANT_MATRIX
#define CONFIG_ENABLE_QUANT_MATRIX          0
#endif
#ifndef CONFIG_ENABLE_OBMC
#define CONFIG_ENABLE_OBMC                  0
#endif
#ifndef CONFIG_ENABLE_FILM_GRAIN
#define CONFIG_ENABLE_FILM_GRAIN            0
#endif
#ifndef CONFIG_ENABLE_HIGH_BIT_DEPTH
#define CONFIG_ENABLE_HIGH_BIT_DEPTH        0
#endif
#ifndef CONFIG_ENABLE_RESTORATION
#define CONFIG_ENABLE_RESTORATION           0
#endif
#ifndef CONFIG_ENABLE_GLOBAL_MOTION
#define CONFIG_ENABLE_GLOBAL_MOTION         0
#endif
#ifndef CONFIG_ENABLE_INTER_COMPOUND
#define CONFIG_ENABLE_INTER_COMPOUND        0
#endif
#ifndef CONFIG_ENABLE_INTER_INTRA
#define CONFIG_ENABLE_INTER_INTRA           0
#endif
#ifndef CONFIG_ENABLE_RANDOM_ACCESS
#define CONFIG_ENABLE_RANDOM_ACCESS         0
#endif
#ifndef CONFIG_ENABLE_FULL_SUBPEL
#define CONFIG_ENABLE_FULL_SUBPEL           0
#endif
#ifndef CONFIG_ENABLE_FILTER_INTRA
#define CONFIG_ENABLE_FILTER_INTRA          0
#endif
#ifndef CONFIG_ENABLE_SUPERRES
#define CONFIG_ENABLE_SUPERRES              0
#endif
#ifndef CONFIG_ENABLE_PALETTE
#define CONFIG_ENABLE_PALETTE               0
#endif
#ifndef CONFIG_ENABLE_INTRA_BC
#define CONFIG_ENABLE_INTRA_BC              0
#endif
#ifndef CONFIG_ENABLE_LOSSLESS
#define CONFIG_ENABLE_LOSSLESS              0
#endif
#ifndef CONFIG_ENABLE_TEMPORAL_FILTERING
#define CONFIG_ENABLE_TEMPORAL_FILTERING    0
#endif
#ifndef CONFIG_ENABLE_WARP
#define CONFIG_ENABLE_WARP                  0
#endif
#ifndef CONFIG_ENABLE_TPL
#define CONFIG_ENABLE_TPL                   0
#endif
#ifndef CONFIG_ENABLE_VMAF
#define CONFIG_ENABLE_VMAF                  0
#endif
#ifndef CONFIG_ENABLE_RESIZE
#define CONFIG_ENABLE_RESIZE                0
#endif
#ifndef CONFIG_ENABLE_TX_PF_N2
#define CONFIG_ENABLE_TX_PF_N2              0
#endif
#ifndef CONFIG_ENABLE_NON_DCT_LARGE_TX
#define CONFIG_ENABLE_NON_DCT_LARGE_TX      0
#endif
#ifndef CONFIG_ENABLE_MD_CDF_UPDATE
#define CONFIG_ENABLE_MD_CDF_UPDATE         0
#endif
#ifndef CONFIG_ENABLE_DLF_SEARCH
#define CONFIG_ENABLE_DLF_SEARCH            0
#endif
#ifndef CONFIG_RTC_SB64
#define CONFIG_RTC_SB64                     1
#endif
#endif // MINIMAL_BUILD

#endif

#if MOBILE_BUILD
// Mobile build: one library for RTC (M9+), VOD (M9+) and AVIF still images (M6+).
#define MIN_ENC_PRESET                      ENC_M6

#ifndef CONFIG_LOG_QUIET
#define CONFIG_LOG_QUIET                    1
#endif

// Below features are off because no in-scope encode uses them (8-bit; RTC M9-M13, VOD M9-M11 and AVIF M6-M9, as
// higher VOD/AVIF presets map to these; no film grain, superres, resize, QM or VMAF; natural content, so no
// palette or IntraBC), or, where commented, because no preset >= M6 reaches them.
// Local warp (CONFIG_ENABLE_WARP) stays on: VOD with the default hierarchy uses it at every in-scope preset.
// Lossless coding (CONFIG_ENABLE_LOSSLESS) stays on for VOD and AVIF: its 4x4 inverse WHT runs on dav1d's kernel
// (av1_inv_txfm_neon.c), and RTC mode rejects it (svt_av1_verify_settings()).

#ifndef CONFIG_ENABLE_QUANT_MATRIX
#define CONFIG_ENABLE_QUANT_MATRIX          0
#endif
#ifndef CONFIG_ENABLE_OBMC
#define CONFIG_ENABLE_OBMC                  0
#endif
#ifndef CONFIG_ENABLE_FILM_GRAIN
#define CONFIG_ENABLE_FILM_GRAIN            0
#endif
#ifndef CONFIG_ENABLE_HIGH_BIT_DEPTH
#define CONFIG_ENABLE_HIGH_BIT_DEPTH        0
#endif
#ifndef CONFIG_ENABLE_GLOBAL_MOTION
#define CONFIG_ENABLE_GLOBAL_MOTION         0
#endif
#ifndef CONFIG_ENABLE_INTER_INTRA
#define CONFIG_ENABLE_INTER_INTRA           0
#endif
#ifndef CONFIG_ENABLE_FULL_SUBPEL
#define CONFIG_ENABLE_FULL_SUBPEL           0
#endif
#ifndef CONFIG_ENABLE_SUPERRES
#define CONFIG_ENABLE_SUPERRES              0
#endif
#ifndef CONFIG_ENABLE_PALETTE
#define CONFIG_ENABLE_PALETTE               0
#endif
#ifndef CONFIG_ENABLE_INTRA_BC
#define CONFIG_ENABLE_INTRA_BC              0
#endif
#ifndef CONFIG_ENABLE_VMAF
#define CONFIG_ENABLE_VMAF                  0
#endif
#ifndef CONFIG_ENABLE_RESIZE
#define CONFIG_ENABLE_RESIZE                0
#endif
#ifndef CONFIG_ENABLE_DLF_SEARCH
#define CONFIG_ENABLE_DLF_SEARCH            0
#endif
// Loop restoration serves only AVIF M6 in scope: turning it off costs some BD-rate there but saves binary size.
#ifndef CONFIG_ENABLE_RESTORATION
#define CONFIG_ENABLE_RESTORATION           0
#endif
// The ext-tx sets allow no non-DCT type at 64-point sizes and only IDTX at 32-point sizes: the C forward
// fallbacks for the other types are dead, and the inverse codes IDTX with dav1d kernels (av1_inv_txfm_neon.c).
#ifndef CONFIG_ENABLE_NON_DCT_LARGE_TX
#define CONFIG_ENABLE_NON_DCT_LARGE_TX      0
#endif
// MD SQ MV search and the multi-position PSAD kernel (svt_pme_sad_loop_kernel): md_sq_mv_search_level is 0
// everywhere, NSQ MV search and PME levels 1-3 are VAR-based, and PME levels 4/5 search at most 7 columns.
#ifndef CONFIG_ENABLE_PSAD_SEARCH
#define CONFIG_ENABLE_PSAD_SEARCH           0
#endif
// 1:4 partitions need nsq_geom_level 1/2 (<= M5 default, <= M3 all-intra, never RTC): the vertical 1:4 transforms
// (4x16, 8x32, 16x64) are unused. The horizontal ones stay: subres and TPL code W x H/4 transforms.
#ifndef CONFIG_ENABLE_HV4
#define CONFIG_ENABLE_HV4                   0
#endif
// Compound types other than AVG: get_inter_compound_level() is 0 above M2, so MD searches MD_COMP_AVG only and
// the sequence header disables jnt/masked compound.
#ifndef CONFIG_ENABLE_COMPOUND_TYPES
#define CONFIG_ENABLE_COMPOUND_TYPES        0
#endif
// Partial-frequency transforms beyond TPL's: MD uses the full transform (pf_level 1) or N4 (full-loop override),
// and TPL codes N2 only at 16x16, 16x4 and 32x8 (dispenser_search_level 0/1, subsample_tx never 1). Other N2 sizes
// and DC-only are unused.
#ifndef CONFIG_ENABLE_TX_PF_ALL
#define CONFIG_ENABLE_TX_PF_ALL             0
#endif
// TPL params level 1 (<= M2) features: rate estimation, the full transform, directional intra (edge filter) and
// subpel search. Levels 4/5 (every preset > M2) use compute_rate 0, N2/N4, DC-only intra and FULL_PEL.
#ifndef CONFIG_ENABLE_TPL_HQ
#define CONFIG_ENABLE_TPL_HQ                0
#endif
// New/nearest/near combination, unipred 3x3 and bipred 3x3 MD injection are enabled only at <= M1.
#ifndef CONFIG_ENABLE_MD_MR_INJECTION
#define CONFIG_ENABLE_MD_MR_INJECTION       0
#endif
// derive_tf_params forces TF level 0 for every low-delay encode, so the zz-based filter that only the low-delay
// TF levels select (use_zz_based_filter) is never used.
#ifndef CONFIG_ENABLE_TF_ZZ_FILTER
#define CONFIG_ENABLE_TF_ZZ_FILTER          0
#endif
// Every preset >= M6 selects 64x64 superblocks (enc_handle.c), so the 128-wide kernels are never dispatched.
#ifndef CONFIG_RTC_SB64
#define CONFIG_RTC_SB64                     1
#endif
#endif // MOBILE_BUILD

#ifndef MIN_ENC_PRESET
#define MIN_ENC_PRESET                      ENC_MR
#endif

// When set to 1, EB_CPU_FLAGS_NEON is unconditionally set for all ARCH_AARCH64
// builds, i.e. requiring Neon for library to work. This also allows linker to
// strip code for all C functions which are optimized with Neon SIMD and thus
// reduce final binary size.
// Neon is mandatory in Armv8.0-A (AArch64), which is our minimum Arm target,
// so it is guaranteed for deployment builds, however tests use C functions,
// and hence for unit-test builds (SVT_AV1_UNIT_TEST_BUILD, set by CMake when
// BUILD_TESTING is ON) this must stay at 0.
#if defined(ARCH_AARCH64) && !defined(SVT_AV1_UNIT_TEST_BUILD)
#define CONFIG_ARM_NEON_IS_GUARANTEED       1
#endif

#ifndef CONFIG_ARM_NEON_IS_GUARANTEED
#define CONFIG_ARM_NEON_IS_GUARANTEED       0
#endif

// Same for x86 builds and AVX2 as minimum required SIMD level.
// AVX2 was first released in 2013 on Haswell microarchitecture, all x86
// processors since support it.
// You can set it to 1 to reduce binary size if deployment platforms are
// guaranteed to be not older than Haswell.
#if 0
#define CONFIG_X86_AVX2_IS_GUARANTEED       0
#endif

#ifndef CONFIG_X86_AVX2_IS_GUARANTEED
#define CONFIG_X86_AVX2_IS_GUARANTEED       0
#endif

#ifndef CONFIG_LOG_QUIET
#define CONFIG_LOG_QUIET                    0
#endif

#ifndef CONFIG_ENABLE_QUANT_MATRIX
#define CONFIG_ENABLE_QUANT_MATRIX          1
#endif

#ifndef CONFIG_ENABLE_OBMC
#define CONFIG_ENABLE_OBMC                  1
#endif

#ifndef CONFIG_ENABLE_FILM_GRAIN
#define CONFIG_ENABLE_FILM_GRAIN            1
#endif

#ifndef CONFIG_ENABLE_HIGH_BIT_DEPTH
#define CONFIG_ENABLE_HIGH_BIT_DEPTH        1
#endif

#ifndef CONFIG_ENABLE_RESTORATION
#define CONFIG_ENABLE_RESTORATION           1
#endif

#ifndef CONFIG_ENABLE_PSAD_SEARCH
#define CONFIG_ENABLE_PSAD_SEARCH           1
#endif

#ifndef CONFIG_ENABLE_HV4
#define CONFIG_ENABLE_HV4                   1
#endif

#ifndef CONFIG_ENABLE_COMPOUND_TYPES
#define CONFIG_ENABLE_COMPOUND_TYPES        1
#endif

#ifndef CONFIG_ENABLE_TX_PF_ALL
#define CONFIG_ENABLE_TX_PF_ALL             1
#endif

#ifndef CONFIG_ENABLE_TPL_HQ
#define CONFIG_ENABLE_TPL_HQ                1
#endif

#ifndef CONFIG_ENABLE_MD_MR_INJECTION
#define CONFIG_ENABLE_MD_MR_INJECTION       1
#endif

#ifndef CONFIG_ENABLE_TF_ZZ_FILTER
#define CONFIG_ENABLE_TF_ZZ_FILTER          1
#endif

#ifndef CONFIG_ENABLE_GLOBAL_MOTION
#define CONFIG_ENABLE_GLOBAL_MOTION         1
#endif

#ifndef CONFIG_ENABLE_INTER_COMPOUND
#define CONFIG_ENABLE_INTER_COMPOUND        1
#endif

#ifndef CONFIG_ENABLE_INTER_INTRA
#define CONFIG_ENABLE_INTER_INTRA           1
#endif

#ifndef CONFIG_ENABLE_RANDOM_ACCESS
#define CONFIG_ENABLE_RANDOM_ACCESS         1
#endif

#ifndef CONFIG_ENABLE_FULL_SUBPEL
#define CONFIG_ENABLE_FULL_SUBPEL           1
#endif

#ifndef CONFIG_ENABLE_FILTER_INTRA
#define CONFIG_ENABLE_FILTER_INTRA          1
#endif

#ifndef CONFIG_ENABLE_SUPERRES
#define CONFIG_ENABLE_SUPERRES              1
#endif

#ifndef CONFIG_ENABLE_PALETTE
#define CONFIG_ENABLE_PALETTE               1
#endif
#ifndef CONFIG_ENABLE_INTRA_BC
#define CONFIG_ENABLE_INTRA_BC              1
#endif

#ifndef CONFIG_ENABLE_LOSSLESS
#define CONFIG_ENABLE_LOSSLESS             1
#endif

#ifndef CONFIG_ENABLE_TEMPORAL_FILTERING
#define CONFIG_ENABLE_TEMPORAL_FILTERING   1
#endif

#ifndef CONFIG_ENABLE_WARP
#define CONFIG_ENABLE_WARP                 1
#endif

#ifndef CONFIG_ENABLE_TPL
#define CONFIG_ENABLE_TPL                  1
#endif

#ifndef CONFIG_ENABLE_VMAF
#define CONFIG_ENABLE_VMAF                 1
#endif

#ifndef CONFIG_ENABLE_RESIZE
#define CONFIG_ENABLE_RESIZE               1
#endif

#ifndef CONFIG_ENABLE_TX_PF_N2
#define CONFIG_ENABLE_TX_PF_N2             1
#endif

#ifndef CONFIG_ENABLE_NON_DCT_LARGE_TX
#define CONFIG_ENABLE_NON_DCT_LARGE_TX     1
#endif

#ifndef CONFIG_ENABLE_MD_CDF_UPDATE
#define CONFIG_ENABLE_MD_CDF_UPDATE        1
#endif

#ifndef CONFIG_ENABLE_DLF_SEARCH
#define CONFIG_ENABLE_DLF_SEARCH          1
#endif

// RTC forces super_block_size = 64 (enc_handle.c), so 128-wide blocks never occur and the 128-wide
// variance / sub-pel variance / SAD kernels are never dispatched. When set, their RTCD registration
// is dropped and LTO strips them. Bit-exact at SB=64.
#ifndef CONFIG_RTC_SB64
#define CONFIG_RTC_SB64                   0
#endif

// Fast (non-bit-exact) all-int16 forward transforms for the LBD path: every
// two-product cospi butterfly uses per-product vqrdmulhq_s16 instead of the
// widening multiply. ~3x fewer instructions; tiny rounding error vs the
// bit-exact int16 path. Guarded so a build picks exactly one path.
#ifndef CONFIG_ENABLE_FAST_LBD_TXFM
#define CONFIG_ENABLE_FAST_LBD_TXFM         0
#endif

// Single-thread kernel dispatch: at lp=1, bypass thread creation and run all
// pipeline kernels cooperatively on one thread. Eliminates 15 context switches
// per frame and all inter-stage semaphore/mutex overhead.
#ifndef CONFIG_SINGLE_THREAD_KERNEL
#define CONFIG_SINGLE_THREAD_KERNEL         1
#endif

// Native 8-bit CDEF path (interior blocks in uint8 lanes). NEON, and x86 from
// SSE4.1 up.
#if defined(ARCH_AARCH64) || defined(ARCH_X86_64)
#define CDEF_8BITS_PATH 1
#else
#define CDEF_8BITS_PATH 0
#endif

// When high-bit-depth (10/12-bit) support is compiled out, fold the effective encoder bit depth to
// the compile-time constant EB_EIGHT_BIT so that `bit_depth > EB_EIGHT_BIT` (and derived 16-bit)
// branches become dead and are eliminated by the optimizer -- no per-site #if guards, no empty ifs.
// SVT_EFFECTIVE_* read the effective value in an expression; SVT_FOLD_* pin a local to it in place
// (a no-op when HBD is enabled, so it never becomes a self-assignment).
#if CONFIG_ENABLE_HIGH_BIT_DEPTH
#define SVT_EFFECTIVE_BIT_DEPTH(bit_depth) (bit_depth)
#define SVT_EFFECTIVE_IS_16BIT_PIPELINE(v) (v)
#define SVT_EFFECTIVE_HBD_MD(v)            (v)
#define SVT_FOLD_HBD_MD(hbd_md)            ((void)0)
#define SVT_FOLD_BIT_DEPTH(bit_depth)      ((void)0)
#else
// The dead ternary branch keeps the argument "used" (avoids -Wunused on locals that only feed this
// macro) while still folding to a compile-time constant so branches are eliminated by the optimizer.
#define SVT_EFFECTIVE_BIT_DEPTH(bit_depth) (0 ? (bit_depth) : EB_EIGHT_BIT)
#define SVT_EFFECTIVE_IS_16BIT_PIPELINE(v) (0 ? (v) : 0)
#define SVT_EFFECTIVE_HBD_MD(v)            (0 ? (v) : 0)
#define SVT_FOLD_HBD_MD(hbd_md)            ((hbd_md) = 0)
#define SVT_FOLD_BIT_DEPTH(bit_depth)      ((bit_depth) = EB_EIGHT_BIT)
#endif

// clang-format on

// RTC-minimal is always rtc-tuned and never all-intra; fold these to compile-time constants so the
// _default/_allintra signal-derivation variants dead-code-eliminate.
#define SVT_RTC_TUNE(scs) (RTC_BUILD ? true : (scs)->static_config.rtc)
#define SVT_ALLINTRA(scs) (RTC_BUILD ? false : (scs)->allintra)

// RTC and MOBILE builds run only enc_mode >= M6 (RTC: >= M7), which makes some paths dead; svt_av1_verify_settings()
// rejects the options that would reach them. A plain 0/1 constant (rather than `RTC_BUILD || MOBILE_BUILD` at the
// use site) keeps each guarded expression the same shape in every build, so a PGO profile collected from one build
// keeps matching the functions that use it in another.
#if RTC_BUILD || MOBILE_BUILD
#define SVT_RTC_OR_MOBILE_BUILD 1
#else
#define SVT_RTC_OR_MOBILE_BUILD 0
#endif

#endif // EbConfigMacros_h
