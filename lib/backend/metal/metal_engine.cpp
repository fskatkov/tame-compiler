#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "tame/backend/metal/metal_engine.h"

using namespace tame::frontend;
using namespace tame::backend;

MetalEngine::MetalEngine() {
    device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        std::println(stderr, "failed to initialize Metal device\n");
        return;
    }

    command_queue = device->newCommandQueue();

    MTL::HeapDescriptor *heap_descriptor = MTL::HeapDescriptor::alloc()->init();
    heap_descriptor->setSize(256 * 1024 * 1024);
    heap_descriptor->setStorageMode(MTL::StorageModeShared);
    heap_descriptor->setType(MTL::HeapTypeAutomatic);

    buffer_heap = device->newHeap(heap_descriptor);

    heap_descriptor->release();

    if (!buffer_heap) {
        command_queue->release();
        device->release();
        std::println(stderr, "failed to allocate Metal heap\n");
        return;
    }

    NS::Error *library_initialization_error = nullptr;
    MTL::Library *general_library = device->newLibrary(
        NS::String::string("general.metallib", NS::UTF8StringEncoding),
        &library_initialization_error
    );

    if (!general_library) {
        buffer_heap->release();
        command_queue->release();
        device->release();
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
    for (auto &[_, pipeline] : pipeline_cache) {
        pipeline->release();
    }
    pipeline_cache.clear();

    matmul_f32_pipeline_state->release();
    matmul_i32_pipeline_state->release();
    buffer_heap->release();
    command_queue->release();
    device->release();
}

std::size_t MetalEngine::compile_kernel(std::string_view source) {
    auto compile = [this, source](std::string_view data_type) -> MTL::ComputePipelineState* {
        const std::string source_with_predefined_type = std::format("typedef {} DTYPE;\n\n{}", data_type, source);

        MTL::CompileOptions *compile_options = MTL::CompileOptions::alloc()->init();
        compile_options->setFastMathEnabled(true);

        NS::Error *dynamic_library_error{nullptr};
        MTL::Library *dynamic_library = device->newLibrary(
            NS::String::string(source_with_predefined_type.c_str(), NS::UTF8StringEncoding),
            compile_options,
            &dynamic_library_error
        );

        compile_options->release();

        if (!dynamic_library) {
            std::println(stderr, "error: {}\n", dynamic_library_error->localizedDescription()->utf8String());
            return nullptr;
        }

        MTL::Function *dynamic_function = dynamic_library->newFunction(
            NS::String::string("tensor_op", NS::UTF8StringEncoding)
        );

        dynamic_library->release();

        MTL::ComputePipelineState *pipeline_state = device->newComputePipelineState(
            dynamic_function,
            &dynamic_library_error
        );

        dynamic_function->release();

        return pipeline_state;
    };

    const std::size_t pipeline_id = precompiled_kernels.size();

    precompiled_kernels.push_back({
        .float32_pipeline_state = compile("float"),
        .int32_pipeline_state = compile("int")
    });

    return pipeline_id;
}

MTL::Buffer *MetalEngine::dispatch(
    const std::size_t pipeline_id,
    const std::span<MTL::Buffer *const> buffers,
    const std::size_t elements_quantity,
    const TensorDataType data_type
) const {
    if (elements_quantity == 0) {
        return nullptr;
    }

    const auto &kernel_pipeline_pair = precompiled_kernels[pipeline_id];

    const auto *pipeline_state = data_type == TensorDataType::Float32
                                                    ? kernel_pipeline_pair.float32_pipeline_state
                                                    : kernel_pipeline_pair.int32_pipeline_state;

    const std::size_t data_size = elements_quantity * (data_type == TensorDataType::Float32
                                                           ? sizeof(float)
                                                           : sizeof(int));

    auto *resulting_buffer = allocate_buffer(data_size);
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

    const auto thread_group_size = std::min<uint>(pipeline_state->maxTotalThreadsPerThreadgroup(), elements_quantity_uint);

    compute_encoder->dispatchThreads(MTL::Size(elements_quantity_uint, 1, 1), MTL::Size(thread_group_size, 1, 1));
    compute_encoder->endEncoding();

    command_buffer->commit();
    command_buffer->waitUntilCompleted();

    return resulting_buffer;
}

MTL::Buffer *MetalEngine::dispatch_matmul(
    const MTL::Buffer *lhs_buffer,
    const MTL::Buffer *rhs_buffer,
    const uint M, const uint N, const uint K,
    const TensorDataType data_type
) const {
    const std::size_t data_size = M * N * (data_type == TensorDataType::Float32 ? sizeof(float) : sizeof(int));

    auto *resulting_buffer = allocate_buffer(data_size);
    auto *command_buffer = command_queue->commandBuffer();
    auto *compute_encoder = command_buffer->computeCommandEncoder();

    if (data_type == TensorDataType::Float32) {
        compute_encoder->setComputePipelineState(matmul_f32_pipeline_state);
    } else {
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

    return resulting_buffer;
}