#pragma once
#include "util.h"
#include <limits>
#include <algorithm>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#define CLAMP(v,lower,upper) std::max(lower, std::min(v, upper))

#define F_C_MULT(x,y) static_cast<basetype_t>((static_cast<longbasetype_t>(x)*static_cast<longbasetype_t>(y)) >> F_C_FRAC)
#define F_E_MULT(x,y) static_cast<basetype_t>((static_cast<longbasetype_t>(x)*static_cast<longbasetype_t>(y)) >> F_E_FRAC)
#define F_P_MULT(x,y) static_cast<basetype_t>((static_cast<longbasetype_t>(x)*static_cast<longbasetype_t>(y)) >> F_P_FRAC)

#define F_C_DIVD(x,y) static_cast<basetype_t>((static_cast<longbasetype_t>(x) << F_C_FRAC) / (static_cast<longbasetype_t>(y)))
#define F_E_DIVD(x,y) static_cast<basetype_t>((static_cast<longbasetype_t>(x) << F_E_FRAC) / (static_cast<longbasetype_t>(y)))
#define F_P_DIVD(x,y) static_cast<basetype_t>((static_cast<longbasetype_t>(x) << F_P_FRAC) / (static_cast<longbasetype_t>(y)))

#define F_C_SQRT(x) static_cast<basetype_t>(sqrtF2F(static_cast<longbasetype_t>(x) << (F_C_BITS+1)) >> (F_C_BITS + 1))
#define F_E_SQRT(x) static_cast<basetype_t>(sqrtF2F(static_cast<longbasetype_t>(x) << (F_E_BITS+1)) >> (F_E_BITS + 1))
#define F_P_SQRT(x) static_cast<basetype_t>(sqrtF2F(static_cast<longbasetype_t>(x) << (F_P_BITS+1)) >> (F_P_BITS + 1))

//template <typename T>
inline basetype_t to_fixed(float value, float range) {
    return static_cast<basetype_t>(CLAMP(value, -range, range) / range * std::numeric_limits<basetype_t>::max() + 0.5f); // +0.5f is te proberen of std::floor ipv static_cast
}

template <typename T>
inline float to_float(T value, float range) {
    return value < 0
        ? -static_cast<float>(value) / std::numeric_limits<T>::min() * range
        : static_cast<float>(value) / std::numeric_limits<T>::max() * range
        ;
}

#ifdef __ARM_NEON__
inline float32x4_t clamp_vec(float32x4_t value, float lower, float upper) {
    return vmaxq_f32(vdupq_n_f32(lower), vminq_f32(value, vdupq_n_f32(upper)));
}

inline int16x4_t to_fixed(float32x4_t value, float range) {
    // Rescale
    float a = std::numeric_limits<basetype_t>::max() / range;
    float32x4_t b = { 0.5f, 0.5f, 0.5f, 0.5f };

    // Convert to signed int and truncate
    return vmovn_s32(vcvtq_s32_f32(vaddq_f32(vmulq_n_f32(clamp_vec(value, -range, range), a), b)));
}

inline float32x4_t to_float(int16x4_t value, float range) {
    int16x4_t zero_vec = { 0, 0, 0, 0 };
    uint32x4_t mask_vec = vmovl_u16(vclt_s16(value, zero_vec));
    float a = 1 / std::numeric_limits<basetype_t>::min() * range;
    float b = 1 / std::numeric_limits<basetype_t>::max() * range;
    float32x4_t value_f = vcvtq_f32_s32(vmovl_s16(value));

    return vbslq_f32(mask_vec, vmulq_n_f32(value_f, a), vmulq_n_f32(value_f, b));
}
#endif

inline longbasetype_t sqrtF2F(longbasetype_t x)
{
    uint32_t t, q, b, r;
    r = x;
    b = 1 << 30;
    q = 0;
    while (b > (1 << 20)) // i<<6
    {
        t = q + b;
        if (r >= t)
        {
            r -= t;
            q = t + b; // equivalent to q += 2*b
        }
        r <<= 1;
        b >>= 1;
    }
    q >>= 8;
    return q;
}

inline longbasetype_t sqrtF2Flong(longbasetype_t x)
{
    uint64_t t, q, b, r;
    r = x;
    b = 1ull << 62;
    q = 0ull;    
    while (b > (1 << 16))
    {
        t = q + b;
        if (r >= t)
        {
            r -= t;
            q = t + b; // equivalent to q += 2*b
        }
        r <<= 1;
        b >>= 1;
    }
    q >>= 16;
    return q;
}

