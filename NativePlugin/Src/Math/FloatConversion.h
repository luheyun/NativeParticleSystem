#ifndef FLOATCONVERSION_H
#define FLOATCONVERSION_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <math.h>
#include "Utilities/UnionTuple.h"

using std::sqrt;

#ifndef kPI
#define kPI 3.14159265358979323846264338327950288419716939937510F
#endif

inline float Abs(float v)
{
	return v < 0.0F ? -v : v;
}

inline double Abs(double v)
{
	return v < 0.0 ? -v : v;
}

inline int Abs(int v)
{
	return v < 0 ? -v : v;
}

inline bool IsFinite(const float& value)
{
	// Returns false if value is NaN or +/- infinity
	UInt32 intval = AliasAs<UInt32>(value).second;
	return (intval & 0x7f800000) != 0x7f800000;
}

inline float InvSqrt(float p) { return 1.0F / sqrt(p); }
inline float Sqrt(float p) { return sqrt(p); }

inline float SqrtImpl(float f)
{
	return sqrt(f);
}

// Returns true if the distance between f0 and f1 is smaller than epsilon
inline bool CompareApproximately(float f0, float f1, float epsilon = 0.000001F)
{
	float dist = (f0 - f1);
	dist = Abs(dist);
	return dist <= epsilon;
}

/// CopySignf () returns x with its sign changed to y's.
inline float CopySignf(float x, float y)
{
	union
	{
		float f;
		UInt32 i;
	} u, u0, u1;
	u0.f = x; u1.f = y;
	UInt32 a = u0.i;
	UInt32 b = u1.i;
	SInt32 mask = 1 << 31;
	UInt32 sign = b & mask;
	a &= ~mask;
	a |= sign;

	u.i = a;
	return u.f;
}

inline float FastInvSqrt(float f)
{
	// The Newton iteration trick used in FastestInvSqrt is a bit faster on
	// Pentium4 / Windows, but lower precision. Doing two iterations is precise enough,
	// but actually a bit slower.
	if (fabs(f) == 0.0F)
		return f;
	return 1.0F / sqrtf(f);
}

inline float FastestInvSqrt(float f)
{
	union
	{
		float f;
		int i;
	} u;
	float fhalf = 0.5f*f;
	u.f = f;
	int i = u.i;
	i = 0x5f3759df - (i >> 1);
	u.i = i;
	f = u.f;
	f = f*(1.5f - fhalf*f*f);
	// f = f*(1.5f - fhalf*f*f); // uncommenting this would be two iterations
	return f;
}

inline float FloatMin(float a, float b)
{
	return std::min(a, b);
}

inline float FloatMax(float a, float b)
{
	return std::max(a, b);
}

inline float Lerp(float from, float to, float t)
{
	return to * t + from * (1.0F - t);
}

// Returns the t^2
template<class T>
T Sqr(const T& t)
{
	return t * t;
}

#define kDeg2Rad (2.0F * kPI / 360.0F)
#define kRad2Deg (1.F / kDeg2Rad)

inline float Deg2Rad(float deg)
{
	// TODO : should be deg * kDeg2Rad, but can't be changed,
	// because it changes the order of operations and that affects a replay in some RegressionTests
	return deg / 360.0F * 2.0F * kPI;
}

inline float Ceilf(float f)
{
	// Use std::ceil().
	// We are interested in reliable functions that do not lose precision.
	// Casting to int and back to float would not be helpful.
	return ceil(f);
}

inline float Floorf(float f)
{
	// Use std::floor().
	// We are interested in reliable functions that do not lose precision.
	// Casting to int and back to float would not be helpful.
	return floor(f);
}

// Returns float remainder for t / length
inline float Repeat(float t, float length)
{
	return t - Floorf(t / length) * length;
}

// Returns double remainder for t / length
inline double RepeatD(double t, double length)
{
	return t - floor(t / length) * length;
}

inline UInt32 FloorfToIntPos(float f)
{
	return (UInt32)f;
}

inline UInt32 RoundfToIntPos(float f)
{
	return FloorfToIntPos(f + 0.5F);
}

///  Fast conversion of float [0...1] to 0 ... 65535
inline int NormalizedToWord(float f)
{
	f = FloatMax(f, 0.0F);
	f = FloatMin(f, 1.0F);
	return RoundfToIntPos(f * 65535.0f);
}

///  Fast conversion of float [0...1] to 0 ... 255
inline int NormalizedToByte(float f)
{
	f = FloatMax(f, 0.0F);
	f = FloatMin(f, 1.0F);
	return RoundfToIntPos(f * 255.0f);
}

///  Fast conversion of float [0...1] to 0 ... 65535
inline float WordToNormalized(int p)
{
	return (float)p / 65535.0F;
}

#endif