#pragma once

#include <cmath>
#include "simd.hpp"
#include "defines.hpp"

struct alignas(SIMD_ALIGNMENT) Float4 {
    f32 data[4];

    Float4(const f32 x, const f32 y, const f32 z, const f32 w) 
		: data { x, y, z, w} 
	{}
    Float4() 
		: data { 0.0f, 0.0f, 0.0f, 0.0f }
	{}

    inline f32 x() const {
        return this->data[0];
    }
    inline f32 y() const {
        return this->data[1];
    }
    inline f32 z() const {
        return this->data[2];
    }
    inline f32 w() const {
        return this->data[3];
    }
	
	Float4 add(const Float4& other) const {
    	return {
    		this->data[0] + other.data[0], 
			this->data[1] + other.data[1],
			this->data[2] + other.data[2],
    		this->data[3] + other.data[3]
		};
    }
	Float4 sub(const Float4& other) const {
    	return {
    		this->data[0] - other.data[0], 
			this->data[1] - other.data[1], 
			this->data[2] - other.data[2],
    		this->data[3] - other.data[3]
		};
    }
	Float4 mul(const Float4& other) const {
    	return {
    		this->data[0] * other.data[0],
			this->data[1] * other.data[1],
			this->data[2] * other.data[2],
			this->data[3] * other.data[3]
		};
    }
	Float4 mul(const f32 scalar) const {
    	return {
    		this->data[0] * scalar, 
			this->data[1] * scalar,
			this->data[2] * scalar,
			this->data[3] * scalar,
		};
    }
	Float4 div(const Float4& other) const {
    	return {
    		this->data[0] / other.data[0],
			this->data[1] / other.data[1],
			this->data[2] / other.data[2],
			this->data[3] / other.data[3]
		};
    }
	Float4 div(const f32 scalar) const {
    	return {
    		this->data[0] / scalar,
			this->data[1] / scalar,
			this->data[2] / scalar,
			this->data[3] / scalar
		};
    }
	inline Float4 negate() const {
	    return {
		    -this->data[0],
			-this->data[1],
	    	-this->data[2],
	    	-this->data[3],
	    };
    }
	
	f32 dot(const Float4& other) const {
    	return this->data[0] * other.data[0] + this->data[1] * other.data[1] + this->data[2] * other.data[2] + this->data[3] * other.data[3];
    }
	f32 magnitude() const {
    	return std::sqrt(this->dot(*this));
    }
	Float4 normalize() const {
    	return this->div(this->magnitude());
    }
	
	Float4 operator+(const Float4& other) const {
    	return this->add(other);
    }
	Float4 operator-(const Float4& other) const {
    	return this->add(other);
    }
	Float4 operator-() const {
	    return this->negate();
    }
	Float4 operator*(const Float4& other) const {
    	return this->mul(other);
    }
	Float4 operator*(const f32 scalar) const {
    	return this->mul(scalar);
    }
	Float4 operator/(const Float4& other) const {
    	return this->div(other);
    }
	Float4 operator/(const f32 scalar) const {
    	return this->div(scalar);
    }
};