#pragma once

Vector3 Vector3::add(const Vector3& other) const {
    const simd4f added = SIMD::add(this->data, other.data);

	return {
		.data = added
	};
}; 

Vector3 Vector3::sub(const Vector3& other) const {
    const simd4f substracted = SIMD::sub(this->data, other.data);

	return {
		.data = substracted
	};
};


Vector3 Vector3::mul(const Vector3& other) const {
    const simd4f multiplied = SIMD::mul(this->data, other.data);

	return {
		.data = multiplied
	};
};

Vector3 Vector3::div(const Vector3& other) const {
    const simd4f divided = SIMD::div(this->data, other.data);

    return {
    	.data = divided
    };
};

Vector3 Vector3::mul(const float scalar) const {
    const simd4f vectorized = SIMD::from_scalar(scalar);
    const simd4f multiplied = SIMD::mul(this->data, vectorized);

    Vector3 result{};
    result.data = multiplied;
    return result;
};

Vector3 Vector3::div(const float scalar) const {
    const simd4f vectorized = SIMD::from_scalar(scalar);
    const simd4f divided = SIMD::div(this->data, vectorized);

    return {
    	.data = divided
    };
};

inline Vector3 Vector3::cross(const Vector3& other) const {
	const simd4f cross = SIMD::cross(this->data, other.data);
	
	return {
		.data = cross
	};
}

float Vector3::dot(const Vector3& other) const {
    return SIMD::dot(this->data, other.data);
};

float Vector3::magnitude() const {
    return SIMD::magnitude(this->data);
};

Vector3 Vector3::normalize() const {
    return this->div(this->magnitude());
};