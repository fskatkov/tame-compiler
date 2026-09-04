#include "tame/backend/virtual_machine.h"

using namespace tame::frontend;
using namespace tame::backend;

VirtualMachine::VirtualMachine(diagnostics::DiagnosticEngine &diagnostic_engine, MetalEngine &metal_engine)
    : metal_engine(metal_engine), diagnostic_engine(diagnostic_engine) {
    vm_stack.reserve(256);
}

VirtualMachineResult VirtualMachine::execute(std::unique_ptr<CodeBuffer> code_buffer) {
    if (!code_buffer) {
        return VirtualMachineResult::COMPILE_TIME_ERROR;
    }

    code_buffer_ = std::move(code_buffer);
    address = code_buffer_->data.data();
    return execute_instruction();
}

std::uint8_t VirtualMachine::read_byte() {
    return *address++;
}

Value VirtualMachine::read_constant() {
    return code_buffer_->values.at(read_byte());
}

template<typename... T>
VirtualMachineResult VirtualMachine::execute_operation(const std::string &symbol, T... operands) {
    const auto rhs = pop();
    const auto lhs = pop();

    return std::visit<VirtualMachineResult>(overloaded{
        operands...,
        [&](const auto &, const auto &) {
            report_error(std::format("unsupported operand types for `{}`: [{}] and [{}]",
                symbol,
                lhs.get_type(),
                rhs.get_type()
            ));

            return VirtualMachineResult::RUNTIME_ERROR;
        }
    }, lhs.value, rhs.value);
}

void VirtualMachine::reset_stack() {
    vm_stack.clear();
}

void VirtualMachine::push(const Value &value) {
    vm_stack.push_back(value);
}

Value VirtualMachine::pop() {
    auto current_value = vm_stack.back();
    vm_stack.pop_back();
    return current_value;
}

Value VirtualMachine::peek(const int &distance) const {
    return vm_stack.at(vm_stack.size() - distance - 1);
}

void VirtualMachine:: report_error(const std::string &message) {
    const std::size_t instruction_index = std::distance(code_buffer_->data.data(), address) - 1;
    const auto location = code_buffer_->locations[instruction_index];

    diagnostic_engine.report(
        diagnostics::DiagnosticReport::DiagnosticReportType::FATAL,
        message,
        location
    );

    reset_stack();
}

const std::array<VirtualMachine::InstructionHandler, 256> VirtualMachine::dispatch_table = [] {
    std::array<InstructionHandler, 256> table{};

    table[std::to_underlying(Instruction::OP_CONSTANT)] = &VirtualMachine::execute_constant;
    table[std::to_underlying(Instruction::OP_BUILD_TENSOR)] = &VirtualMachine::execute_tensor;

    table[std::to_underlying(Instruction::OP_ADD)] = &VirtualMachine::execute_addition;
    table[std::to_underlying(Instruction::OP_SUB)] = &VirtualMachine::execute_subtraction;
    table[std::to_underlying(Instruction::OP_MUL)] = &VirtualMachine::execute_multiplication;
    table[std::to_underlying(Instruction::OP_MATMUL)] = &VirtualMachine::execute_matmul;
    table[std::to_underlying(Instruction::OP_DIV)] = &VirtualMachine::execute_division;

    table[std::to_underlying(Instruction::OP_DEFINE_VARIABLE)] = &VirtualMachine::execute_define_global_variable;

    table[std::to_underlying(Instruction::OP_GET_GLOBAL)] = &VirtualMachine::execute_get_global_variable;
    table[std::to_underlying(Instruction::OP_SET_GLOBAL)] = &VirtualMachine::execute_set_global_variable;
    table[std::to_underlying(Instruction::OP_GET_LOCAL)] = &VirtualMachine::execute_get_local_variable;
    table[std::to_underlying(Instruction::OP_SET_LOCAL)] = &VirtualMachine::execute_set_local_variable;

    table[std::to_underlying(Instruction::OP_EXECUTE_GPU)] = &VirtualMachine::execute_gpu_kernel;

    table[std::to_underlying(Instruction::OP_POP)] = &VirtualMachine::execute_pop;
    table[std::to_underlying(Instruction::OP_PRINT)] = &VirtualMachine::execute_print;
    table[std::to_underlying(Instruction::OP_RETURN)] = &VirtualMachine::execute_return;

    return table;
}();

VirtualMachineResult VirtualMachine::execute_instruction() {
    while (true) {
        const auto current_instruction = (this->*dispatch_table.at(read_byte()))();

        if (current_instruction == VirtualMachineResult::OK) {
            continue;
        }

        if (current_instruction == VirtualMachineResult::HALT) {
            return VirtualMachineResult::OK;
        }

        return current_instruction;
    }
}

inline VirtualMachineResult VirtualMachine::execute_constant() {
    push(read_constant());
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_tensor() {
    const auto is_floating_point = read_byte();
    const auto tensor_rank = read_byte();

    std::vector<int> tensor_shape;
    tensor_shape.reserve(tensor_rank);

    auto elements_quantity = 1;
    for (int i = 0; i < tensor_rank; ++i) {
        const auto dimension = read_byte();
        tensor_shape.push_back(dimension);
        elements_quantity *= dimension;
    }

    const auto blob_ptr_index = read_byte();

    const auto tensor = std::make_shared<TensorStructure>();
    tensor->tensor_shape = std::move(tensor_shape);
    tensor->set_strides();
    tensor->data_type = is_floating_point == 0 ? TensorDataType::Float32 : TensorDataType::Int32;

    if (elements_quantity == 0) {
        tensor->buffer = nullptr;
        push(tensor);
        return VirtualMachineResult::OK;
    }

    const auto &blob_ptr = code_buffer_->values.at(blob_ptr_index).get<BlobPtr>();
    tensor->buffer = metal_engine.allocate_buffer(blob_ptr->size());
    std::memcpy(tensor->buffer->contents(), blob_ptr->data(), blob_ptr->size());

    push(tensor);
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_addition() {
    return execute_operation("+",
        [&](const int &first_operand, const int &second_operand) {
            push(first_operand + second_operand);
            return VirtualMachineResult::OK;
        },
        [&](const float &first_operand, const float &second_operand) {
            push(first_operand + second_operand);
            return VirtualMachineResult::OK;
        }
    );
}

inline VirtualMachineResult VirtualMachine::execute_subtraction() {
    return execute_operation("-",
        [&](const int &first_operand, const int &second_operand) {
            push(first_operand - second_operand);
            return VirtualMachineResult::OK;
        },
        [&](const float &first_operand, const float &second_operand) {
            push(first_operand - second_operand);
            return VirtualMachineResult::OK;
        }
    );
}

inline VirtualMachineResult VirtualMachine::execute_multiplication() {
    return execute_operation("*",
        [&](const int &first_operand, const int &second_operand) {
            push(first_operand * second_operand);
            return VirtualMachineResult::OK;
        },
        [&](const float &first_operand, const float &second_operand) {
            push(first_operand * second_operand);
            return VirtualMachineResult::OK;
        }
    );
}

inline VirtualMachineResult VirtualMachine::execute_matmul() {
    return execute_operation("@", [&](const TensorPtr &first_tensor, const TensorPtr &second_tensor) {
        const auto M = first_tensor->tensor_shape[0];
        const auto N = second_tensor->tensor_shape[1];
        const auto K = first_tensor->tensor_shape[1];

        if (first_tensor->tensor_shape[1] != second_tensor->tensor_shape[0]) {
            report_error("tensor dimension mismatch");
            return VirtualMachineResult::RUNTIME_ERROR;
        }

        if (first_tensor->data_type != second_tensor->data_type) {
            report_error("impossible to multiply tensors of different element types");
            return VirtualMachineResult::RUNTIME_ERROR;
        }

        const auto resulting_tensor = std::make_shared<TensorStructure>();
        resulting_tensor->tensor_shape = {M, N};
        resulting_tensor->set_strides();
        resulting_tensor->data_type = first_tensor->data_type;
        resulting_tensor->buffer = metal_engine.dispatch_matmul(
            first_tensor->buffer, second_tensor->buffer, M, N, K, first_tensor->data_type
        );

        push(resulting_tensor);
        return VirtualMachineResult::OK;
    });
}

inline VirtualMachineResult VirtualMachine::execute_division() {
    return execute_operation("/",
        [&](const int &first_operand, const int &second_operand) {
            push(first_operand / second_operand);
            return VirtualMachineResult::OK;
        },
        [&](const float &first_operand, const float &second_operand) {
            push(first_operand / second_operand);
            return VirtualMachineResult::OK;
        }
    );
}

inline VirtualMachineResult VirtualMachine::execute_gpu_kernel() {
    const auto quantity = read_byte();

    if (quantity == 0) {
        report_error("not enough tensors to execute");
        return VirtualMachineResult::RUNTIME_ERROR;
    }

    std::vector<Value> current_values(quantity);
    for (int i = quantity - 1; i >= 0; --i) {
        current_values[i] = pop();
    }

    const auto pipeline_id = static_cast<std::size_t>(pop().get<int>());

    std::vector<int> tensor_shape;
    TensorDataType data_type{TensorDataType::Float32};
    bool is_tensor{false};

    for (const auto &value : current_values) {
        if (value.is<TensorPtr>()) {
            const auto current_tensor = value.get<TensorPtr>();
            tensor_shape = current_tensor->tensor_shape;
            data_type = current_tensor->data_type;
            is_tensor = true;
            break;
        }
    }

    if (!is_tensor) {
        report_error("at least one operand must be a tensor");
        return VirtualMachineResult::RUNTIME_ERROR;
    }

    const auto tensor_rank = tensor_shape.size();

    std::size_t elements_quantity{1};
    for (const auto &dimension : tensor_shape) {
        elements_quantity *= dimension;
    }

    const std::size_t element_size = data_type == TensorDataType::Float32 ? sizeof(float) : sizeof(int);

    std::vector<TensorPtr> retained_tensors(quantity);
    for (std::size_t i = 0; i < quantity; ++i) {
        if (auto current_value = current_values[i]; current_value.is<TensorPtr>()) {
            retained_tensors[i] = current_value.get<TensorPtr>();

            if (retained_tensors[i]->data_type != data_type) {
                report_error("tensor element type mismatch");
                return VirtualMachineResult::RUNTIME_ERROR;
            }

            const std::size_t current_elements = retained_tensors[i]->buffer
                                                     ? retained_tensors[i]->buffer->length() / element_size
                                                     : 0;

            if (current_elements != elements_quantity) {
                report_error("tensor dimension mismatch");
                return VirtualMachineResult::RUNTIME_ERROR;
            }
        } else if (current_value.is<float>() || current_value.is<int>()) {
            const auto scalar_tensor = std::make_shared<TensorStructure>();
            scalar_tensor->tensor_shape = tensor_shape;
            scalar_tensor->strides = std::vector<uint64_t>(tensor_rank > 0 ? tensor_rank : 1, 0);
            scalar_tensor->data_type = data_type;
            scalar_tensor->buffer = metal_engine.allocate_buffer(element_size);

            if (data_type == TensorDataType::Float32) {
                const float final_value = current_value.get<float>();
                std::memcpy(scalar_tensor->buffer->contents(), &final_value, sizeof(float));
            } else {
                const int final_value = current_value.get<int>();
                std::memcpy(scalar_tensor->buffer->contents(), &final_value, sizeof(int));
            }

            retained_tensors[i] = scalar_tensor;
        } else {
            report_error("expected tensor or scalar input");
            return VirtualMachineResult::RUNTIME_ERROR;
        }
    }

    const auto resulting_buffer = metal_engine.dispatch(pipeline_id, retained_tensors, elements_quantity, data_type);

    if (!resulting_buffer && elements_quantity > 0) {
        report_error("tensor op execution failed");
        return VirtualMachineResult::RUNTIME_ERROR;
    }

    const auto resulting_tensor = std::make_shared<TensorStructure>();
    resulting_tensor->tensor_shape = std::move(tensor_shape);
    resulting_tensor->set_strides();
    resulting_tensor->data_type = data_type;
    resulting_tensor->buffer = resulting_buffer;

    push(resulting_tensor);
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_define_global_variable() {
    const auto variable_name = read_constant().get<std::shared_ptr<std::string>>();
    vm_variables.insert_or_assign(*variable_name, peek(0));
    pop();
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_get_global_variable() {
    const auto variable_name = read_constant().get<std::shared_ptr<std::string>>();

    if (const auto it = vm_variables.find(*variable_name); it != vm_variables.end()) {
        push(it->second);
    } else {
        report_error(std::format("undefined variable `{}`", *variable_name));
        return VirtualMachineResult::RUNTIME_ERROR;
    }

    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_set_global_variable() {
    const auto variable_name = read_constant().get<std::shared_ptr<std::string>>();

    if (const auto it = vm_variables.find(*variable_name); it != vm_variables.end()) {
        vm_variables.insert_or_assign(*variable_name, peek(0));
    } else {
        report_error(std::format("undefined variable `{}`", *variable_name));
        return VirtualMachineResult::RUNTIME_ERROR;
    }

    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_get_local_variable() {
    const auto local_variable_index = read_byte();
    push(vm_stack[local_variable_index]);
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_set_local_variable() {
    const auto local_variable_index = read_byte();
    vm_stack[local_variable_index] = peek(0);
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_pop() {
    pop();
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_print() {
    metal_engine.synchronize_engine();
    std::cout << peek(0).get_value() << "\n";
    pop();
    return VirtualMachineResult::OK;
}

inline VirtualMachineResult VirtualMachine::execute_return() {
    if (!vm_stack.empty()) {
        const auto final_value = pop();
        push(final_value);
    }

    return VirtualMachineResult::HALT;
}

inline VirtualMachineResult VirtualMachine::execute_unknown_operation() {
    return VirtualMachineResult::OK;
}
