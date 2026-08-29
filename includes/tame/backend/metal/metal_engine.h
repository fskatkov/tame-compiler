#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "tame/support/common.h"
#include "tame/structures/value.h"

namespace tame::backend {
    class MetalEngine {
    public:
        explicit MetalEngine();
        ~MetalEngine();

        MTL::Buffer *dispatch(
            const std::string &source,
            std::span<MTL::Buffer *const> buffers,
            std::size_t elements_quantity,
            frontend::TensorDataType data_type
        );

        MTL::Buffer *dispatch_matmul(
            const MTL::Buffer *lhs_buffer,
            const MTL::Buffer *rhs_buffer,
            uint M, uint N, uint K,
            frontend::TensorDataType data_type
        ) const;

        [[nodiscard]] MTL::Buffer *allocate_buffer(std::size_t size_bytes) const {
            return device->newBuffer(size_bytes, MTL::ResourceStorageModeShared);
        }
    private:
        MTL::Device *device;
        MTL::CommandQueue *command_queue;
        MTL::ComputePipelineState *matmul_i32_pipeline_state;
        MTL::ComputePipelineState *matmul_f32_pipeline_state;
        std::unordered_map<std::string, MTL::ComputePipelineState *> pipeline_cache;
    };
}