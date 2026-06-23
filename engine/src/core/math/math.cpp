#include "math.hpp"

#include <cmath>


f32 MATH::sin(const f32 val) {
	return std::sin(val);
}

f32 MATH::cos(const f32 val) {
	return std::cos(val);
}

f32 MATH::tan(const f32 val) {
	return std::tan(val);
}

f32 MATH::asin(const f32 val) {
	return std::asin(val);
}

f32 MATH::acos(const f32 val) {
	return std::acos(val);
}

f32 MATH::atan(const f32 val) {
	return std::atan(val);
}

f32 MATH::atan2(const f32 y, f32 x) {
	return std::atan2(y, x);
}

f32 MATH::floor(const f32 val) {
	return std::floor(val);
}

f32 MATH::ceil(const f32 val) {
	return std::ceil(val);
}

f32 MATH::round(const f32 val) {
	return std::round(val);
}

f32 MATH::sqrt(const f32 val) {
	return std::sqrt(val);
}

f32 MATH::log(const f32 val) {
	return std::log(val);
}

f32 MATH::log2(const f32 val) {
	return std::log2(val);
}

f32 MATH::log10(const f32 val) {
	return std::log10(val);
}

f32 MATH::pow(const f32 val, const f32 exp) {
	return std::pow(val, exp);
}

f32 MATH::mod(const f32 val, const f32 mod) {
	return std::fmodf(val, mod);
}