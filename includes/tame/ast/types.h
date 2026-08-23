#pragma once

#include <vector>

namespace tame::ast {
    struct TypeAnnotation {
        virtual ~TypeAnnotation() = default;
    };

    struct TensorTypeAnnotation : public TypeAnnotation {
        std::vector<int> shape;

        explicit TensorTypeAnnotation(std::vector<int> shape)
            : shape(std::move(shape)) {  }
    };

    struct FloatTypeAnnotation : public TypeAnnotation {
        explicit FloatTypeAnnotation() = default;
    };

    struct IntTypeAnnotation : public TypeAnnotation {
        explicit IntTypeAnnotation() = default;
    };
}
