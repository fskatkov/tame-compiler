#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "tame/support/common.h"

class MetalEngine {
public:
    explicit MetalEngine();
    ~MetalEngine();

    std::vector<float> dispatch_matmul(
        const std::vector<float> &lhs,
        const std::vector<float> &rhs,
        uint M, uint N, uint K
    );
private:
    MTL::Device *device;
    MTL::CommandQueue *command_queue;
    MTL::ComputePipelineState *matmul_pipeline_state;
    std::unordered_map<std::string, MTL::ComputePipelineState *> pipeline_cache;
};
