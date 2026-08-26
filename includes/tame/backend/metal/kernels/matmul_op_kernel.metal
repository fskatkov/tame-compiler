#include <metal_stdlib>

using namespace metal;

constant uint TILE_SIZE = 32;

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
    threadgroup T lhs_tensor_tile[TILE_SIZE][TILE_SIZE],
    threadgroup T rhs_tensor_tile[TILE_SIZE][TILE_SIZE]
) {
    uint row = grid_id.y;
    uint col = grid_id.x;

    T sum = T(0);
    uint tiles_quantity = (K + TILE_SIZE - 1) / TILE_SIZE;

    for (uint t = 0; t < tiles_quantity; ++t) {
         uint tiled_k = t * TILE_SIZE + local_id.x;
         if (row < M && tiled_k < K) {
            lhs_tensor_tile[local_id.y][local_id.x] = lhs_tensor[row * K + tiled_k];
         } else {
            lhs_tensor_tile[local_id.y][local_id.x] = 0.0;
         }

         tiled_k = t * TILE_SIZE + local_id.y;
         if (tiled_k < K && col < N) {
            rhs_tensor_tile[local_id.y][local_id.x] = rhs_tensor[tiled_k * N + col];
         } else {
            rhs_tensor_tile[local_id.y][local_id.x] = 0.0;
         }

         threadgroup_barrier(mem_flags::mem_threadgroup);

         for (uint i = 0; i < TILE_SIZE; ++i) {
            sum += lhs_tensor_tile[local_id.y][i] * rhs_tensor_tile[i][local_id.x];
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
    uint2 local_id [[thread_position_in_threadgroup]]
) {
    threadgroup float lhs_tile[TILE_SIZE][TILE_SIZE];
    threadgroup float rhs_tile[TILE_SIZE][TILE_SIZE];

    matmul_op<float>(lhs, rhs, resulting_tensor, M, N, K, grid_id, local_id, lhs_tile, rhs_tile);
}

kernel void matmul_op_i32(
    device const int *lhs [[buffer(0)]],
    device const int *rhs [[buffer(1)]],
    device int *resulting_tensor [[buffer(2)]],
    constant uint &M [[buffer(3)]],
    constant uint &N [[buffer(4)]],
    constant uint &K [[buffer(5)]],
    uint2 grid_id [[thread_position_in_grid]],
    uint2 local_id [[thread_position_in_threadgroup]]
) {
    threadgroup int lhs_tile[TILE_SIZE][TILE_SIZE];
    threadgroup int rhs_tile[TILE_SIZE][TILE_SIZE];

    matmul_op<int>(lhs, rhs, resulting_tensor, M, N, K, grid_id, local_id, lhs_tile, rhs_tile);
}
