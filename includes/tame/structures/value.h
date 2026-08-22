#pragma once

#include "tame/support/common.h"

namespace tame::frontend {
    struct NIL {
        auto operator<=>(const NIL &) const = default;
    };

    struct TensorStructure {
        using TensorStorage = std::variant<std::vector<float>, std::vector<int>>;

        std::vector<std::size_t> tensor_shape;
        TensorStorage tensor_data;

        [[nodiscard]] std::string get_shape() const {
            if (tensor_shape.empty()) return "tensor<>";

            auto stringified_tensor_shape = tensor_shape | std::views::transform([](auto elem) {
                return std::to_string(elem);
            }) | std::views::join_with('x');
            return std::format("tensor<{}>", std::ranges::to<std::string>(stringified_tensor_shape));
        }

        [[nodiscard]] std::string print() const {
            return std::visit([this](const auto &data) -> std::string {
                using TensorUnderlyingType = typename std::decay_t<decltype(data)>::value_type;
                const std::string_view data_type = std::is_same_v<TensorUnderlyingType, float> ? "f32" : "i32";

                if (tensor_shape.empty() || data.empty()) {
                    return std::format("tensor([], dtype={})\n", data_type);
                }

                std::vector<std::size_t> strides(tensor_shape.size(), 1);
                for (int i = static_cast<int>(tensor_shape.size()) - 2; i >= 0; --i) {
                    strides[i] = strides[i + 1] * tensor_shape[i + 1];
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
                            if constexpr (std::is_same_v<TensorUnderlyingType, float>) {
                                resulting_tensor += std::format("{:8.4f}", data[offset + i]);
                            } else {
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

                            self(depth + 1, offset + i * strides[depth], indent + 1);
                        }
                    }

                    resulting_tensor += ']';
                };

                format_tensor_dimension(0, 0, 7);

                auto stringified_shape = tensor_shape | std::views::transform([](auto elem) {
                                             return std::to_string(elem);
                                         })
                                         | std::views::join_with(std::string_view(", "))
                                         | std::ranges::to<std::string>();
                resulting_tensor += std::format(",\n       shape=({}), dtype={})", stringified_shape, data_type);
                return resulting_tensor;
            }, tensor_data);
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
            [](StringPtr) { return std::string("str"); },
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
