#ifndef SHURIKENCURVES_H
#define SHURIKENCURVES_H

#include "ParticleSystemParticles.h"
#include "PolynomialCurve.h"
#include "Math/AnimationCurve.h"


struct MinMaxCurve;

// Some profile numbers from a run with 250,000 particles evaluating 3 velocity properties each on Intel i7-2600 CPU @ 3.4 GHz
// Scalar:									4.6  ms
// Optimized curve:							7.2  ms
// Random between 2 scalars:				9.5  ms
// Random between 2 curves:					9.5  ms
// Non-optimized curve:						10.0 ms
// Random between 2 non-optimized curves:	12.0 ms

enum ParticleSystemCurveEvalMode
{
	kEMScalar,
	kEMOptimized,
	kEMOptimizedMinMax,
	kEMSlow,
};

enum MinMaxCurveState
{
	kMMCScalar = 0, 
	kMMCCurve = 1, 
	kMMCTwoCurves = 2, 
	kMMCTwoConstants = 3
};

struct MinMaxOptimizedPolyCurves
{
	void Integrate();
	void DoubleIntegrate();
	Vector2f FindMinMaxIntegrated();
	Vector2f FindMinMaxDoubleIntegrated();

	OptimizedPolynomialCurve max;
	OptimizedPolynomialCurve min;	
};

inline float EvaluateIntegrated (const MinMaxOptimizedPolyCurves& curves, float t, float factor)
{
	const float v0 = curves.min.EvaluateIntegrated (t);
	const float v1 = curves.max.EvaluateIntegrated (t);
	return Lerp (v0, v1, factor);
}

inline float EvaluateDoubleIntegrated (const MinMaxOptimizedPolyCurves& curves, float t, float factor)
{
	const float v0 = curves.min.EvaluateDoubleIntegrated (t);
	const float v1 = curves.max.EvaluateDoubleIntegrated (t);
	return Lerp (v0, v1, factor);
}

struct MinMaxPolyCurves
{
	void Integrate();
	void DoubleIntegrate();
	Vector2f FindMinMaxIntegrated();
	Vector2f FindMinMaxDoubleIntegrated();

	PolynomialCurve max;
	PolynomialCurve min;
};

inline float EvaluateIntegrated (const MinMaxPolyCurves& curves, float t, float factor)
{
	const float v0 = curves.min.EvaluateIntegrated (t);
	const float v1 = curves.max.EvaluateIntegrated (t);
	return Lerp (v0, v1, factor);
}

inline float EvaluateDoubleIntegrated (const MinMaxPolyCurves& curves, float t, float factor)
{
	const float v0 = curves.min.EvaluateDoubleIntegrated (t);
	const float v1 = curves.max.EvaluateDoubleIntegrated (t);
	return Lerp (v0, v1, factor);
}

struct MinMaxAnimationCurves
{
	bool SupportsProcedural ();

	AnimationCurve max;
	AnimationCurve min;
};

bool BuildCurves (MinMaxOptimizedPolyCurves& polyCurves, const MinMaxAnimationCurves& editorCurves, float scalar, short minMaxState);
void BuildCurves (MinMaxPolyCurves& polyCurves, const MinMaxAnimationCurves& editorCurves, float scalar, short minMaxState);
bool CurvesSupportProcedural (const MinMaxAnimationCurves& editorCurves, short minMaxState);

struct MinMaxCurve
{
	MinMaxOptimizedPolyCurves polyCurves;
private: 
	float scalar;		// Since scalar is baked into the optimized curve we use the setter function to modify it.

public:
	short minMaxState;	// see enum MinMaxCurveState
	bool isOptimizedCurve;
	
	MinMaxAnimationCurves editorCurves;
	
	MinMaxCurve ();

	inline float GetScalar() const { return scalar; }
	inline void SetScalar(float value) { scalar = value; BuildCurves(polyCurves, editorCurves, scalar, minMaxState); }
	
	bool IsOptimized () const { return isOptimizedCurve; }
	bool UsesMinMax () const { return (minMaxState == kMMCTwoCurves) || (minMaxState == kMMCTwoConstants); }
	
	Vector2f FindMinMax() const;
	Vector2f FindMinMaxIntegrated() const;
	Vector2f FindMinMaxDoubleIntegrated() const;
};

inline float EvaluateSlow (const MinMaxCurve& curve, float t, float factor)
{
	const float v = curve.editorCurves.max.Evaluate(t) * curve.GetScalar ();
	if (curve.minMaxState == kMMCTwoCurves)
		return Lerp (curve.editorCurves.min.Evaluate(t) * curve.GetScalar (), v, factor);
	else
		return v;
}

template<ParticleSystemCurveEvalMode mode>
inline float Evaluate (const MinMaxCurve& curve, float t, float factor = 1.0F)
{
	if(mode == kEMScalar)
	{
		return curve.GetScalar();
	}
	if(mode == kEMOptimized)
	{
		return curve.polyCurves.max.Evaluate (t);
	}
	else if (mode == kEMOptimizedMinMax)
	{
		const float v0 = curve.polyCurves.min.Evaluate (t);
		const float v1 = curve.polyCurves.max.Evaluate (t);
		return Lerp (v0, v1, factor);
	}
	else if (mode == kEMSlow)
	{
		return EvaluateSlow (curve, t, factor);
	}
	return 0;
}

inline float Evaluate (const MinMaxCurve& curve, float t, float randomValue = 1.0F)
{
	if (curve.minMaxState == kMMCScalar)
		return curve.GetScalar ();

	if (curve.minMaxState == kMMCTwoConstants)
		return Lerp ( curve.editorCurves.min.GetKey(0).value * curve.GetScalar (), 
					  curve.editorCurves.max.GetKey(0).value * curve.GetScalar (), randomValue);

	if (curve.isOptimizedCurve)
		return Evaluate<kEMOptimizedMinMax> (curve, t, randomValue);
	else
		return Evaluate<kEMSlow> (curve, t, randomValue);
	return 0;
}

struct DualMinMax3DPolyCurves
{
	MinMaxOptimizedPolyCurves optX;
	MinMaxOptimizedPolyCurves optY;
	MinMaxOptimizedPolyCurves optZ;
	MinMaxPolyCurves x;
	MinMaxPolyCurves y;
	MinMaxPolyCurves z;
};

#endif // SHURIKENCURVES_H
