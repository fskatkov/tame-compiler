#include <metal_stdlib>

using namespace metal;

constant uint TILE_SIZE = 32;

kernel void matmul_op(
    device const float *lhs_tensor [[buffer(0)]],
    device const float *rhs_tensor [[buffer(1)]],
    device float *resulting_tensor [[buffer(2)]],
    constant uint &M [[buffer(3)]],
    constant uint &N [[buffer(4)]],
    constant uint &K [[buffer(5)]],
    uint2 grid_id [[thread_position_in_grid]],
    uint2 local_id [[thread_position_in_threadgroup]]
) {
    threadgroup float lhs_tensor_tile[TILE_SIZE][TILE_SIZE];
    threadgroup float rhs_tensor_tile[TILE_SIZE][TILE_SIZE];

    uint row = grid_id.y;
    uint col = grid_id.x;

    float sum = 0;
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
