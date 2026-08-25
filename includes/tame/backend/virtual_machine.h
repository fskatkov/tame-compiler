#pragma once

#include "tame/support/common.h"
#include "tame/support/diagnostic_engine.h"
#include "tame/structures/code_buffer.h"

namespace tame::backend {
    class VirtualMachine {
    public:
        explicit VirtualMachine(diagnostics::DiagnosticEngine &diagnostic_engine);
        VirtualMachineResult execute(std::unique_ptr<CodeBuffer> code_buffer);
    private:
        diagnostics::DiagnosticEngine &diagnostic_engine;

        std::unique_ptr<CodeBuffer> code_buffer_;
        const std::uint8_t *address = nullptr;

        std::vector<frontend::Value> vm_stack;
        std::unordered_map<std::string, frontend::Value> vm_variables;

        std::uint8_t read_byte();
        frontend::Value read_constant();

        template<typename... T>
        VirtualMachineResult execute_operation(const std::string &symbol, T... operands);

        void reset_stack();
        void push(const frontend::Value &value);
        frontend::Value pop();
        [[nodiscard]] frontend::Value peek(const int &distance) const;

        void report_error(const std::string &message);

        using InstructionHandler = VirtualMachineResult (VirtualMachine::*)();
        static const std::array<InstructionHandler, 256> dispatch_table;

        VirtualMachineResult execute_instruction();

        inline VirtualMachineResult execute_constant();

        inline VirtualMachineResult execute_addition();
        inline VirtualMachineResult execute_subtraction();
        inline VirtualMachineResult execute_multiplication();
        inline VirtualMachineResult execute_division();

        inline VirtualMachineResult execute_define_global_variable();
        inline VirtualMachineResult execute_get_global_variable();
        inline VirtualMachineResult execute_set_global_variable();
        inline VirtualMachineResult execute_get_local_variable();
        inline VirtualMachineResult execute_set_local_variable();

        inline VirtualMachineResult execute_pop();
        inline VirtualMachineResult execute_print();

        inline VirtualMachineResult execute_return();
        inline VirtualMachineResult execute_unknown_operation();
    };
};

