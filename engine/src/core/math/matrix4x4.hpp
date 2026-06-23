#pragma once
#include "vector4.hpp"
#include "float4.hpp"
#include "float4x4.hpp"

struct Matrix4x4 {
	Vector4 data[4];
	
	Matrix4x4() = default;
	Matrix4x4(const Vector4& v1, const Vector4& v2, const Vector4& v3, const Vector4& v4) 
		: data{ v1, v2, v3, v4 } 
	{}
	
	inline static Matrix4x4 identity() {
		return {
			Vector4 { SIMD::set(1, 0,0,0) },
			Vector4 { SIMD::set(0, 1,0,0) },
			Vector4 { SIMD::set(0, 0,1,0) },
			Vector4 { SIMD::set(0, 0,0,1) },
		};
	}
	inline static Matrix4x4 load(const Float4* float4) {
		return {
			Vector4::load(float4[0]),
			Vector4::load(float4[1]),
			Vector4::load(float4[2]),
			Vector4::load(float4[3]),
		};
	}
	inline static Matrix4x4 load(const Float3* float3) {
		return {
			Vector4::load(float3[0]),
			Vector4::load(float3[1]),
			Vector4::load(float3[2]),
			Vector4::load(float3[3]),
		};
	}
	
	inline static Matrix4x4 load(const Float4x4& float44) {
		return {
			Vector4::load(float44.data[0]),
			Vector4::load(float44.data[1]),
			Vector4::load(float44.data[2]),
			Vector4::load(float44.data[3])
		};
	}
	
	inline void store(Float4x4& out) const {
		this->data[0].store(out.data[0]);
		this->data[1].store(out.data[1]);
		this->data[2].store(out.data[2]);
		this->data[3].store(out.data[3]);
	}
	
	inline Matrix4x4 transpose() const;
	
	inline Matrix4x4 add(const Matrix4x4& other) const;
	inline Matrix4x4 sub(const Matrix4x4& other) const;
	
	inline Matrix4x4 mul(const Matrix4x4& other) const;
	inline Vector4 mul(const Vector4& other) const;
	
	Matrix4x4 operator+(const Matrix4x4& other) const {
		return this->add(other);
	}
	Matrix4x4 operator-(const Matrix4x4& other) const {
		return this->sub(other);
	}
	Matrix4x4 operator*(const Matrix4x4& other) const {
		return this->mul(other);
	}
	Vector4 operator*(const Vector4& other) const {
		return this->mul(other);
	}
};
	

#include "matrix4x4.inl"