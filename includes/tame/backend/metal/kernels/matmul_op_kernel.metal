#include <metal_stdlib>

using namespace metal;

template<typename T>
void matmul_op(
    device const T *lhs_tensor,
    device const T *rhs_tensor,
    device T *resulting_tensor,
    constant uint &M,
    constant uint &N,
    constant uint &K,
    uint2 grid_id,
    uint2 local_id,
    uint2 threadgroup_size,
    threadgroup T *shared_memory
) {
    uint row = grid_id.y;
    uint col = grid_id.x;

    uint TILE_DIMENSION = threadgroup_size.x;

    threadgroup T* lhs_tensor_tile = shared_memory;
    threadgroup T* rhs_tensor_tile = shared_memory + (TILE_DIMENSION * TILE_DIMENSION);

    T sum = T(0);
    uint tiles_quantity = (K + TILE_DIMENSION - 1) / TILE_DIMENSION;

    for (uint t = 0; t < tiles_quantity; ++t) {
         uint tiled_k_lhs = t * TILE_DIMENSION + local_id.x;
         if (row < M && tiled_k_lhs < K) {
            lhs_tensor_tile[local_id.y * TILE_DIMENSION + local_id.x] = lhs_tensor[row * K + tiled_k_lhs];
         } else {
            lhs_tensor_tile[local_id.y * TILE_DIMENSION + local_id.x] = T(0);
         }

        uint tiled_k_rhs = t * TILE_DIMENSION + local_id.y;
         if (tiled_k_rhs < K && col < N) {
            rhs_tensor_tile[local_id.y * TILE_DIMENSION + local_id.x] = rhs_tensor[tiled_k_rhs * N + col];
         } else {
            rhs_tensor_tile[local_id.y * TILE_DIMENSION + local_id.x] = T(0);
         }

         threadgroup_barrier(mem_flags::mem_threadgroup);

         for (uint i = 0; i < TILE_DIMENSION; ++i) {
            sum += lhs_tensor_tile[local_id.y * TILE_DIMENSION + i] * rhs_tensor_tile[i * TILE_DIMENSION + local_id.x];
         }

         threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (row < M && col < N) {
        resulting_tensor[row * N + col] = sum;
    }
}

kernel void matmul_op_f32(
    device const float *lhs [[buffer(0)]],
    device const float *rhs [[buffer(1)]],
    device float *resulting_tensor [[buffer(2)]],
    constant uint &M [[buffer(3)]],
    constant uint &N [[buffer(4)]],
    constant uint &K [[buffer(5)]],
    uint2 grid_id [[thread_position_in_grid]],
    uint2 local_id [[thread_position_in_threadgroup]],
    uint2 threadgroup_size [[threads_per_threadgroup]],
    threadgroup float *shared_memory [[threadgroup(0)]]
) {
    matmul_op<float>(lhs, rhs, resulting_tensor, M, N, K, grid_id, local_id, threadgroup_size, shared_memory);
}

kernel void matmul_op_i32(
    device const int *lhs [[buffer(0)]],
    device const int *rhs [[buffer(1)]],
    device int *resulting_tensor [[buffer(2)]],
    constant uint &M [[buffer(3)]],
    constant uint &N [[buffer(4)]],
    constant uint &K [[buffer(5)]],
    uint2 grid_id [[thread_position_in_grid]],
    uint2 local_id [[thread_position_in_threadgroup]],
    uint2 threadgroup_size [[threads_per_threadgroup]],
    threadgroup int *shared_memory [[threadgroup(0)]]
) {
    matmul_op<int>(lhs, rhs, resulting_tensor, M, N, K, grid_id, local_id, threadgroup_size, shared_memory);
}
