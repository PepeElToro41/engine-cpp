#pragma once
#include "simd.hpp"

inline Matrix4x4 Matrix4x4::transpose() const {
	const Vector4* vectors = this->data;
	
	const simd4f t0 = SIMD::shuffle<1, 0, 1, 0>(vectors[0].data, vectors[1].data);
	const simd4f t1 = SIMD::shuffle<1, 0, 1, 0>(vectors[2].data, vectors[3].data);
	
	const simd4f t2 = SIMD::shuffle<3, 2, 3, 2>(vectors[0].data, vectors[1].data);
	const simd4f t3 = SIMD::shuffle<3, 2, 3, 2>(vectors[2].data, vectors[3].data);
	
	return {
		Vector4 { SIMD::shuffle<2, 0, 2, 0>(t0, t1) },
		Vector4 { SIMD::shuffle<3, 1, 3, 1>(t0, t1) },
		Vector4 { SIMD::shuffle<2, 0, 2, 0>(t2, t3) },
		Vector4 { SIMD::shuffle<3, 1, 3, 1>(t2, t3) },
	};
}

inline Matrix4x4 Matrix4x4::add(const Matrix4x4& other) const {
	return {
		this->data[0].add(other.data[0]),
		this->data[1].add(other.data[1]),
		this->data[2].add(other.data[2]),
		this->data[3].add(other.data[3]),
	};
}
	
inline Matrix4x4 Matrix4x4::sub(const Matrix4x4& other) const {
	return {
		this->data[0].sub(other.data[0]),
		this->data[1].sub(other.data[1]),
		this->data[2].sub(other.data[2]),
		this->data[3].sub(other.data[3]),
	};
}

namespace MATRIX_4X4 {
	inline Vector4 resolve_mul(const Matrix4x4& a, const Vector4& vec) {
		simd4f e0 = SIMD::repeat<0>(vec.data);
		simd4f e1 = SIMD::repeat<1>(vec.data);
		simd4f e2 = SIMD::repeat<2>(vec.data);
		simd4f e3 = SIMD::repeat<3>(vec.data);
	
		simd4f m0 = SIMD::mul(a.data[0].data, e0);
		simd4f m1 = SIMD::mul(a.data[1].data, e1);
		simd4f m2 = SIMD::mul(a.data[2].data, e2);
		simd4f m3 = SIMD::mul(a.data[3].data, e3);
	
		simd4f a0 = SIMD::add(m0, m1);
		simd4f a1 = SIMD::add(m2, m3);
		return Vector4 { 
			SIMD::add(a0, a1)
		};
	}
}

inline Matrix4x4 Matrix4x4::mul(const Matrix4x4& other) const {
	return {
		MATRIX_4X4::resolve_mul(*this, other.data[0]),
		MATRIX_4X4::resolve_mul(*this, other.data[1]),
		MATRIX_4X4::resolve_mul(*this, other.data[2]),
		MATRIX_4X4::resolve_mul(*this, other.data[3]),
	};
}

inline Vector4 Matrix4x4::mul(const Vector4& other) const {
	return MATRIX_4X4::resolve_mul(*this, other);
}