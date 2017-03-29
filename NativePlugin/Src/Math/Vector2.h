#pragma once

class Vector2f
{
public:
	float x, y;

    Vector2f(float x, float y) { this->x = x; this->y = y; }
	explicit Vector2f(const float* p)			{ x = p[0]; y = p[1]; }

	float& operator[] (int i)					{ return (&x)[i]; }
	const float& operator[] (int i)const		{ return (&x)[i]; }
	Vector2f& operator *= (const float s) { x *= s; y *= s; return *this; }

	static const Vector2f zero;
};

inline Vector2f operator * (const Vector2f& inV, float s)					{ return Vector2f(inV.x * s, inV.y * s); }
inline Vector2f operator * (const float s, const Vector2f& inV)				{ return Vector2f(inV.x * s, inV.y * s); }
