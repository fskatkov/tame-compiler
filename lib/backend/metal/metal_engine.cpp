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

    MTL::Function *matmul_op_f32 = general_library->newFunction(NS::String::string("matmul_op_f32", NS::UTF8StringEncoding));
    matmul_f32_pipeline_state = device->newComputePipelineState(matmul_op_f32, &library_initialization_error);
    matmul_op_f32->release();

    MTL::Function *matmul_op_i32 = general_library->newFunction(NS::String::string("matmul_op_i32", NS::UTF8StringEncoding));
    matmul_i32_pipeline_state = device->newComputePipelineState(matmul_op_i32, &library_initialization_error);
    matmul_op_i32->release();

    general_library->release();
}

MetalEngine::~MetalEngine() {
    matmul_f32_pipeline_state->release();
    matmul_i32_pipeline_state->release();
    command_queue->release();
    device->release();
}

template<typename T>
std::vector<T> MetalEngine::dispatch_matmul(const std::vector<T> &lhs, const std::vector<T> &rhs, uint M, uint N, uint K) {
    const std::size_t lhs_size = lhs.size() * sizeof(T);
    const std::size_t rhs_size = rhs.size() * sizeof(T);
    const std::size_t resulting_size = M * N * sizeof(T);

    auto *lhs_buffer = device->newBuffer(lhs.data(), lhs_size, MTL::ResourceStorageModeShared);
    auto *rhs_buffer = device->newBuffer(rhs.data(), rhs_size, MTL::ResourceStorageModeShared);
    auto *resulting_buffer = device->newBuffer(resulting_size, MTL::ResourceStorageModeShared);

    auto *command_buffer = command_queue->commandBuffer();
    auto *compute_encoder = command_buffer->computeCommandEncoder();

    if constexpr (std::is_same_v<T, float>) {
        compute_encoder->setComputePipelineState(matmul_f32_pipeline_state);
    } else if constexpr (std::is_same_v<T, int>) {
        compute_encoder->setComputePipelineState(matmul_i32_pipeline_state);
    }

    compute_encoder->setBuffer(lhs_buffer, 0, 0);
    compute_encoder->setBuffer(rhs_buffer, 0, 1);
    compute_encoder->setBuffer(resulting_buffer, 0, 2);

    compute_encoder->setBytes(&M, sizeof(uint), 3);
    compute_encoder->setBytes(&N, sizeof(uint), 4);
    compute_encoder->setBytes(&K, sizeof(uint), 5);

    compute_encoder->dispatchThreads(MTL::Size(N, M, 1), MTL::Size(32, 32, 1));
    compute_encoder->endEncoding();

    command_buffer->commit();
    command_buffer->waitUntilCompleted();

    std::vector<T> resulting_tensor(M * N);
    std::memcpy(resulting_tensor.data(), resulting_buffer->contents(), resulting_size);

    lhs_buffer->release();
    rhs_buffer->release();
    resulting_buffer->release();

    return resulting_tensor;
}

template std::vector<float> MetalEngine::dispatch_matmul<float>(
    const std::vector<float> &lhs,
    const std::vector<float> &rhs,
    uint M, uint N, uint K
);

template std::vector<int> MetalEngine::dispatch_matmul<int>(
    const std::vector<int> &lhs,
    const std::vector<int> &rhs,
    uint M, uint N, uint K
);