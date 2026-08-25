#include "tame/backend/virtual_machine.h"

using namespace tame::frontend;
using namespace tame::backend;

VirtualMachine::VirtualMachine(diagnostics::DiagnosticEngine &diagnostic_engine) : diagnostic_engine(diagnostic_engine) {
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
    diagnostic_engine.report(
        diagnostics::DiagnosticReport::DiagnosticReportType::FATAL,
        message,
        SourceLocation{}
    );

    reset_stack();
}

const std::array<VirtualMachine::InstructionHandler, 256> VirtualMachine::dispatch_table = [] {
    std::array<InstructionHandler, 256> table{};

    table[std::to_underlying(Instruction::OP_CONSTANT)] = &VirtualMachine::execute_constant;

    table[std::to_underlying(Instruction::OP_ADD)] = &VirtualMachine::execute_addition;
    table[std::to_underlying(Instruction::OP_SUB)] = &VirtualMachine::execute_subtraction;
    table[std::to_underlying(Instruction::OP_MUL)] = &VirtualMachine::execute_multiplication;
    table[std::to_underlying(Instruction::OP_DIV)] = &VirtualMachine::execute_division;

    table[std::to_underlying(Instruction::OP_DEFINE_VARIABLE)] = &VirtualMachine::execute_define_global_variable;

    table[std::to_underlying(Instruction::OP_GET_GLOBAL)] = &VirtualMachine::execute_get_global_variable;
    table[std::to_underlying(Instruction::OP_SET_GLOBAL)] = &VirtualMachine::execute_set_global_variable;
    table[std::to_underlying(Instruction::OP_GET_LOCAL)] = &VirtualMachine::execute_get_local_variable;
    table[std::to_underlying(Instruction::OP_SET_LOCAL)] = &VirtualMachine::execute_set_local_variable;

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

inline VirtualMachineResult VirtualMachine::execute_addition() {
    return execute_operation("+",
        [&](const int &first_operand, const int &second_operand) {
            push(first_operand + second_operand);
            return VirtualMachineResult::OK;
        },
        [&](const float &first_operand, const float &second_operand) {
            push(first_operand + second_operand);
            return VirtualMachineResult::RUNTIME_ERROR;
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
            return VirtualMachineResult::RUNTIME_ERROR;
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
            return VirtualMachineResult::RUNTIME_ERROR;
        }
    );
}

inline VirtualMachineResult VirtualMachine::execute_division() {
    return execute_operation("/",
        [&](const int &first_operand, const int &second_operand) {
            push(first_operand / second_operand);
            return VirtualMachineResult::OK;
        },
        [&](const float &first_operand, const float &second_operand) {
            push(first_operand / second_operand);
            return VirtualMachineResult::RUNTIME_ERROR;
        }
    );
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
