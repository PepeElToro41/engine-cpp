#pragma once

inline f32 FloatQuaternion::normal() const {
	return this->vector.magnitude();
}

inline FloatQuaternion FloatQuaternion::normalize() const {
	return FloatQuaternion { this->vector.normalize() };
}

inline FloatQuaternion FloatQuaternion::conjugate() const {
	return { 
		-this->vector.x(), 
		-this->vector.y(), 
		-this->vector.z(), 
		this->vector.w() 
	};
}

inline FloatQuaternion FloatQuaternion::inverse() const {
	return this->conjugate().normalize();
}

inline FloatQuaternion FloatQuaternion::mul(const FloatQuaternion& other) const {
	const f32* d0 = this->vector.data; 
	const f32* d1 = other.vector.data; 
	
	return {
		d0[0] * d1[3] + d0[1] * d1[2] - d0[2] * d1[1] + d0[3] * d1[0],
		-d0[0] * d1[2] + d0[1] * d1[3] + d0[2] * d1[0] + d0[3] * d1[1],
		d0[0] * d1[1] - d0[1] * d1[0] + d0[2] * d1[3] + d0[3] * d1[2],
		-d0[0] * d1[0] - d0[1] * d1[1] - d0[2] * d1[2] + d0[3] * d1[3]
	};
}

inline f32 FloatQuaternion::dot(const FloatQuaternion& other) const {
	return this->vector.dot(other.vector);
}

inline FloatQuaternion FloatQuaternion::slerp(const FloatQuaternion& other, const f32 alpha) const {
	const FloatQuaternion v0 = this->normalize();
	FloatQuaternion v1 = other.normalize();
	
	f32 dot = v0.dot(v1);
	
	if (dot < 0.0f) {
		v1.vector.data[0] = -v0.vector.data[0];
		v1.vector.data[1] = -v0.vector.data[1];
		v1.vector.data[2] = -v0.vector.data[2];
		v1.vector.data[3] = -v0.vector.data[3];
		dot = -dot;
	}
	
	constexpr f32 dot_threshold = 0.9995f;
	if (dot > dot_threshold) {
		// use lerp if the values are too close
		const FloatQuaternion lerp_quat {
			MATH::lerp(v0.vector.x(), v1.vector.x(), alpha),
			MATH::lerp(v0.vector.y(), v1.vector.y(), alpha),
			MATH::lerp(v0.vector.z(), v1.vector.z(), alpha),
			MATH::lerp(v0.vector.w(), v1.vector.w(), alpha)
		};
		return lerp_quat.normalize();
	}
	
	const f32 theta0 = MATH::acos(dot);
	const f32 theta1 = theta0 * alpha;
	
	const f32 sin_theta0 = MATH::sin(theta0);
	const f32 sin_theta1 = MATH::sin(theta1);
	
	f32 s0 = MATH::cos(theta1) - dot * sin_theta1 / sin_theta0;
	f32 s1 = sin_theta1 / sin_theta0;
	
	return {
		(v0.vector.x() * s0) + (v1.vector.x() * s1),
		(v0.vector.y() * s0) + (v1.vector.y() * s1),
		(v0.vector.z() * s0) + (v1.vector.z() * s1), 
		(v0.vector.w() * s0) + (v1.vector.w() * s1)
	};
}