#pragma once
#include <util.h>
#include <limits>

#define CLAMP(v,lower,upper) std::max(lower, std::min(v, upper))

template <typename T>
inline float to_float(T value, float range) {
    return value < 0
        ? -static_cast<float>(value) / std::numeric_limits<T>::min() * range
        : static_cast<float>(value) / std::numeric_limits<T>::max() * range
        ;
}

//template <typename T>
inline basetype_t to_fixed(float value, float range) {
    return std::ceil(CLAMP(value,-range,range) / range * std::numeric_limits<basetype_t>::max()); // +0.5f is te proberen of std::floor ipv static_cast
}

