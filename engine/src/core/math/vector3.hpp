#pragma once

#include <cmath>
#include "simd.hpp"
#include "defines.hpp"
#include "float3.hpp"

struct alignas(SIMD_ALIGNMENT) Vector3 {
    simd4f data;

    static Vector3 load(const Float3* float3) {
        const Vector3 result{
			.data = SIMD::load(float3->data)
        };
        return result;
    }
	
	static Vector3 zero() {
    	return {
    		.data = SIMD::zero()
		};
    }

    void store(Float3& out) const {
        SIMD::store(out.data, this->data);
    }

    inline Vector3 add(const Vector3& other) const;
    inline Vector3 sub(const Vector3& other) const;
    inline Vector3 mul(const Vector3& other) const;
    inline Vector3 div(const Vector3& other) const;

    inline Vector3 mul(f32 scalar) const;
    inline Vector3 div(f32 scalar) const;

    inline f32 dot(const Vector3& other) const; 
    inline f32 magnitude() const;
    inline Vector3 cross(const Vector3& other) const;
    inline Vector3 normalize() const;

    inline f32 x() const {
        return SIMD::extract_first(this->data);
    }
    inline f32 y() const {
        return SIMD::extract<1>(this->data);
    }
    inline f32 z() const {
        return SIMD::extract<2>(this->data);
    }

    inline Vector3 operator+(const Vector3& other) const {
        return this->add(other);
    }
    inline Vector3 operator-(const Vector3& other) const {
        return this->sub(other);
    }
    inline Vector3 operator*(const Vector3& other) const {
        return this->mul(other);
    }
    inline Vector3 operator/(const Vector3& other) const {
        return this->div(other);
    }

    inline Vector3 operator*(const f32 scalar) const {
        return this->mul(scalar);
    }
    inline Vector3 operator/(const f32 scalar) const {
        return this->div(scalar);
    }
};

#include "vector3.inl"