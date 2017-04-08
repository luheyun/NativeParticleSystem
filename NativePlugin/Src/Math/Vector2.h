#pragma once

#include "Math/FloatConversion.h"

class Vector2f
{
public:
	float x, y;

	Vector2f() : x(0.f), y(0.f) {}
    Vector2f(float x, float y) { this->x = x; this->y = y; }
	explicit Vector2f(const float* p)			{ x = p[0]; y = p[1]; }

	void Set(float inX, float inY)				{ x = inX; y = inY; }

	float& operator[] (int i)					{ return (&x)[i]; }
	const float& operator[] (int i)const		{ return (&x)[i]; }
	Vector2f& operator *= (const float s) { x *= s; y *= s; return *this; }
    Vector2f& operator /= (const float s) { x /= s; y /= s; return *this; }

	static const Vector2f zero;
    static const float epsilon;
};

inline Vector2f operator / (const Vector2f& inV, float s) { Vector2f temp(inV); temp /= s; return temp; }
inline float Dot(const Vector2f& lhs, const Vector2f& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }
inline float Magnitude(const Vector2f& inV) { return SqrtImpl(Dot(inV, inV)); }

inline Vector2f NormalizeSafe(const Vector2f& inV, const Vector2f& defaultV = Vector2f::zero)
{
    float mag = Magnitude(inV);
    if (mag > Vector2f::epsilon)
        return inV / Magnitude(inV);
    else
        return defaultV;
}

inline Vector2f operator * (const Vector2f& inV, float s)					{ return Vector2f(inV.x * s, inV.y * s); }
inline Vector2f operator * (const float s, const Vector2f& inV)				{ return Vector2f(inV.x * s, inV.y * s); }
