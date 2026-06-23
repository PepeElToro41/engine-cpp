#pragma once
#include "math.hpp"

inline Float4x4 Float4x4::transpose() const {
	const Float4* floats = this->data;
	
	return {
		{ floats[0].data[0], floats[1].data[0], floats[2].data[0], floats[3].data[0] },
		{ floats[0].data[1], floats[1].data[1], floats[2].data[1], floats[3].data[1] },
		{ floats[0].data[2], floats[1].data[2], floats[2].data[2], floats[3].data[2] },
		{ floats[0].data[3], floats[1].data[3], floats[2].data[3], floats[3].data[3] },
	};
}

inline Float4x4 Float4x4::inverse() const {
	const Float4* m = this->data;

	const f32 t0  = m[2].data[2] * m[3].data[3];
	const f32 t1  = m[3].data[2] * m[2].data[3];
	const f32 t2  = m[1].data[2] * m[3].data[3];
	const f32 t3  = m[3].data[2] * m[1].data[3];
	const f32 t4  = m[1].data[2] * m[2].data[3];
	const f32 t5  = m[2].data[2] * m[1].data[3];
	const f32 t6  = m[0].data[2] * m[3].data[3];
	const f32 t7  = m[3].data[2] * m[0].data[3];
	const f32 t8  = m[0].data[2] * m[2].data[3];
	const f32 t9  = m[2].data[2] * m[0].data[3];
	const f32 t10 = m[0].data[2] * m[1].data[3];
	const f32 t11 = m[1].data[2] * m[0].data[3];
	const f32 t12 = m[2].data[0] * m[3].data[1];
	const f32 t13 = m[3].data[0] * m[2].data[1];
	const f32 t14 = m[1].data[0] * m[3].data[1];
	const f32 t15 = m[3].data[0] * m[1].data[1];
	const f32 t16 = m[1].data[0] * m[2].data[1];
	const f32 t17 = m[2].data[0] * m[1].data[1];
	const f32 t18 = m[0].data[0] * m[3].data[1];
	const f32 t19 = m[3].data[0] * m[0].data[1];
	const f32 t20 = m[0].data[0] * m[2].data[1];
	const f32 t21 = m[2].data[0] * m[0].data[1];
	const f32 t22 = m[0].data[0] * m[1].data[1];
	const f32 t23 = m[1].data[0] * m[0].data[1];
	

	const f32 d00 = (t0 * m[1].data[1] + t3 * m[2].data[1] + t4 * m[3].data[1]) -
			  (t1 * m[1].data[1] + t2 * m[2].data[1] + t5 * m[3].data[1]);
	const f32 d01 = (t1 * m[0].data[1] + t6 * m[2].data[1] + t9 * m[3].data[1]) -
			  (t0 * m[0].data[1] + t7 * m[2].data[1] + t8 * m[3].data[1]);
	const f32 d02 = (t2 * m[0].data[1] + t7 * m[1].data[1] + t10 * m[3].data[1]) -
			  (t3 * m[0].data[1] + t6 * m[1].data[1] + t11 * m[3].data[1]);
	const f32 d03 = (t5 * m[0].data[1] + t8 * m[1].data[1] + t11 * m[2].data[1]) -
			  (t4 * m[0].data[1] + t9 * m[1].data[1] + t10 * m[2].data[1]);

	const f32 d = 1.0f / (m[0].data[0] * d00 + m[1].data[0] * d01 +
					m[2].data[0] * d02 + m[3].data[0] * d03);

	if (MATH::abs(d) < 1e-6f) {
		return Float4x4::identity();
	}
	
	Float4x4 out_matrix;
	Float4* o = out_matrix.data;
	
	o[0].data[0] = d * d00;
	o[0].data[1] = d * d01;
	o[0].data[2] = d * d02;
	o[0].data[3] = d * d03;

	o[1].data[0] = d * ((t1 * m[1].data[0] + t2 * m[2].data[0] + t5 * m[3].data[0]) -
				   (t0 * m[1].data[0] + t3 * m[2].data[0] + t4 * m[3].data[0]));
	o[1].data[1] = d * ((t0 * m[0].data[0] + t7 * m[2].data[0] + t8 * m[3].data[0]) -
				   (t1 * m[0].data[0] + t6 * m[2].data[0] + t9 * m[3].data[0]));
	o[1].data[2] = d * ((t3 * m[0].data[0] + t6 * m[1].data[0] + t11 * m[3].data[0]) -
				   (t2 * m[0].data[0] + t7 * m[1].data[0] + t10 * m[3].data[0]));
	o[1].data[3] = d * ((t4 * m[0].data[0] + t9 * m[1].data[0] + t10 * m[2].data[0]) -
				   (t5 * m[0].data[0] + t8 * m[1].data[0] + t11 * m[2].data[0]));

	o[2].data[0] = d * ((t12 * m[1].data[3] + t15 * m[2].data[3] + t16 * m[3].data[3]) -
				   (t13 * m[1].data[3] + t14 * m[2].data[3] + t17 * m[3].data[3]));
	o[2].data[1] = d * ((t13 * m[0].data[3] + t18 * m[2].data[3] + t21 * m[3].data[3]) -
				   (t12 * m[0].data[3] + t19 * m[2].data[3] + t20 * m[3].data[3]));
	o[2].data[2] = d * ((t14 * m[0].data[3] + t19 * m[1].data[3] + t22 * m[3].data[3]) -
				   (t15 * m[0].data[3] + t18 * m[1].data[3] + t23 * m[3].data[3]));
	o[2].data[3] = d * ((t17 * m[0].data[3] + t20 * m[1].data[3] + t23 * m[2].data[3]) -
				   (t16 * m[0].data[3] + t21 * m[1].data[3] + t22 * m[2].data[3]));

	o[3].data[0] = d * ((t14 * m[2].data[2] + t17 * m[3].data[2] + t13 * m[1].data[2]) -
				   (t16 * m[3].data[2] + t12 * m[1].data[2] + t15 * m[2].data[2]));
	o[3].data[1] = d * ((t20 * m[3].data[2] + t12 * m[0].data[2] + t19 * m[2].data[2]) -
				   (t18 * m[2].data[2] + t21 * m[3].data[2] + t13 * m[0].data[2]));
	o[3].data[2] = d * ((t18 * m[1].data[2] + t23 * m[3].data[2] + t15 * m[0].data[2]) -
				   (t22 * m[3].data[2] + t14 * m[0].data[2] + t19 * m[1].data[2]));
	o[3].data[3] = d * ((t22 * m[2].data[2] + t16 * m[0].data[2] + t21 * m[1].data[2]) -
				   (t20 * m[1].data[2] + t23 * m[2].data[2] + t17 * m[0].data[2]));

	return out_matrix;
}

inline Float4x4 Float4x4::add(const Float4x4& other) const {
	return {
		this->data[0].add(other.data[0]),
		this->data[1].add(other.data[1]),
		this->data[2].add(other.data[2]),
		this->data[3].add(other.data[3]),
	};
}
	
inline Float4x4 Float4x4::sub(const Float4x4& other) const {
	return {
		this->data[0].sub(other.data[0]),
		this->data[1].sub(other.data[1]),
		this->data[2].sub(other.data[2]),
		this->data[3].sub(other.data[3]),
	};
}

inline Float4x4 Float4x4::mul(const Float4x4& other) const {
	Float4x4 result;
	const Float4* a_data = this->data;
	const Float4* b_data = other.data;
	Float4* out_data = result.data;
	
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			out_data[i].data[j] = 
				a_data[i].data[0] * b_data[0].data[j] +
				a_data[i].data[1] * b_data[1].data[j] +
				a_data[i].data[2] * b_data[2].data[j] +
				a_data[i].data[3] * b_data[3].data[j];
		}
	}
	
	return result;
}

inline Float4 Float4x4::mul(const Float4& other) const {
	Float4 result;
	const Float4* a_data = this->data;
	
	for (int i = 0; i < 4; ++i) {
		result.data[i] = 
			a_data[i].data[0] * other.data[0] +
			a_data[i].data[1] * other.data[1] +
			a_data[i].data[2] * other.data[2] +
			a_data[i].data[3] * other.data[3];
	}
	
	return result;
}