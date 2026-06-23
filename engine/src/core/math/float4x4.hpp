#pragma once

#include "float4.hpp"
#include <cmath>

struct Float4x4 {
	Float4 data[4];
	
	Float4x4() = default;
	Float4x4(const Float4& f1, const Float4& f2, const Float4& f3, const Float4& f4) 
		: data{ f1, f2, f3, f4 } 
	{}
	
	static Float4x4 identity() {
		return {
			{ 1, 0, 0, 0 },
			{ 0, 1, 0, 0 },
			{ 0, 0, 1, 0 },
			{ 0, 0, 0, 1 },
		}; 
	}
	
	// TODO: this should be in a separated place
	static Float4x4 ortographic_projection(const f32 left, const f32 right, const f32 bottom, const f32 top, const f32 near, const f32 far) {
		const f32 rl = 1.0f / (right - left);
		const f32 tb = 1.0f / (top - bottom);
		const f32 fn = 1.0f / (far - near);
		
		return { 
			{ 2.0f / rl,	 0	,	  0	  , -(right + left) / rl },
			{		0	   , 2.0f / tb,	  0	  , -(top + bottom) / tb },
			{		0	   ,	 0	, -2.0f / fn, -(far + near) / fn   },
			{		0	   ,	 0	,	  0	  ,			1			},
		}; 
	}
	
	// TODO: this should be in a separated place
	static Float4x4 perspective_projection(const f32 fov, const f32 aspect, const f32 near, const f32 far) {
		const f32 f = 1.0f / std::tan(fov / 2.0f);
		
		const f32 v22 = -(far + near) / (far - near);
		const f32 v32 = -(2.0f * far * near) / (far - near);
		
		return { 
			{ f / aspect,	 0	,	  0	    ,  0    },
			{		0	    ,    f    ,	  0	    ,  0    },
			{		0	    ,	 0	,     v22     ,  1    },
			{		0	    ,	 0	,	  v32		,  0    },
		}; 
	}
	
	inline Float4x4 transpose() const;
	inline Float4x4 inverse() const;
	
	inline Float4x4 add(const Float4x4& other) const;
	inline Float4x4 sub(const Float4x4& other) const;
	inline Float4x4 mul(const Float4x4& other) const;
	inline Float4 mul(const Float4& other) const;
	
	Float4x4 operator+(const Float4x4& other) const {
		return this->add(other);
	}
	Float4x4 operator-(const Float4x4& other) const {
		return this->sub(other);
	}
	Float4x4 operator*(const Float4x4& other) const {
		return this->mul(other);
	}
	Float4 operator*(const Float4& other) const {
		return this->mul(other);
	}
};

#include "float4x4.inl"