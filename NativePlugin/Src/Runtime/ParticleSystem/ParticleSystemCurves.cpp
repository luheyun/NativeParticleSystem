#include "PluginPrefix.h"
#include "ParticleSystemCurves.h"
#include "Math/AnimationCurve.h"
#include "Math/Vector2.h"
#include "Math/Polynomials.h"

// Calculates the min max range of an animation curve analytically
static void CalculateCurveRangesValue(Vector2f& minMaxValue, const AnimationCurve& curve)
{
	const int keyCount = curve.GetKeyCount();
	if (keyCount == 0)
		return;

	if (keyCount == 1)
	{
		CalculateMinMax(minMaxValue, curve.GetKey(0).value);
	}
	else
	{
		int segmentCount = keyCount - 1;	
		CalculateMinMax(minMaxValue, curve.GetKey(0).value);
		for (int i = 0;i<segmentCount;i++)
		{
			AnimationCurve::Cache cache;
			curve.CalculateCacheData(cache, i, i + 1, 0.0F);

			// Differentiate polynomial
			float a = 3.0f * cache.coeff[0];
			float b = 2.0f * cache.coeff[1];
			float c = 1.0f * cache.coeff[2];

			const float start = curve.GetKey(i).time;
			const float end = curve.GetKey(i+1).time;

			float roots[2];
			int numRoots = QuadraticPolynomialRootsGeneric(a, b, c, roots[0], roots[1]);
			for(int r = 0; r < numRoots; r++)
				if((roots[r] >= 0.0f) && ((roots[r] + start) < end))
					CalculateMinMax(minMaxValue, Polynomial::EvalSegment(roots[r], cache.coeff));
			CalculateMinMax(minMaxValue, Polynomial::EvalSegment(end-start, cache.coeff));
		}
	}
}

bool BuildCurves (MinMaxOptimizedPolyCurves& polyCurves, const MinMaxAnimationCurves& editorCurves, float scalar, short minMaxState)
{
	bool isOptimizedCurve = polyCurves.max.BuildOptimizedCurve(editorCurves.max, scalar);
	if ((minMaxState != kMMCTwoCurves) && (minMaxState != kMMCTwoConstants))
		isOptimizedCurve = isOptimizedCurve && polyCurves.min.BuildOptimizedCurve(editorCurves.max, scalar);
	else
		isOptimizedCurve = isOptimizedCurve && polyCurves.min.BuildOptimizedCurve(editorCurves.min, scalar);
	return isOptimizedCurve;
}

void BuildCurves (MinMaxPolyCurves& polyCurves, const MinMaxAnimationCurves& editorCurves, float scalar, short minMaxState)
{
	polyCurves.max.BuildCurve(editorCurves.max, scalar);
	if ((minMaxState != kMMCTwoCurves) && (minMaxState != kMMCTwoConstants))
		polyCurves.min.BuildCurve(editorCurves.max, scalar);
	else
		polyCurves.min.BuildCurve(editorCurves.min, scalar);	
}

bool CurvesSupportProcedural (const MinMaxAnimationCurves& editorCurves, short minMaxState)
{
	bool isValid = PolynomialCurve::IsValidCurve(editorCurves.max);
	if ((minMaxState != kMMCTwoCurves) && (minMaxState != kMMCTwoConstants))
		return isValid;
	else
		return isValid && PolynomialCurve::IsValidCurve(editorCurves.min);
}


void MinMaxOptimizedPolyCurves::Integrate ()
{
	max.Integrate ();
	min.Integrate ();
}

void MinMaxOptimizedPolyCurves::DoubleIntegrate ()
{
	max.DoubleIntegrate ();
	min.DoubleIntegrate ();
}

Vector2f MinMaxOptimizedPolyCurves::FindMinMaxIntegrated()
{
	Vector2f minRange = min.FindMinMaxIntegrated();
	Vector2f maxRange = max.FindMinMaxIntegrated();
	Vector2f result = Vector2f(std::min(minRange.x, maxRange.x), std::max(minRange.y, maxRange.y));
	return result;
}

Vector2f MinMaxOptimizedPolyCurves::FindMinMaxDoubleIntegrated()
{
	Vector2f minRange = min.FindMinMaxDoubleIntegrated();
	Vector2f maxRange = max.FindMinMaxDoubleIntegrated();
	Vector2f result = Vector2f(std::min(minRange.x, maxRange.x), std::max(minRange.y, maxRange.y));
	return result;
}

void MinMaxPolyCurves::Integrate ()
{
	max.Integrate ();
	min.Integrate ();
}

void MinMaxPolyCurves::DoubleIntegrate ()
{
	max.DoubleIntegrate ();
	min.DoubleIntegrate ();
}

Vector2f MinMaxPolyCurves::FindMinMaxIntegrated()
{
	Vector2f minRange = min.FindMinMaxIntegrated();
	Vector2f maxRange = max.FindMinMaxIntegrated();
	Vector2f result = Vector2f(std::min(minRange.x, maxRange.x), std::max(minRange.y, maxRange.y));
	return result;
}

Vector2f MinMaxPolyCurves::FindMinMaxDoubleIntegrated()
{
	Vector2f minRange = min.FindMinMaxDoubleIntegrated();
	Vector2f maxRange = max.FindMinMaxDoubleIntegrated();
	Vector2f result = Vector2f(std::min(minRange.x, maxRange.x), std::max(minRange.y, maxRange.y));
	return result;
}

MinMaxCurve::MinMaxCurve ()
:	scalar (1.0f)
,	minMaxState (kMMCScalar)
,	isOptimizedCurve(false)
{
	SetPolynomialCurveToValue (editorCurves.max, polyCurves.max, 1.0f);
	SetPolynomialCurveToValue (editorCurves.min, polyCurves.min, 0.0f);
}

Vector2f MinMaxCurve::FindMinMax() const
{
	Vector2f result = Vector2f(std::numeric_limits<float>::infinity (), -std::numeric_limits<float>::infinity ());
	CalculateCurveRangesValue(result, editorCurves.max);
	if((minMaxState == kMMCTwoCurves) || (minMaxState == kMMCTwoConstants))
		CalculateCurveRangesValue(result, editorCurves.min);
	return result * GetScalar();
}

Vector2f MinMaxCurve::FindMinMaxIntegrated() const
{
	if(IsOptimized())
	{
		MinMaxOptimizedPolyCurves integrated = polyCurves;
		integrated.Integrate();
		return integrated.FindMinMaxIntegrated();
	}
	else
	{
		MinMaxPolyCurves integrated;
		BuildCurves(integrated, editorCurves, GetScalar(), minMaxState);
		integrated.Integrate();
		return integrated.FindMinMaxIntegrated();
	}
}

Vector2f MinMaxCurve::FindMinMaxDoubleIntegrated() const
{
	if(IsOptimized())
	{
		MinMaxOptimizedPolyCurves doubleIntegrated = polyCurves;
		doubleIntegrated.DoubleIntegrate();
		return doubleIntegrated.FindMinMaxDoubleIntegrated();
	}
	else
	{
		MinMaxPolyCurves doubleIntegrated;
		BuildCurves(doubleIntegrated, editorCurves, GetScalar(), minMaxState);
		doubleIntegrated.DoubleIntegrate();
		return doubleIntegrated.FindMinMaxDoubleIntegrated();
	}
}
