#pragma once

#include "float4.hpp"
#include "float3.hpp"
#include "math.hpp"


struct alignas(SIMD_ALIGNMENT) FloatQuaternion {
	Float4 vector;

	FloatQuaternion(const f32 x, const f32 y, const f32 z, const f32 w) 
		: vector (x, y, z, w) 
	{}
	explicit FloatQuaternion(const Float4 vector) : vector(vector) {}
	
	static FloatQuaternion from_axis(const Float3 axis, const f32 angle, const bool normalize) {
		const f32 half_angle = 0.5f * angle;
		const f32 s = MATH::sin(half_angle);
		const f32 c = MATH::cos(half_angle);

		if (normalize) {
			return FloatQuaternion {s * axis.x(), s * axis.y(), s * axis.z(), c}.normalize();
		}
		const FloatQuaternion out {s * axis.x(), s * axis.y(), s * axis.z(), c};
		return out;
	}
	static FloatQuaternion identity() {
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}
	
	inline f32 normal() const;
	inline FloatQuaternion normalize() const;
	inline FloatQuaternion conjugate() const;
	inline FloatQuaternion inverse() const;
	inline FloatQuaternion mul(const FloatQuaternion& other) const;
	inline f32 dot(const FloatQuaternion& other) const;
	inline FloatQuaternion slerp(const FloatQuaternion& other, const f32 alpha) const;
};

#include "float_quat.inl"