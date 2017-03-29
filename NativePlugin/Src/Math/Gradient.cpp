#include "PluginPrefix.h"
#include "Gradient.h"

GradientNEW::GradientNEW()
:	m_NumColorKeys(2)
,	m_NumAlphaKeys(2)
{
	m_Keys[0] = m_Keys[1] = ColorRGBA32(0xffffffff);
	m_ColorTime[0] = m_AlphaTime[0] = NormalizedToWord(0.0f);
	m_ColorTime[1] = m_AlphaTime[1] = NormalizedToWord(1.0f);

	for(UInt32 i = 2; i < kGradientMaxNumKeys; i++)
	{
		m_Keys[i] = ColorRGBA32(0);
		m_ColorTime[i] = NormalizedToWord(0.0f);
		m_AlphaTime[i] = NormalizedToWord(0.0f);
	}
}

GradientNEW::~GradientNEW()
{
}

void GradientNEW::SetKeys (ColorKey* colorKeys, unsigned numColorKeys, AlphaKey* alphaKeys, unsigned numAlphaKeys)
{
	SetColorKeys (colorKeys, numColorKeys);
	SetAlphaKeys (alphaKeys, numAlphaKeys);
}

void GradientNEW::SwapColorKeys(int i, int j)
{
	ColorRGBA32 tmpCol = m_Keys[i];
	UInt16 tmpTime = m_ColorTime[i];
	m_Keys[i].r = m_Keys[j].r;
	m_Keys[i].g = m_Keys[j].g;
	m_Keys[i].b = m_Keys[j].b;
	m_ColorTime[i] = m_ColorTime[j];
	m_Keys[j].r = tmpCol.r;
	m_Keys[j].g = tmpCol.g;
	m_Keys[j].b = tmpCol.b;
	m_ColorTime[j] = tmpTime;
}

void GradientNEW::SwapAlphaKeys(int i, int j)
{
	ColorRGBA32 tmpCol = m_Keys[i];
	UInt16 tmpTime = m_AlphaTime[i];
	m_Keys[i].a = m_Keys[j].a;
	m_AlphaTime[i] = m_AlphaTime[j];
	m_Keys[j].a = tmpCol.a;
	m_AlphaTime[j] = tmpTime;
}

void GradientNEW::SetColorKeys (ColorKey* colorKeys, unsigned numKeys)
{
	if (numKeys > kGradientMaxNumKeys)
		numKeys = kGradientMaxNumKeys;

	for (unsigned i=0; i<numKeys; ++i)
	{
		const ColorRGBAf& color = colorKeys[i].m_Color;
		m_Keys[i].r = NormalizedToByte(color.r);
		m_Keys[i].g = NormalizedToByte(color.g);
		m_Keys[i].b = NormalizedToByte(color.b);
		m_ColorTime[i] = NormalizedToWord(colorKeys[i].m_Time);
	}
	m_NumColorKeys = numKeys;

	// Ensure sorted!
	int i = 0;
	const int keyCount = m_NumColorKeys;
	while ((i + 1) < keyCount)
	{
		if (m_ColorTime[i] > m_ColorTime[i+1])
		{
			SwapColorKeys(i, i + 1);
			if (i > 0)
				i -= 2;
		}
		i++;
	}

	ValidateColorKeys();
}

void GradientNEW::SetAlphaKeys (AlphaKey* alphaKeys, unsigned numKeys)
{
	if (numKeys > kGradientMaxNumKeys)
		numKeys = kGradientMaxNumKeys;

	for (unsigned i=0; i<numKeys; ++i)
	{
		float alpha = alphaKeys[i].m_Alpha;
		m_Keys[i].a = NormalizedToByte(alpha);
		m_AlphaTime[i] = NormalizedToWord(alphaKeys[i].m_Time);
	}
	m_NumAlphaKeys = numKeys;

	// Ensure sorted!
	int i = 0;
	const int keyCount = m_NumAlphaKeys;
	while ((i + 1) < keyCount)
	{
		if (m_AlphaTime[i] > m_AlphaTime[i+1])
		{
			SwapAlphaKeys(i, i + 1);
			if (i > 0)
				i -= 2;
		}
		i++;
	}
	
	ValidateAlphaKeys();
}

ColorRGBA32 GradientNEW::GetConstantColor () const
{
	return m_Keys[0];
}

void GradientNEW::SetConstantColor (ColorRGBA32 color)
{
	m_Keys[0] = color;
	m_NumAlphaKeys = 1;
	m_NumColorKeys = 1;
}

void GradientNEW::ValidateColorKeys()
{
	// Make sure there is a minimum of 2 keys
	if(m_NumColorKeys < 2)
	{
		m_NumColorKeys = 2;
		for(int rgb = 0; rgb < 3; rgb++)
			m_Keys[1][rgb] = m_Keys[0][rgb];
		m_ColorTime[0] = NormalizedToWord(0.0f);
		m_ColorTime[1] = NormalizedToWord(1.0f);
	}
}

void GradientNEW::ValidateAlphaKeys()
{
	// Make sure there is a minimum of 2 keys
	if(m_NumAlphaKeys < 2)
	{
		m_NumAlphaKeys = 2;
		m_Keys[1].a = m_Keys[0].a;
		m_AlphaTime[0] = NormalizedToWord(0.0f);
		m_AlphaTime[1] = NormalizedToWord(1.0f);
	}
}

void GradientNEW::InitializeOptimized(OptimizedGradient& gradient)
{
	// Copy all time values
	for(int i = 0; i < m_NumColorKeys; ++i)
		gradient.times[i] = m_ColorTime[i];

	for(int i = 0, i2 = m_NumColorKeys; i < m_NumAlphaKeys; ++i, ++i2)
		gradient.times[i2] = m_AlphaTime[i];

	// Remove duplicates
	int keyCount = m_NumColorKeys + m_NumAlphaKeys;
	for(int i = 0; i < keyCount-1; ++i)
	{
		for(int j = i+1; j < keyCount; )
		{
			if(gradient.times[i] == gradient.times[j])
			{
				std::swap(gradient.times[j], gradient.times[keyCount-1]);
				keyCount--;
				continue;	
			}
			++j;
		}
	}

	// Sort
	int i = 0;
	while ((i + 1) < keyCount)
	{
		if (gradient.times[i] > gradient.times[i+1])
		{
			std::swap(gradient.times[i], gradient.times[i+1]);
			if (i > 0)
				i -= 2;
		}
		i++;
	}

	for(int i = 0; i < keyCount; ++i)
		gradient.colors[i] = Evaluate(WordToNormalized(gradient.times[i]));
	gradient.keyCount = keyCount;

	for(int i = 1; i < keyCount; ++i)
		gradient.rcp[i] = ((((1<<24)) / std::max<UInt32>(gradient.times[i] - gradient.times[i-1], 1)))+1;
}