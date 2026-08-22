#pragma once

#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <memory>
#include <format>
#include <ranges>
#include <charconv>
#include <cstddef>
#include <utility>

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static bool is_digit(const char &symbol) {
    return symbol >= '0' && symbol <= '9';
}

static bool is_alpha(const char &symbol) {
    return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || symbol == '_';
}

static bool is_alpha_numeric(const char &symbol) {
    return is_digit(symbol) || is_alpha(symbol);
}

struct SourceLocation {
    std::size_t line{1};
    std::size_t column{1};
    std::size_t offset{0};
    std::size_t length{1};
};
