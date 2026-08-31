#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "tame/support/common.h"
#include "tame/structures/value.h"

namespace tame::backend {
    struct KernelPipelinePair {
        MTL::ComputePipelineState *float32_pipeline_state{nullptr};
        MTL::ComputePipelineState *int32_pipeline_state{nullptr};
    };

    class MetalEngine {
    public:
        explicit MetalEngine();
        ~MetalEngine();

        [[nodiscard]] std::size_t compile_kernel(std::string_view source);

        MTL::Buffer *dispatch(
            std::size_t pipeline_id,
            std::span<MTL::Buffer *const> buffers,
            std::size_t elements_quantity,
            frontend::TensorDataType data_type
        ) const;

        MTL::Buffer *dispatch_matmul(
            const MTL::Buffer *lhs_buffer,
            const MTL::Buffer *rhs_buffer,
            uint M, uint N, uint K,
            frontend::TensorDataType data_type
        ) const;

        [[nodiscard]] MTL::Buffer *allocate_buffer(std::size_t size_bytes) const {
            if (MTL::Buffer *buffer = buffer_heap->newBuffer(size_bytes, MTL::ResourceStorageModeShared)) {
                return buffer;
            }

            return device->newBuffer(size_bytes, MTL::ResourceStorageModeShared);
        }
    private:
        MTL::Device *device;
        MTL::CommandQueue *command_queue;
        MTL::Heap *buffer_heap;

        MTL::ComputePipelineState *matmul_i32_pipeline_state;
        MTL::ComputePipelineState *matmul_f32_pipeline_state;

        std::vector<KernelPipelinePair> precompiled_kernels;

        std::unordered_map<std::string, MTL::ComputePipelineState *> pipeline_cache;
    };
}