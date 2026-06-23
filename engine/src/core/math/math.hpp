#pragma once

#include "defines.hpp"

#if !LRELEASE 
#include "core/logging.hpp"
#endif

namespace MATH {
    constexpr f32 PI = 3.14159265358979323846f;
	constexpr f32 TWO_PI = PI * 2.0f;
	constexpr f32 FOUR_PI = PI / 4.0f;
	constexpr f32 HALF_PI = PI / 2.0f;
	constexpr f32 QUARTER_PI = PI / 4.0f;
	constexpr f32 ONE_OVER_PI = 1.0f / PI;

	constexpr f32 SQRT2 = 1.41421356237309504880f;
	constexpr f32 SQRT3 = 1.73205080756887729352f;
	
	constexpr f32 DEG2RAD = PI / 180.0f;
	constexpr f32 RAD2DEG = 180.0f / PI;
	
	constexpr f32 FLOAT_EPSILON = 1.192092896e-07f;
	constexpr f32 FLOAT_MAX = 3.40282e+38F;
	constexpr f32 FLOAT_MIN = -FLOAT_MAX;
	
	template <typename T>
    inline T next_power_of_two(T value) {
        if (value <= 1) return (T)1;
        if (value == 2) return (T)2;

        T result = value - 1;

        result |= result >> 1;
        result |= result >> 2;
        result |= result >> 4;

        if (sizeof(T) >= 2)  result |= result >> 8;
        if (sizeof(T) >= 4)  result |= result >> 16;
        if (sizeof(T) >= 8)  result |= result >> 32;
        if (sizeof(T) >= 16) result |= result >> 64;

        return result + 1;
    }
	
	inline f32 abs(const f32 value) {
	    if (value < 0.0f) return -value;
    	return value;
    }
	template <typename T>
	inline T abs(const T value) {
    	if (value < (T)0) return -value;
    	return value;
    }
	
	inline f32 sign(const f32 val) {
		return val == 0.0f ? 0.0f : val < 0.0f ? -1.0f : 1.0f;
	}
	template <typename T>
	inline T sign(const T val) {
		return val == 0 ? 0 : val < 0 ? -1 : 1;
	}
	
	inline f32 deg(const f32 rad) {
		return rad * RAD2DEG;
	}
	inline f32 rad(const f32 deg) {
		return deg * DEG2RAD;
	}
	
	f32 sin(const f32 val);
	f32 cos(const f32 val);
	f32 tan(const f32 val);
	f32 asin(const f32 val);
	f32 acos(const f32 val);
	f32 atan(const f32 val);
	f32 atan2(const f32 y, f32 x);
		
	f32 floor(const f32 val);
	f32 ceil(const f32 val);
	f32 round(const f32 val);
	
	f32 sqrt(const f32 val);
	f32 log(const f32 val);
	f32 log2(const f32 val);
	f32 log10(const f32 val);

	template<typename T>
	T clamp(T value, T min, T max) {
		if (value < min) return min;
		if (value > max) return max;
		return value;
	}

	f32 pow(const f32 val, const f32 exp);
	f32 mod(const f32 val, const f32 mod);
	
	
	inline f32 lerp(const f32 a, const f32 b, const f32 t) {
		return a + t * (b - a);
	}
	inline f32 smoothstep(const f32 edge_0, const f32 edge_1, const f32 x) {
		const f32 cl = clamp((x - edge_0) / (edge_1 - edge_0), 0.0f, 1.0f);
		return cl * cl * (3.0f - 2.0f * cl);
	}
	
    // INFO: alignment must be a power of two
    inline usz next_aligned(const usz value, const usz alignment) {
        #if !LRELEASE 
        if ((alignment & (alignment - 1)) != 0) {
            LOG_FATAL("Alignment must be a power of two")
            return value;
        };
        #endif


        return (value + alignment - 1) & ~(alignment - 1);
    }
}
