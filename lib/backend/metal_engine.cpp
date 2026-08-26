#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "tame/backend/metal/metal_engine.h"

MetalEngine::MetalEngine() {
    device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        throw std::runtime_error("failed to initialize Metal device");
    }

    command_queue = device->newCommandQueue();

    NS::Error *library_initialization_error = nullptr;
    MTL::Library *general_library = device->newLibrary(
        NS::String::string("general.metallib", NS::UTF8StringEncoding),
        &library_initialization_error
    );

    if (!general_library) {
        std::println(stderr, "{}\n", library_initialization_error->localizedDescription()->utf8String());
        return;
    }

    MTL::Function *matmul_op = general_library->newFunction(NS::String::string("matmul_op", NS::UTF8StringEncoding));
    matmul_pipeline_state = device->newComputePipelineState(matmul_op, &library_initialization_error);;

    matmul_op->release();
    general_library->release();
}

MetalEngine::~MetalEngine() {
    matmul_pipeline_state->release();
    command_queue->release();
    device->release();
}

std::vector<float> MetalEngine::dispatch_matmul(
    const std::vector<float> &lhs,
    const std::vector<float> &rhs,
    uint M, uint N, uint K
) {

}