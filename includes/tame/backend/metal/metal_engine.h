#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "tame/support/common.h"

class MetalEngine {
public:
    explicit MetalEngine();
    ~MetalEngine();

    template<typename T>
    std::vector<T> dispatch_matmul(
        const std::vector<T> &lhs,
        const std::vector<T> &rhs,
        uint M, uint N, uint K
    );
private:
    MTL::Device *device;
    MTL::CommandQueue *command_queue;
    MTL::ComputePipelineState *matmul_pipeline_state;
    std::unordered_map<std::string, MTL::ComputePipelineState *> pipeline_cache;
};
