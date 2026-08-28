#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tame/backend/metal/metal_engine.h"

using namespace tame::backend;

namespace {
    class MetalEngineTest : public ::testing::Test {
    protected:
        MetalEngine metal_engine;

        template<typename T>
        static std::vector<T> matmul_op(
            std::span<const T> lhs,
            std::span<const T> rhs,
            uint32_t M, uint32_t N, uint32_t K
        ) {
            std::vector<T> resulting_matrix(M * N, T{0});
            for (uint32_t i = 0; i < M; ++i) {
                for (uint32_t k = 0; k < K; ++k) {
                    for (uint32_t j = 0; j < N; ++j) {
                        resulting_matrix[i * N + j] += lhs[i * K + k] * rhs[k * N + j];
                    }
                }
            }
            return resulting_matrix;
        }
    };
}

TEST_F(MetalEngineTest, DispatchMatmulF32) {
    constexpr uint32_t M = 4, N = 4, K = 4;

    std::vector<float> lhs(M * K);
    std::iota(lhs.begin(), lhs.end(), 1.0f);

    std::vector<float> rhs(N * K);
    std::iota(rhs.begin(), rhs.end(), 0.5f);

    const auto expected_matrix = matmul_op<float>(lhs, rhs, M, N, K);
    const auto current_matrix = metal_engine.dispatch_matmul<float>(lhs, rhs, M, N, K);

    for (std::size_t i = 0; i < current_matrix.size(); ++i) {
        EXPECT_NEAR(current_matrix[i], expected_matrix[i], 1e-4f) << "got mismatch at " << i;
    }
}

TEST_F(MetalEngineTest, DispatchDynamicAddition) {
    const std::string source = R"(
        #include <metal_stdlib>
        using namespace metal;

        kernel void tensor_op(
            device const DTYPE* buffer_0 [[buffer(0)]],
            device const DTYPE* buffer_1 [[buffer(1)]],
            device DTYPE* resulting_tensor [[buffer(2)]],
            constant uint& total_elements [[buffer(3)]],
            uint id [[thread_position_in_grid]]
        ) {
            if (id >= total_elements) { return; }
            resulting_tensor[id] = buffer_0[id] + buffer_1[id];
        }
    )";

    const std::vector<float> first_matrix{1.5f, 2.5f, 3.5f, 4.5f};
    const std::vector<float> second_matrix{10.0f, 20.0f, 30.0f, 40.0f};
    const std::vector<const std::vector<float>*> input_matrices{&first_matrix, &second_matrix};

    const auto resulting_matrix = metal_engine.dispatch<float>(source, input_matrices, first_matrix.size());
    const std::vector<float> expected_matrix{11.5f, 22.5f, 33.5f, 44.5f};

    for (std::size_t i = 0; i < resulting_matrix.size(); ++i) {
        EXPECT_FLOAT_EQ(resulting_matrix[i], expected_matrix[i]);
    }
}
