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

        [[nodiscard]] MTL::Buffer *allocate_buffer(std::size_t size_bytes) const;

        void synchronize_engine();

        [[nodiscard]] MTL::Buffer *dispatch(
            std::size_t pipeline_id,
            std::span<MTL::Buffer *const> buffers,
            std::size_t elements_quantity,
            frontend::TensorDataType data_type
        );

        MTL::Buffer *dispatch_matmul(
            const MTL::Buffer *lhs_buffer,
            const MTL::Buffer *rhs_buffer,
            uint M, uint N, uint K,
            frontend::TensorDataType data_type
        );
    private:
        MTL::Device *device;
        MTL::CommandQueue *command_queue;
        MTL::Heap *buffer_heap;

        MTL::ComputePipelineState *matmul_i32_pipeline_state;
        MTL::ComputePipelineState *matmul_f32_pipeline_state;
        std::vector<KernelPipelinePair> precompiled_kernels;

        MTL::CommandBuffer *active_command_buffer{nullptr};
        [[nodiscard]] MTL::CommandBuffer *get_active_buffer();
    };
}