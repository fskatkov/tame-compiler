#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "tame/backend/metal/metal_engine.h"

MetalEngine::MetalEngine() {

}

MetalEngine::~MetalEngine() {

}

template<typename T>
std::vector<T> MetalEngine::dispatch_matmul(const std::vector<T> &lhs, const std::vector<T> &rhs, uint M, uint N, uint K) {

}
