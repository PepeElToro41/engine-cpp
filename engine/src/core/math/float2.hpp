#include <cmath>
#include "defines.hpp"

struct Float2 {
    f32 data[2];

    Float2(f32 x, f32 y) {
        this->data[0] = x;
        this->data[1] = y;
    }

    Float2() {
        this->data[0] = 0;
        this->data[1] = 0;
    }

    f32 x() const {
        return this->data[0];
    }
    f32 y() const {
        return this->data[1];
    }
    
    Float2 operator+(const Float2& other) const {
        return Float2(this->data[0] + other.data[0], this->data[1] + other.data[1]);
    }
    Float2 operator-(const Float2& other) const {
        return Float2(this->data[0] - other.data[0], this->data[1] - other.data[1]);
    }
    Float2 operator*(const Float2& other) const {
        return Float2(this->data[0] * other.data[0], this->data[1] * other.data[1]);
    }
    Float2 operator/(const Float2& other) const {
        return Float2(this->data[0] / other.data[0], this->data[1] / other.data[1]);
    }
    Float2 operator*(const f32 scalar) const {
        return Float2(this->data[0] * scalar, this->data[1] * scalar);
    }
    Float2 operator/(const f32 scalar) const {
        return Float2(this->data[0] / scalar, this->data[1] / scalar);
    }

    f64 dot(const Float2& other) const {
        return this->data[0] * other.data[0] + this->data[1] * other.data[1];
    }
    f64 magnitude() const {
        return std::sqrt(this->data[0] * this->data[0] + this->data[1] * this->data[1]);
    }
};