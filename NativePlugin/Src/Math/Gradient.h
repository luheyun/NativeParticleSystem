#ifndef GRADIENT_H
#define GRADIENT_H

#include "Color.h"
#include "FloatConversion.h"

enum
{
	kGradientMaxNumKeys = 8,
	kOptimizedGradientMaxNumKeys = kGradientMaxNumKeys + kGradientMaxNumKeys, // color keys + alpha keys
};

// Optimized version of gradient
struct OptimizedGradient
{
	static inline UInt32 InverseLerpWordOptimized (UInt32 from, UInt32 rcp, UInt32 v)
	{
		return ((v - from) * rcp)>>16;
	}

	inline ColorRGBA32 Evaluate(float normalizedTime) const
	{
		UInt32 time = NormalizedToWord(normalizedTime);

		// Color blend
		const UInt32 numKeys = keyCount;
		time = std::min(std::max((UInt32)times[0], time), (UInt32)times[keyCount-1]); // TODO: Is this necessary?
		for (UInt32 i = 1; i < numKeys; i++)
		{
			const UInt32 currTime = times[i];
			if(time <= currTime)
			{
				const UInt32 prevTime = times[i-1];
				const UInt32 frac = InverseLerpWordOptimized(prevTime, rcp[i], time);
				return Lerp (colors[i-1], colors[i], frac);
			}
		}
		return ColorRGBA32 (0xff,0xff,0xff,0xff);
	}

	ColorRGBA32 colors[kOptimizedGradientMaxNumKeys];
	UInt32 times[kOptimizedGradientMaxNumKeys];
	UInt32 rcp[kOptimizedGradientMaxNumKeys]; // precomputed reciprocals
	UInt32 keyCount;
};

// Work in progress (Rename NEW to something else when found..)
class GradientNEW
{
public:
	GradientNEW ();
	~GradientNEW ();

	ColorRGBA32 Evaluate(float time) const;

	struct ColorKey
	{
		ColorKey () {}
		ColorKey (ColorRGBAf color, float time) {m_Color = color; m_Time = time;}
		ColorRGBAf	m_Color;
		float		m_Time;
	};

	struct AlphaKey
	{
		AlphaKey () {}
		AlphaKey (float alpha, float time) {m_Alpha = alpha; m_Time = time;}
		float		m_Alpha;
		float		m_Time;
	};

	void SetKeys (ColorKey* colorKeys, unsigned numColorKeys, AlphaKey* alphaKeys, unsigned numAlphaKeys);

	void SetColorKeys (ColorKey* colorKeys, unsigned numKeys);
	void SetAlphaKeys (AlphaKey* alphaKeys, unsigned numKeys);

	void SetNumColorKeys (int numColorKeys) { m_NumColorKeys = numColorKeys;};
	void SetNumAlphaKeys (int numAlphaKeys) { m_NumAlphaKeys = numAlphaKeys; };

	int GetNumColorKeys () const { return m_NumColorKeys; }
	int GetNumAlphaKeys () const { return m_NumAlphaKeys; }

	ColorRGBA32& GetKey (unsigned index) { return m_Keys[index]; }
	const ColorRGBA32& GetKey (unsigned index) const { return m_Keys[index]; }

	UInt16& GetColorTime (unsigned index) { return m_ColorTime[index]; }
	const UInt16& GetColorTime (unsigned index) const { return m_ColorTime[index]; }
	
	UInt16& GetAlphaTime(unsigned index) { return m_AlphaTime[index]; }
	const UInt16& GetAlphaTime(unsigned index) const { return m_AlphaTime[index]; }

	ColorRGBA32 GetConstantColor () const;
	void SetConstantColor (ColorRGBA32 color);

	void SwapColorKeys (int i, int j);
	void SwapAlphaKeys (int i, int j);

	void InitializeOptimized(OptimizedGradient& g); 

private:
	static inline UInt32 InverseLerpWord (UInt32 from, UInt32 to, UInt32 v)
	{
		UInt32 nom = (v - from) << 16;
		UInt32 den = std::max<UInt32>(to - from, 1);
		UInt32 res = nom / den;
		return res;
	}

	static inline UInt32 LerpByte(UInt32 u0, UInt32 u1, UInt32 scale)
	{
		//DebugAssert((scale & 0xff) == scale);
		return u0 + (((u1 - u0) * scale) >> 8) & 0xff;
	}
	
	void ValidateColorKeys();
	void ValidateAlphaKeys();

	ColorRGBA32 m_Keys[kGradientMaxNumKeys];
	UInt16 m_ColorTime[kGradientMaxNumKeys]; 
	UInt16 m_AlphaTime[kGradientMaxNumKeys];
	UInt8 m_NumColorKeys;
	UInt8 m_NumAlphaKeys;
};

inline ColorRGBA32 GradientNEW::Evaluate(float normalizedTime) const
{
	ColorRGBA32 color = ColorRGBA32 (0xff,0xff,0xff,0xff);
	const UInt32 time = NormalizedToWord(normalizedTime);
	
	// Color blend
	const UInt32 numColorKeys = m_NumColorKeys;
	const UInt32 timeColor = std::min(std::max((UInt32)m_ColorTime[0], time), (UInt32)m_ColorTime[numColorKeys-1]);
	for (UInt32 i = 1; i < numColorKeys; i++)
	{
		const UInt32 currTime = m_ColorTime[i];
		if(timeColor <= currTime)
		{
			const UInt32 prevTime = m_ColorTime[i-1];
			const UInt32 frac = InverseLerpWord(prevTime, currTime, timeColor) >> 8; // frac is byte
			color = Lerp (m_Keys[i-1], m_Keys[i], frac);
			break;
		}
	}
	
	// Alpha blend
	const UInt32 numAlphaKeys = m_NumAlphaKeys;
	const UInt32 timeAlpha = std::min(std::max((UInt32)m_AlphaTime[0], time), (UInt32)m_AlphaTime[numAlphaKeys-1]);
	for (UInt32 i = 1; i < numAlphaKeys; i++)
	{
		const UInt32 currTime = m_AlphaTime[i];
		if(timeAlpha <= currTime)
		{
			const UInt32 prevTime = m_AlphaTime[i-1];
			const UInt32 frac = InverseLerpWord(prevTime, currTime, timeAlpha) >> 8; // frac is byte
			color.a = LerpByte(m_Keys[i-1].a, m_Keys[i].a, frac);
			break;
		}
	}
	
	return color;
}

/// Simple class to interpolate between colors.
template<int size>
class GradientDeprecated
 {
	public:
		/// Get a color
		ColorRGBA32 &operator[] (int i) { AssertIf (i < 0 || i >= size); return m_Colors[i]; }
		/// Get a color
		const ColorRGBA32 &operator[] (int i) const { AssertIf (i < 0 || i >= size);  return m_Colors[i]; }

		/// Get the color value at a given position
		/// @param position a position in unnormalized 16.16 bit fixed
		ColorRGBA32 GetFixed (UInt32 position) const {
			return Lerp (m_Colors[position >> 16], m_Colors[(position >> 16) + 1], (position >> 8) & 255);
		}
		
	private:
		/// The array of colors this interpolator works through
		ColorRGBA32 m_Colors[size];
};

#endif
