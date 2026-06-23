#pragma once

#include "simd.hpp"
#include "float4.hpp"
#include "vector4.hpp"

struct Quaternion {
	Vector4 vector;
	
	Quaternion() = default;
	explicit Quaternion(const simd4f data) : vector(data) {}
	explicit Quaternion(const Vector4& vector) : vector(vector) {}
	
	static Quaternion load(const Float4& float4) {
		return Quaternion { 
			SIMD::load(float4.data)
		};
	}
};