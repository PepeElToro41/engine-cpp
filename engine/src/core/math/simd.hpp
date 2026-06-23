#pragma once
#include "defines.hpp"

#define SIMD_ALIGNMENT 16

#define SIMD_USE_SSE 1
#define SIMD_USE_NEON 0

#if SIMD_USE_SSE

#include <immintrin.h>

typedef  __m128i simd4i;
typedef  __m128i simd4u;
typedef  __m128  simd4f;
typedef  __m128d simd2d;

namespace SIMD {
    inline simd4f load(const float* ptr) {
        return _mm_load_ps(ptr);
    }
	
	inline simd4f set(const float a, const float b, const float c, const float d) {
    	return _mm_setr_ps(a, b, c, d);
    }

	inline simd4f zero() {
	    return _mm_setzero_ps();
    }
		
    inline void store(float* to, const simd4f& from) {
        _mm_store_ps(to, from);
    }

    inline simd4f add(const simd4f a, const simd4f b) {
        return _mm_add_ps(a, b);
    }
    inline simd4f sub(const simd4f a, const simd4f b) {
        return _mm_sub_ps(a, b);
    }

    inline simd4f from_scalar(const float scalar) {
        return _mm_set1_ps(scalar);
    }

    inline simd4f mul(const simd4f a, const simd4f b) {
        return _mm_mul_ps(a, b);
    }
    
    inline simd4f div(const simd4f a, const simd4f b) {
        return _mm_div_ps(a, b);
    }

    inline float dot(const simd4f a, const simd4f b) {
        const simd4f mul = _mm_mul_ps(a, b);
        const simd4f shuf1 = _mm_movehl_ps(mul, mul);
        const simd4f sum1  = _mm_add_ps(mul, shuf1);
        
        const simd4f shuf2 = _mm_shuffle_ps(sum1, sum1, 1);
        const simd4f sum2  = _mm_add_ss(sum1, shuf2);
        
        return _mm_cvtss_f32(sum2);
    }

    inline simd4f cross(const simd4f a, const simd4f b) {
        simd4f t1 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 0, 2, 1));
        t1 = _mm_mul_ps(t1, a);
        simd4f t2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 2, 1));
        t2 = _mm_mul_ps(t2, b);
        const simd4f t3 = _mm_sub_ps(t1, t2);
        return _mm_shuffle_ps(t3, t3, _MM_SHUFFLE(0, 0, 2, 1));
    }
    
    inline float sqrt(const float a) {
        const simd4f result = _mm_sqrt_ss(_mm_set_ss(a));
        return _mm_cvtss_f32(result);
    }

    inline float magnitude(const simd4f a) {
        const float dot_result = dot(a, a);
        return sqrt(dot_result);
    }
 
    inline float extract_first(const simd4f a) {
        return _mm_cvtss_f32(a);
    }

    template <int INDEX>
    inline float extract(const simd4f a) {
        const simd4f shuffled = _mm_shuffle_ps(a, a, _MM_SHUFFLE(INDEX, INDEX, INDEX, INDEX));
        return _mm_cvtss_f32(shuffled);
    }
	
	template <u32 AXIS>
	inline simd4f repeat(const simd4f a) {
    	return _mm_shuffle_ps(a, a, _MM_SHUFFLE(AXIS, AXIS, AXIS, AXIS));
    }
	
	template <u32 A, u32 B, u32 C, u32 D>
	inline simd4f shuffle(const simd4f a, const simd4f b) {
	    return _mm_shuffle_ps(a, b, _MM_SHUFFLE(A, B, C, D));
    }
	
	
}

#elif SIMD_USE_NEON

#include <arm_neon.h>

typedef int32x4_t   simd4i;
typedef uint32x4_t  simd4u;
typedef float32x4_t simd4f; 
typedef float64x2_t simd2d;

// Generic shuffle vector template
template <unsigned I1, unsigned I2, unsigned I3, unsigned I4>
inline simd4f NEON_SHUFFLE_4F(simd4f a, simd4f b) {
	simd4f lo = vcopy_laneq_f32(vdup_n_f32(0), 0, I1 >= 4? b : a, I1 & 0b11);
	lo = vcopy_laneq_f32(lo, 1, I2 >= 4? b : a, I2 & 0b11);

	simd4f hi = vcopy_laneq_f32(vdup_n_f32(0), 0, I3 >= 4? b : a, I3 & 0b11);
	hi = vcopy_laneq_f32(hi, 1, I4 >= 4? b : a, I4 & 0b11);

	return vcombine_f32(lo, hi);
}

// Specific shuffle used for cross product
inline simd4f NEON_SHUFFLE_1200(simd4f inV1, simd4f inV2) {
    return vcopyq_laneq_f32(vextq_f32(inV1, inV1, 1), 2, inV1, 0);
}

namespace SIMD {
    inline simd4f load(const float* ptr) {
        return vld1q_f32(ptr);
    }

    inline simd4f loadu(const float* ptr) {
        return vld1q_f32(ptr);
    }

    inline void store(float* ptr, simd4f& out) {
        vst1q_f32(ptr, out);
    }
    inline void storeu(float* ptr, simd4f& out) {
        vst1q_f32(ptr, out);
    }

    inline simd4f mul(simd4f a, simd4f b) {
        return vmulq_f32(a, b);
    }

    inline simd4f add(simd4f a, simd4f b) {
        return vaddq_f32(a, b);
    }

    inline simd4f sub(simd4f a, simd4f b) {
        return vsubq_f32(a, b);
    }

    inline simd4f div(simd4f a, simd4f b) {
        return vdivq_f32(a, b);
    }

    inline float dot(simd4f a, simd4f b) {
        simd4f mul = vmulq_f32(a, b);
        return vaddvq_f32(mul);
    }

    inline simd4f cross(simd4f a, simd4f b) {
        simd4f t1 = NEON_SHUFFLE_1200(b, b);
        t1 = vmulq_f32(t1, a);
        simd4f t2 = NEON_SHUFFLE_1200(a, a);
        t2 = vmulq_f32(t2, b);
        simd4f t3 = vsubq_f32(t1, t2);
        return NEON_SHUFFLE_1200(t3, t3);
    }

    inline float magnitude(simd4f a) {
        float dot_result = dot(a, a);
        simd4f result = vsqrtq_f32(vdup_n_f32(dot_result));
        return vget_lane_f32(result, 0);
    }

    inline float extract_first(simd4f a) {
        return vgetq_lane_f32(a, 0);
    }

    template <int Index>
    inline float extract(simd4f a) {
        return vgetq_lane_f32(a, Index);
    }
}

#else

// TODO: no SIMD support

#include <cstdint>

typedef int32_t simd4i[4];
typedef uint32_t simd4u[4];
typedef float simd4f[4];
typedef double simd2d[4];

#endif



