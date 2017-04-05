#pragma once

#include "Allocator/MemoryMacros.h"
#include "Math/FloatConversion.h"

class ColorRGBAf
{
public:
	float	r, g, b, a;

	ColorRGBAf() {}

	ColorRGBAf(float inR, float inG, float inB, float inA = 1.0F) : r(inR), g(inG), b(inB), a(inA) {}
	explicit ColorRGBAf(const float* c) : r(c[0]), g(c[1]), b(c[2]), a(c[3]) {}

	template<class TransferFunction>
	void Transfer(TransferFunction& transfer);

	void Set(float inR, float inG, float inB, float inA) { r = inR; g = inG; b = inB; a = inA; }

	void SetHex(UInt32 hex)
	{
		Set(float(hex >> 24) / 255.0f,
			float((hex >> 16) & 255) / 255.0f,
			float((hex >> 8) & 255) / 255.0f,
			float(hex & 255) / 255.0f);
	}

	UInt32 GetHex() const
	{
		UInt32 hex = (NormalizedToByte(r) << 24) | (NormalizedToByte(g) << 16) | (NormalizedToByte(b) << 8) | NormalizedToByte(a);
		return hex;
	}

	float AverageRGB() const { return (r + g + b)*(1.0F / 3.0F); }
	float GreyScaleValue() const { return r * 0.30f + g * 0.59f + b * 0.11f; }

	ColorRGBAf& operator = (const ColorRGBAf& in) { Set(in.r, in.g, in.b, in.a); return *this; }

	bool Equals(const ColorRGBAf& inRGB) const
	{
		return (r == inRGB.r && g == inRGB.g && b == inRGB.b && a == inRGB.a);
	}

	bool NotEquals(const ColorRGBAf& inRGB) const
	{
		return (r != inRGB.r || g != inRGB.g || b != inRGB.b || a != inRGB.a);
	}

	float* GetPtr()				{ return &r; }
	const float* GetPtr() const	{ return &r; }

	ColorRGBAf& operator += (const ColorRGBAf &inRGBA)
	{
		r += inRGBA.r; g += inRGBA.g; b += inRGBA.b; a += inRGBA.a;
		return *this;
	}

	ColorRGBAf& operator *= (const ColorRGBAf &inRGBA)
	{
		r *= inRGBA.r; g *= inRGBA.g; b *= inRGBA.b; a *= inRGBA.a;
		return *this;
	}

private:
	// intentionally undefined
	bool operator == (const ColorRGBAf& inRGB) const;
	bool operator != (const ColorRGBAf& inRGB) const;
};

inline ColorRGBAf operator + (const ColorRGBAf& inC0, const ColorRGBAf& inC1)
{
	return ColorRGBAf(inC0.r + inC1.r, inC0.g + inC1.g, inC0.b + inC1.b, inC0.a + inC1.a);
}

inline ColorRGBAf operator * (float inScale, const ColorRGBAf& inC0)
{
	return ColorRGBAf(inC0.r * inScale, inC0.g * inScale, inC0.b * inScale, inC0.a * inScale);
}

inline ColorRGBAf operator * (const ColorRGBAf& inC0, float inScale)
{
	return ColorRGBAf(inC0.r * inScale, inC0.g * inScale, inC0.b * inScale, inC0.a * inScale);
}

inline ColorRGBAf Lerp(const ColorRGBAf& c0, const ColorRGBAf& c1, float t)
{
	return (1.0f - t) * c0 + t * c1;
}

class ALIGN_TYPE(4) ColorRGBA32
{
public:
	UInt8 r, g, b, a;

	ColorRGBA32() {}
	ColorRGBA32(UInt8 r, UInt8 g, UInt8 b, UInt8 a) { this->r = r; this->g = g; this->b = b; this->a = a; }
	ColorRGBA32(UInt32 c) { *(UInt32*)this = c; }

	UInt8* GetPtr()		{ return &r; }
	const UInt8* GetPtr()const	{ return &r; }

	UInt8& operator [] (long i) { return GetPtr()[i]; }
	const UInt8& operator [] (long i)const { return GetPtr()[i]; }

	inline ColorRGBA32 SwizzleToBGRA() const { return ColorRGBA32(b, g, r, a); }

	inline void operator *= (const ColorRGBA32& inC1)
	{
       // This is much faster, but doesn't guarantee 100% matching result (basically color values van vary 1/255 but not at ends, check out unit test in cpp file).
		UInt32& u = reinterpret_cast<UInt32&> (*this);
		const UInt32& v = reinterpret_cast<const UInt32&> (inC1);
		UInt32 result = (((u & 0x000000ff) * ((v & 0x000000ff) + 1)) >> 8) & 0x000000ff;
		result |= (((u & 0x0000ff00) >> 8) * (((v & 0x0000ff00) >> 8) + 1)) & 0x0000ff00;
		result |= (((u & 0x00ff0000) * (((v & 0x00ff0000) >> 16) + 1)) >> 8) & 0x00ff0000;
		result |= (((u & 0xff000000) >> 8) * (((v & 0xff000000) >> 24) + 1)) & 0xff000000;
		u = result;
	}
};

inline ColorRGBA32 operator * (const ColorRGBA32& inC0, const ColorRGBA32& inC1)
{
	// This is much faster, but doesn't guarantee 100% matching result (basically color values van vary 1/255 but not at ends, check out unit test in cpp file).
	const UInt32& u = reinterpret_cast<const UInt32&> (inC0);
	const UInt32& v = reinterpret_cast<const UInt32&> (inC1);
	UInt32 result = (((u & 0x000000ff) * ((v & 0x000000ff) + 1)) >> 8) & 0x000000ff;
	result |= (((u & 0x0000ff00) >> 8) * (((v & 0x0000ff00) >> 8) + 1)) & 0x0000ff00;
	result |= (((u & 0x00ff0000) * (((v & 0x00ff0000) >> 16) + 1)) >> 8) & 0x00ff0000;
	result |= (((u & 0xff000000) >> 8) * (((v & 0xff000000) >> 24) + 1)) & 0xff000000;
	return ColorRGBA32(result);
}

inline ColorRGBA32 Lerp(const ColorRGBA32& c0, const ColorRGBA32& c1, int scale)
{
	const UInt32& u0 = reinterpret_cast<const UInt32&> (c0);
	const UInt32& u1 = reinterpret_cast<const UInt32&> (c1);
	UInt32 vx = u0 & 0x00ff00ff;
	UInt32 rb = vx + ((((u1 & 0x00ff00ff) - vx) * scale) >> 8) & 0x00ff00ff;
	vx = u0 & 0xff00ff00;
	return ColorRGBA32(rb | (vx + ((((u1 >> 8) & 0x00ff00ff) - (vx >> 8)) * scale) & 0xff00ff00));
}

inline ColorRGBA32 SwizzleColorForPlatform(const ColorRGBA32& col) { return col.SwizzleToBGRA(); }
