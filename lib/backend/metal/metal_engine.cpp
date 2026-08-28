#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "tame/backend/metal/metal_engine.h"

using namespace tame::backend;

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
std::vector<T> MetalEngine::dispatch(
    const std::string &source,
    const std::vector<const std::vector<T> *> &values,
    const std::size_t elements_quantity
) {
    if (elements_quantity == 0) {
        return {};
    }

    std::string source_with_predefined_type;
    if constexpr (std::is_same_v<T, float>) {
        source_with_predefined_type = "typedef float DTYPE;\n\n" + source;
    } else if constexpr (std::is_same_v<T, int>) {
        source_with_predefined_type = "typedef int DTYPE;\n\n" + source;
    }

    MTL::ComputePipelineState *pipeline_state = nullptr;

    if (const auto it = pipeline_cache.find(source_with_predefined_type); it != pipeline_cache.end()) {
        pipeline_state = it->second;
    } else {
        NS::Error *dynamic_library_error = nullptr;
        MTL::Library *dynamic_library = device->newLibrary(
            NS::String::string(source_with_predefined_type.c_str(), NS::UTF8StringEncoding),
            MTL::CompileOptions::alloc()->init(),
            &dynamic_library_error
        );

        if (!dynamic_library) {
            std::println(stderr, "error: {}\n", dynamic_library_error->localizedDescription()->utf8String());
            return {};
        }

        MTL::Function *dynamic_function = dynamic_library->newFunction(
            NS::String::string("tensor_op", NS::UTF8StringEncoding)
        );

        dynamic_library->release();

        pipeline_state = device->newComputePipelineState(dynamic_function, &dynamic_library_error);

        dynamic_function->release();

        if (!pipeline_state) {
            std::println(stderr, "pipeline fail: {}\n", dynamic_library_error->localizedDescription()->utf8String());
            return {};
        }

        pipeline_cache[source_with_predefined_type] = pipeline_state;
    }

    const std::size_t data_size = elements_quantity * sizeof(T);

    std::vector<MTL::Buffer *> buffers;
    buffers.reserve(values.size());

    for (const auto *value : values) {
        buffers.push_back(device->newBuffer(value->data(), data_size, MTL::ResourceStorageModeShared));
    }

    auto *resulting_buffer = device->newBuffer(data_size, MTL::ResourceStorageModeShared);
    auto *command_buffer = command_queue->commandBuffer();
    auto *compute_encoder = command_buffer->computeCommandEncoder();

    compute_encoder->setComputePipelineState(pipeline_state);

    std::size_t buffer_index{0};
    for (const auto *buffer : buffers) {
        compute_encoder->setBuffer(buffer, 0, buffer_index++);
    }
    compute_encoder->setBuffer(resulting_buffer, 0, buffer_index++);

    const auto elements_quantity_uint = static_cast<uint>(elements_quantity);
    compute_encoder->setBytes(&elements_quantity_uint, sizeof(uint), buffer_index);

    auto thread_group_size = pipeline_state->maxTotalThreadsPerThreadgroup();
    if (thread_group_size > elements_quantity) {
        thread_group_size = elements_quantity;
    }

    compute_encoder->dispatchThreads(MTL::Size(elements_quantity, 1, 1), MTL::Size(thread_group_size, 1, 1));
    compute_encoder->endEncoding();

    command_buffer->commit();
    command_buffer->waitUntilCompleted();

    std::vector<T> resulting_tensor(elements_quantity);
    std::memcpy(resulting_tensor.data(), resulting_buffer->contents(), data_size);

    for (auto *buffer : buffers) {
        buffer->release();
    }
    resulting_buffer->release();

    return resulting_tensor;
}

template std::vector<float> MetalEngine::dispatch(
    const std::string &source,
    const std::vector<const std::vector<float> *> &values,
    const std::size_t elements_quantity
);

template std::vector<int> MetalEngine::dispatch(
    const std::string &source,
    const std::vector<const std::vector<int> *> &values,
    const std::size_t elements_quantity
);

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