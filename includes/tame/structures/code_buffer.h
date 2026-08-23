#pragma once

#include "tame/support/common.h"
#include "value.h"

namespace tame::backend {
    class CodeBuffer {
    public:
        std::vector<std::uint8_t> data;
        std::vector<frontend::Value> values;

        explicit CodeBuffer() = default;

        void update(const std::uint8_t &byte) {
            data.push_back(byte);
        }

        void insert_value(const frontend::Value &value) {
            const auto new_value = add(value);
            update(std::to_underlying(Instruction::OP_CONSTANT));
            update(new_value);
        }

        std::uint8_t add(const frontend::Value &value) {
            values.push_back(value);
            return static_cast<std::uint8_t>(values.size() - 1);
        }
    };
}
