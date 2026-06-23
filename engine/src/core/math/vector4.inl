#pragma once

Vector4 Vector4::add(const Vector4& other) const {
    const simd4f added = SIMD::add(this->data, other.data);
	
    return Vector4 {
    	.data = added
    };
}; 

Vector4 Vector4::sub(const Vector4& other) const {
    const simd4f substracted = SIMD::sub(this->data, other.data);

    return Vector4 {
    	.data = substracted
    };
};

Vector4 Vector4::mul(const Vector4& other) const {
    const simd4f multiplied = SIMD::mul(this->data, other.data);

    return Vector4 {
    	.data = multiplied
    };
};

Vector4 Vector4::div(const Vector4& other) const {
    const simd4f divided = SIMD::div(this->data, other.data);

	return Vector4 {
		.data = divided
	};
};
 

Vector4 Vector4::mul(const float scalar) const {
    const simd4f vectorized = SIMD::from_scalar(scalar);
    const simd4f multiplied = SIMD::mul(this->data, vectorized);

    return Vector4 {
    	.data = multiplied
    };
};

Vector4 Vector4::div(const float scalar) const {
    const simd4f vectorized = SIMD::from_scalar(scalar);
    const simd4f divided = SIMD::div(this->data, vectorized);

    return Vector4 {
    	.data = divided
    };
};

inline Vector4 Vector4::cross(const Vector4& other) const {
	const simd4f cross = SIMD::cross(this->data, other.data);
	
	return Vector4 {
		.data = cross
	};
}


float Vector4::dot(const Vector4& other) const {
    return SIMD::dot(this->data, other.data);
};

float Vector4::magnitude() const {
    return SIMD::magnitude(this->data);
};

Vector4 Vector4::normalize() const {
    return this->div(this->magnitude());
};