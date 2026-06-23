#include <cmath>
#include "simd.hpp"
#include "defines.hpp"

struct alignas(SIMD_ALIGNMENT) Float3 {
    f32 data[4];

    Float3(const f32 x, const f32 y, const f32 z) 
		: data { x, y, z, 0.0f} 
	{}
    Float3() 
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
	
	inline Float3 add(const Float3& other) const {
    	return {
    		this->data[0] + other.data[0], 
			this->data[1] + other.data[1],
			this->data[2] + other.data[2]
		};
    }
	inline Float3 sub(const Float3& other) const {
    	return {
    		this->data[0] - other.data[0], 
			this->data[1] - other.data[1], 
			this->data[2] - other.data[2]
		};
    }
	inline Float3 mul(const Float3& other) const {
    	return {
    		this->data[0] * other.data[0],
			this->data[1] * other.data[1],
			this->data[2] * other.data[2]
		};
    }
	inline Float3 mul(const f32 scalar) const {
    	return {
    		this->data[0] * scalar, 
			this->data[1] * scalar,
			this->data[2] * scalar
		};
    }
	inline Float3 div(const Float3& other) const {
    	return {
    		this->data[0] / other.data[0],
			this->data[1] / other.data[1],
			this->data[2] / other.data[2]
		};
    }
	inline Float3 div(const f32 scalar) const {
    	return {
    		this->data[0] / scalar,
			this->data[1] / scalar,
			this->data[2] / scalar
		};
    }
	inline Float3 negate() const {
    	return {
    		-this->data[0],
			-this->data[1],
			-this->data[2]
		};
    }

	Float3 cross(const Float3& other) const {
    	return {
    		this->data[1] * other.data[2] - this->data[2] * other.data[1],
			this->data[2] * other.data[0] - this->data[0] * other.data[2],
			this->data[0] * other.data[1] - this->data[1] * other.data[0]
		};
    }
	
	f32 dot(const Float3& other) const {
    	return this->data[0] * other.data[0] + this->data[1] * other.data[1] + this->data[2] * other.data[2];
    }
	f32 magnitude() const {
    	return std::sqrt(this->dot(*this));
    }
	Float3 normalize() const {
    	return this->div(this->magnitude());
    }
	
	Float3 operator+(const Float3& other) const {
    	return this->add(other);
    }
	Float3 operator-(const Float3& other) const {
    	return this->sub(other);
    }
	Float3 operator-() const {
    	return this->negate();
    }
	Float3 operator*(const Float3& other) const {
    	return this->mul(other);
    }
	Float3 operator*(const f32 scalar) const {
    	return this->mul(scalar);
    }
	Float3 operator/(const Float3& other) const {
    	return this->div(other);
    }
	Float3 operator/(const f32 scalar) const {
    	return this->div(scalar);
    }
};