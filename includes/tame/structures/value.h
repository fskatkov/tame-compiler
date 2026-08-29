#pragma once

#include "Metal/Metal.hpp"
#include "tame/support/common.h"

namespace tame::frontend {
    struct NIL {
        auto operator<=>(const NIL &) const = default;
    };

    enum class TensorDataType {
        Float32, Int32
    };

    struct TensorStructure {
        std::vector<int> tensor_shape;
        std::vector<uint64_t> strides;
        std::size_t byte_offset{0};

        TensorDataType data_type{TensorDataType::Float32};
        MTL::Buffer *buffer{nullptr};

        TensorStructure() = default;
        TensorStructure(const TensorStructure&) = delete;
        TensorStructure(TensorStructure &&other) noexcept
            : tensor_shape(std::move(other.tensor_shape)),
              strides(std::move(other.strides)),
              byte_offset(other.byte_offset),
              data_type(other.data_type),
              buffer(std::exchange(other.buffer, nullptr)) {  }

        ~TensorStructure() {
            if (buffer) {
                buffer->release();
            }
        }

        TensorStructure &operator=(const TensorStructure&) = delete;
        TensorStructure &operator=(TensorStructure &&other) noexcept {
            if (this != &other) {
                if (buffer) {
                    buffer->release();
                }

                tensor_shape = std::move(other.tensor_shape);
                strides = std::move(other.strides);
                byte_offset = other.byte_offset;
                data_type = other.data_type;
                buffer = std::exchange(other.buffer, nullptr);
            }

            return *this;
        }

        void set_strides() {
            if (tensor_shape.empty()) {
                strides.clear();
                return;
            }

            strides.assign(tensor_shape.size(), 1);
            for (int i = static_cast<int>(tensor_shape.size()) - 2; i >= 0; --i) {
                strides[i] = strides[i + 1] * static_cast<uint64_t>(tensor_shape[i + 1]);
            }
        }

        [[nodiscard]] std::string get_shape() const {
            if (tensor_shape.empty()) return "tensor<>";

            auto stringified_tensor_shape = tensor_shape | std::views::transform([](auto elem) {
                return std::to_string(elem);
            }) | std::views::join_with('x');
            return std::format("tensor<{}>", std::ranges::to<std::string>(stringified_tensor_shape));
        }

        [[nodiscard]] std::string print() const {
            const std::string_view stringified_data_type = data_type == TensorDataType::Float32 ? "f32" : "i32";

            if (tensor_shape.empty() || !buffer || buffer->length() == 0) {
                return std::format("tensor([], dtype={})\n", stringified_data_type);
            }

            std::string resulting_tensor = "tensor(";

            auto format_tensor_dimension = [&](
                    this auto &self,
                    const std::size_t depth,
                    const std::size_t offset,
                    const std::size_t indent
                ) -> void {
                const std::string indent_str(indent, ' ');

                resulting_tensor += '[';

                if (depth == tensor_shape.size() - 1) {
                    for (std::size_t i = 0; i < tensor_shape[depth]; ++i) {

                        if (data_type == TensorDataType::Float32) {
                            const auto *data = static_cast<const float*>(buffer->contents());
                            resulting_tensor += std::format("{:8.4f}", data[offset + i]);
                        } else {
                            const auto *data = static_cast<const int*>(buffer->contents());
                            resulting_tensor += std::format("{:8}", data[offset + i]);
                        }

                        if (i < tensor_shape[depth] - 1) {
                            resulting_tensor += ", ";
                        }
                    }
                } else {
                    for (std::size_t i = 0; i < tensor_shape[depth]; ++i) {
                        if (i > 0) {
                            resulting_tensor += ",\n" + indent_str + " ";

                            if (depth < tensor_shape.size() - 2) {
                                resulting_tensor += "\n" + indent_str + " ";
                            }
                        }

                        self(depth + 1, offset + i * (strides.empty() ? 1 : strides[depth]), indent + 1);
                    }
                }

                resulting_tensor += ']';
            };

            format_tensor_dimension(0, byte_offset, 7);

            auto stringified_shape = tensor_shape | std::views::transform([](auto elem) {
                                             return std::to_string(elem);
                                         })
                                         | std::views::join_with(std::string_view(", "))
                                         | std::ranges::to<std::string>();
            resulting_tensor += std::format(",\n       shape=({}), dtype={})", stringified_shape, stringified_data_type);
            return resulting_tensor;
        }
    };

    using TensorPtr = std::shared_ptr<TensorStructure>;
    using StringPtr = std::shared_ptr<std::string>;

    struct Value {
        using ValueType = std::variant<int, float, StringPtr, TensorPtr, NIL>;
        ValueType value{NIL{}};

        Value() = default;

        template<typename T>
        requires std::constructible_from<ValueType, T> && (!std::same_as<std::decay_t<T>, Value>)
        Value(T &&val) : value(std::forward<T>(val)) {  }

        template<typename T>
        [[nodiscard]] bool is() const { return std::holds_alternative<T>(value); }

        template<typename T>
        [[nodiscard]] T &get() {return std::get<T>(value); }

        template<typename T>
        [[nodiscard]] const T &get() const { return std::get<T>(value); }

        auto operator<=>(const Value &other) const = default;

        [[nodiscard]] std::string get_type() const;
        [[nodiscard]] std::string get_value() const;
    };

    inline std::string Value::get_type() const {
        return std::visit(overloaded{
            [](int) { return std::string("i32"); },
            [](float) { return std::string("f32"); },
            [](const StringPtr&) { return std::string("str"); },
            [](const TensorPtr &inner_value) { return inner_value->get_shape(); },
            [](NIL) { return std::string("null type"); }
        }, value);
    }

    inline std::string Value::get_value() const {
        return std::visit(overloaded{
            [](const auto inner_value) requires std::is_arithmetic_v<decltype(inner_value)> {
                return std::to_string(inner_value);
            },
            [](const StringPtr &inner_value) { return *inner_value; },
            [](const TensorPtr &inner_value) { return inner_value->print(); },
            [](NIL) { return std::string("null"); }
        }, value);
    }
}
