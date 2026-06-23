#pragma once

#include "simd.hpp"
#include "defines.hpp"
#include "float4.hpp"
#include "float3.hpp"

struct alignas(SIMD_ALIGNMENT) Vector4 {
    simd4f data;

	Vector4() = default;
	explicit Vector4(const simd4f data) : data(data) {}
		
    static Vector4 load(const Float4& float4) {
		return Vector4 { 
			SIMD::load(float4.data)
		};
    }
	static Vector4 load(const Float3& float3) {
		return Vector4 { 
			SIMD::load(float3.data)
		};
    }
	static Vector4 zero() {
		return Vector4 {
			SIMD::zero()
		};
    }

	void store(Float4& out) const {
    	SIMD::store(out.data, this->data);
    }

    inline Vector4 add(const Vector4& other) const;
    inline Vector4 sub(const Vector4& other) const;
    inline Vector4 mul(const Vector4& other) const;
    inline Vector4 div(const Vector4& other) const;

    inline Vector4 mul(f32 scalar) const;
    inline Vector4 div(f32 scalar) const;

    inline f32 dot(const Vector4& other) const; 
    inline f32 magnitude() const;
	inline Vector4 cross(const Vector4& other) const;
    inline Vector4 normalize() const;

    inline f32 x() const {
        return SIMD::extract_first(this->data);
    }
    inline f32 y() const {
        return SIMD::extract<1>(this->data);
    }
    inline f32 z() const {
        return SIMD::extract<2>(this->data);
    }
    inline f32 w() const {
        return SIMD::extract<3>(this->data);
    }

    inline Vector4 operator+(const Vector4& other) const {
        return this->add(other);
    }
    inline Vector4 operator-(const Vector4& other) const {
        return this->sub(other);
    }
    inline Vector4 operator*(const Vector4& other) const {
        return this->mul(other);
    }
    inline Vector4 operator/(const Vector4& other) const {
        return this->div(other);
    }

    inline Vector4 operator*(const f32 scalar) const {
        return this->mul(scalar);
    }
    inline Vector4 operator/(const f32 scalar) const {
        return this->div(scalar);
    }
};

#include "vector4.inl"