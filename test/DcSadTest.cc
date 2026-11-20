/*
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at https://www.aomedia.org/license/software-license. If the
 * Alliance for Open Media Patent License 1.0 was not distributed with this
 * source code in the PATENTS file, you can obtain it at
 * https://www.aomedia.org/license/patent-license.
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "aom_dsp_rtcd.h"
#include "common_dsp_rtcd.h"
#include "compute_sad_c.h"
#include "random.h"

namespace {
using DcSadFunc = uint32_t (*)(const uint8_t *, uint32_t, const uint8_t *,
                               const uint8_t *, uint32_t, bool, bool);
using DcPredFunc = void (*)(uint8_t *, ptrdiff_t, const uint8_t *,
                            const uint8_t *);
using DcSadParam = std::tuple<DcSadFunc, uint32_t, bool, bool>;

const DcPredFunc predictors[3][2][2] = {
    {{svt_aom_dc_128_predictor_16x16_c, svt_aom_dc_top_predictor_16x16_c},
     {svt_aom_dc_left_predictor_16x16_c, svt_aom_dc_predictor_16x16_c}},
    {{svt_aom_dc_128_predictor_32x32_c, svt_aom_dc_top_predictor_32x32_c},
     {svt_aom_dc_left_predictor_32x32_c, svt_aom_dc_predictor_32x32_c}},
    {{svt_aom_dc_128_predictor_64x64_c, svt_aom_dc_top_predictor_64x64_c},
     {svt_aom_dc_left_predictor_64x64_c, svt_aom_dc_predictor_64x64_c}}};

class DcSadTest : public ::testing::TestWithParam<DcSadParam> {
  protected:
    void SetUp() override {
        std::tie(func_, size_, have_top_, have_left_) = GetParam();
    }

    void check(const uint8_t *src, uint32_t stride, const uint8_t *above,
               const uint8_t *left) {
        above = have_top_ ? above : nullptr;
        left = have_left_ ? left : nullptr;
        std::array<uint8_t, 64 * 64> pred;
        const int index = size_ == 16 ? 0 : size_ == 32 ? 1 : 2;
        predictors[index][have_left_][have_top_](
            pred.data(), size_, above, left);
        const uint32_t expected = svt_nxm_sad_kernel_helper_c(
            src, stride, pred.data(), size_, size_, size_);
        ASSERT_EQ(
            expected,
            func_(src, stride, above, left, size_, have_top_, have_left_));
    }

    DcSadFunc func_;
    uint32_t size_;
    bool have_top_;
    bool have_left_;
};

TEST_P(DcSadTest, RandomMatch) {
    svt_av1_test_tool::SVTRandom rnd(0, 255);
    for (uint32_t stride : {size_, size_ + 3, size_ * 2 + 1}) {
        for (uint32_t offset = 0; offset < 32; ++offset) {
            std::vector<uint8_t> src(offset + stride * (size_ - 1) + size_);
            std::vector<uint8_t> above(offset + size_);
            std::vector<uint8_t> left(offset + size_);
            for (int iteration = 0; iteration < 10; ++iteration) {
                for (auto *data : {&src, &above, &left}) {
                    std::generate(data->begin(), data->end(), [&rnd]() {
                        return static_cast<uint8_t>(rnd.random());
                    });
                }
                check(src.data() + offset,
                      stride,
                      above.data() + offset,
                      left.data() + offset);
                if (HasFatalFailure())
                    return;
            }
        }
    }
}

TEST_P(DcSadTest, ExtremeMatch) {
    std::vector<uint8_t> src(size_ * size_);
    std::vector<uint8_t> above(size_);
    std::vector<uint8_t> left(size_);
    for (int src_value : {0, 128, 255}) {
        for (int top_value : {0, 128, 255}) {
            for (int left_value : {0, 128, 255}) {
                std::fill(src.begin(), src.end(), src_value);
                std::fill(above.begin(), above.end(), top_value);
                std::fill(left.begin(), left.end(), left_value);
                check(src.data(), size_, above.data(), left.data());
                if (HasFatalFailure())
                    return;
            }
        }
    }
}

TEST_P(DcSadTest, RoundingMatch) {
    const uint32_t count = size_ * (have_top_ + have_left_);
    if (!count)
        return;
    std::vector<uint8_t> src(size_ * size_, 127);
    std::vector<uint8_t> above(size_);
    std::vector<uint8_t> left(size_);
    for (int dc : {0, 1, 127, 254}) {
        for (int delta : {-1, 0, 1}) {
            uint32_t remaining = dc * count + count / 2 + delta;
            std::fill(above.begin(), above.end(), 0);
            std::fill(left.begin(), left.end(), 0);
            for (uint32_t i = 0; i < size_; ++i) {
                if (have_top_) {
                    above[i] = std::min(remaining, 255u);
                    remaining -= above[i];
                }
                if (have_left_) {
                    left[i] = std::min(remaining, 255u);
                    remaining -= left[i];
                }
            }
            ASSERT_EQ(0u, remaining);
            check(src.data(), size_, above.data(), left.data());
            if (HasFatalFailure())
                return;
        }
    }
}

TEST_P(DcSadTest, DISABLED_Speed) {
    EbCpuFlags flags =
        func_ == svt_aom_dc_sad_c ? 0 : svt_aom_get_cpu_flags_to_use();
#ifdef ARCH_X86_64
    flags &= (EB_CPU_FLAGS_AVX2 << 1) - 1;
#elif defined(ARCH_AARCH64)
    flags &= EB_CPU_FLAGS_NEON;
#endif
    svt_aom_setup_common_rtcd_internal(flags);
    svt_aom_setup_rtcd_internal(flags);
    const DcPredFunc optimized_predictors[3][2][2] = {
        {{svt_aom_dc_128_predictor_16x16, svt_aom_dc_top_predictor_16x16},
         {svt_aom_dc_left_predictor_16x16, svt_aom_dc_predictor_16x16}},
        {{svt_aom_dc_128_predictor_32x32, svt_aom_dc_top_predictor_32x32},
         {svt_aom_dc_left_predictor_32x32, svt_aom_dc_predictor_32x32}},
        {{svt_aom_dc_128_predictor_64x64, svt_aom_dc_top_predictor_64x64},
         {svt_aom_dc_left_predictor_64x64, svt_aom_dc_predictor_64x64}}};
    const int index = size_ == 16 ? 0 : size_ == 32 ? 1 : 2;
    const DcPredFunc predict =
        optimized_predictors[index][have_left_][have_top_];
    ASSERT_EQ(func_, svt_aom_dc_sad);
    if (func_ != svt_aom_dc_sad_c) {
        ASSERT_NE(predictors[index][have_left_][have_top_], predict);
        ASSERT_NE(svt_nxm_sad_kernel_helper_c, svt_nxm_sad_kernel);
    }
    std::vector<uint8_t> src(size_ * size_);
    std::vector<uint8_t> above(size_);
    std::vector<uint8_t> left(size_);
    std::vector<uint8_t> pred(size_ * size_);
    svt_av1_test_tool::SVTRandom rnd(0, 255);
    for (auto *data : {&src, &above, &left}) {
        std::generate(data->begin(), data->end(), [&rnd]() {
            return static_cast<uint8_t>(rnd.random());
        });
    }
    auto separate = [&]() {
        predict(pred.data(), size_, above.data(), left.data());
        return svt_nxm_sad_kernel(
            src.data(), size_, pred.data(), size_, size_, size_);
    };
    auto fused = [&]() {
        return func_(src.data(),
                     size_,
                     above.data(),
                     left.data(),
                     size_,
                     have_top_,
                     have_left_);
    };
    ASSERT_EQ(separate(), fused());
    constexpr int iterations = 200000;
    auto measure = [&](bool use_fused) {
        uint64_t sum = 0;
        const auto start = std::chrono::steady_clock::now();
        if (use_fused) {
            for (int i = 0; i < iterations; ++i) {
                sum += fused();
            }
        } else {
            for (int i = 0; i < iterations; ++i) {
                sum += separate();
            }
        }
        const auto end = std::chrono::steady_clock::now();
        EXPECT_NE(0u, sum);
        return std::chrono::duration<double, std::nano>(end - start).count() /
               iterations;
    };
    for (int i = 0; i < 1000; ++i) {
        ASSERT_EQ(separate(), fused());
    }
    std::array<double, 7> separate_ns;
    std::array<double, 7> fused_ns;
    for (size_t i = 0; i < separate_ns.size(); ++i) {
        if (i & 1) {
            fused_ns[i] = measure(true);
            separate_ns[i] = measure(false);
        } else {
            separate_ns[i] = measure(false);
            fused_ns[i] = measure(true);
        }
    }
    std::sort(separate_ns.begin(), separate_ns.end());
    std::sort(fused_ns.begin(), fused_ns.end());
    std::printf(
        "size=%u top=%d left=%d separate=%.2f ns fused=%.2f ns speedup=%.2fx\n",
        size_,
        have_top_,
        have_left_,
        separate_ns[3],
        fused_ns[3],
        separate_ns[3] / fused_ns[3]);
}

INSTANTIATE_TEST_SUITE_P(C, DcSadTest,
                         ::testing::Combine(::testing::Values(svt_aom_dc_sad_c),
                                            ::testing::Values(16u, 32u, 64u),
                                            ::testing::Bool(),
                                            ::testing::Bool()));
#ifdef ARCH_X86_64
INSTANTIATE_TEST_SUITE_P(
    AVX2, DcSadTest,
    ::testing::Combine(::testing::Values(svt_aom_dc_sad_avx2),
                       ::testing::Values(16u, 32u, 64u), ::testing::Bool(),
                       ::testing::Bool()));
#endif
#ifdef ARCH_AARCH64
INSTANTIATE_TEST_SUITE_P(
    NEON, DcSadTest,
    ::testing::Combine(::testing::Values(svt_aom_dc_sad_neon),
                       ::testing::Values(16u, 32u, 64u), ::testing::Bool(),
                       ::testing::Bool()));
#endif
}  // namespace
