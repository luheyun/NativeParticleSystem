#include "PluginPrefix.h"
#include "PolynomialCurve.h"
#include "Math/Vector2.h"
#include "Math/Polynomials.h"
#include "Math/AnimationCurve.h"

static void DoubleIntegrateSegment (float* coeff)
{
	coeff[0] /= 20.0F;
	coeff[1] /= 12.0F;
	coeff[2] /= 6.0F;
	coeff[3] /= 2.0F;
}

static void IntegrateSegment (float* coeff)
{
	coeff[0] /= 4.0F;
	coeff[1] /= 3.0F;
	coeff[2] /= 2.0F;
	coeff[3] /= 1.0F;
}

void CalculateMinMax(Vector2f& minmax, float value)
{
	minmax.x = std::min(minmax.x, value);
	minmax.y = std::max(minmax.y, value);
}

void ConstrainToPolynomialCurve (AnimationCurve& curve)
{
	const int max = OptimizedPolynomialCurve::kMaxPolynomialKeyframeCount;
	
	// Maximum 3 keys
	if (curve.GetKeyCount () > max)
		curve.RemoveKeys(curve.begin() + max, curve.end());

	// Clamp begin and end to 0...1 range
	if (curve.GetKeyCount () >= 2)
	{
		curve.GetKey(0).time = 0;
		curve.GetKey(curve.GetKeyCount ()-1).time = 1;
	}
}

bool IsValidPolynomialCurve (const AnimationCurve& curve)
{
	// Maximum 3 keys
	if (curve.GetKeyCount () > OptimizedPolynomialCurve::kMaxPolynomialKeyframeCount)
		return false;
	// One constant key can always be representated
	else if (curve.GetKeyCount () <= 1)
		return true;
	// First and last keyframe must be at 0 and 1 time
	else
	{
		float beginTime = curve.GetKey(0).time;
		float endTime = curve.GetKey(curve.GetKeyCount ()-1).time;
		
		return CompareApproximately(beginTime, 0.0F, 0.0001F) && CompareApproximately(endTime, 1.0F, 0.0001F);
	}
}

void SetPolynomialCurveToValue (AnimationCurve& a, OptimizedPolynomialCurve& c, float value)
{
	AnimationCurve::Keyframe keys[2] = { AnimationCurve::Keyframe(0.0f, value), AnimationCurve::Keyframe(1.0f, value) };
	a.Assign(keys, keys + 2);
	c.BuildOptimizedCurve(a, 1.0f);
}

void SetPolynomialCurveToLinear (AnimationCurve& a, OptimizedPolynomialCurve& c)
{
	AnimationCurve::Keyframe keys[2] = { AnimationCurve::Keyframe(0.0f, 0.0f), AnimationCurve::Keyframe(1.0f, 1.0f) };
	keys[0].inSlope = 0.0f; keys[0].outSlope = 1.0f;
	keys[1].inSlope = 1.0f; keys[1].outSlope = 0.0f;
	a.Assign(keys, keys + 2);
	c.BuildOptimizedCurve(a, 1.0f);
}

bool OptimizedPolynomialCurve::BuildOptimizedCurve (const AnimationCurve& editorCurve, float scale)
{
	if (!IsValidPolynomialCurve(editorCurve))
		return false;
	
	const size_t keyCount = editorCurve.GetKeyCount ();
	
	timeValue = 1.0F;
	memset(segments, 0, sizeof(segments));
	
	// Handle corner case 1 & 0 keyframes
	if (keyCount == 0)
		;
	else if (keyCount == 1)
	{	
		// Set constant value coefficient
		for (int i=0;i<kSegmentCount;i++)
			segments[i].coeff[3] = editorCurve.GetKey(0).value * scale;
	}
	else
	{
		float segmentStartTime[kSegmentCount];
		
		for (size_t i=0;i<kSegmentCount;i++)
		{
			bool hasSegment = i+1 < keyCount;
			if (hasSegment)
			{
				AnimationCurve::Cache cache;
				editorCurve.CalculateCacheData(cache, i, i + 1, 0.0F);
				
				memcpy(segments[i].coeff, cache.coeff, sizeof(Polynomial));
				segmentStartTime[i] = editorCurve.GetKey(i).time;
			}
			else
			{
				memcpy(segments[i].coeff, segments[i-1].coeff, sizeof(Polynomial));
				segmentStartTime[i] = 1.0F;//timeValue[i-1];
			}
		}

		// scale curve
		for (int i=0;i<kSegmentCount;i++)
		{
			segments[i].coeff[0] *= scale;
			segments[i].coeff[1] *= scale;
			segments[i].coeff[2] *= scale;
			segments[i].coeff[3] *= scale;
		}

		// Timevalue 0 is always 0.0F. No need to store it.
		timeValue = segmentStartTime[1];
	}

	return true;
}

void OptimizedPolynomialCurve::Integrate ()
{
	for (int i=0;i<kSegmentCount;i++)
		IntegrateSegment(segments[i].coeff);
}

void OptimizedPolynomialCurve::DoubleIntegrate ()
{
	Polynomial velocity0 = segments[0];
	IntegrateSegment (velocity0.coeff);

	velocityValue = Polynomial::EvalSegment(timeValue, velocity0.coeff) * timeValue;
	
	for (int i=0;i<kSegmentCount;i++)
		DoubleIntegrateSegment(segments[i].coeff);
}

Vector2f OptimizedPolynomialCurve::FindMinMaxDoubleIntegrated() const
{
	// Because of velocityValue * t, this becomes a quartic polynomial (4th order polynomial).
	// TODO: Find all roots of quartic polynomial
	Vector2f result = Vector2f::zero;
	const int numSteps = 20;
	const float delta = 1.0f / float(numSteps);
	float acc = delta;
	for(int i = 0; i < numSteps; i++)
	{
		CalculateMinMax(result, EvaluateDoubleIntegrated(acc));
		acc += delta;
	}
	return result;
}

// Find the maximum of the integrated curve (x: min, y: max)
Vector2f OptimizedPolynomialCurve::FindMinMaxIntegrated() const
{
	Vector2f result = Vector2f::zero;

	float start[kSegmentCount] = {0.0f, timeValue};
	float end[kSegmentCount] = {timeValue, 1.0f};
	for(int i = 0; i < kSegmentCount; i++)
	{
		// Differentiate coefficients
		float a = 4.0f*segments[i].coeff[0];
		float b = 3.0f*segments[i].coeff[1];
		float c = 2.0f*segments[i].coeff[2];
		float d = 1.0f*segments[i].coeff[3];

		float roots[3];
		int numRoots = CubicPolynomialRootsGeneric(roots, a, b, c, d);
		for(int r = 0; r < numRoots; r++)
		{
			float root = roots[r] + start[i];
			if((root >= start[i]) && (root < end[i]))
				CalculateMinMax(result, EvaluateIntegrated(root));
		}

		// TODO: Don't use eval integrated, use eval segment (and integrate in loop)
		CalculateMinMax(result, EvaluateIntegrated(end[i]));
	}
	return result;
}

// Find the maximum of a double integrated curve (x: min, y: max)
Vector2f PolynomialCurve::FindMinMaxDoubleIntegrated() const
{
	// Because of velocityValue * t, this becomes a quartic polynomial (4th order polynomial).
	// TODO: Find all roots of quartic polynomial
	Vector2f result = Vector2f::zero;
	const int numSteps = 20;
	const float delta = 1.0f / float(numSteps);
	float acc = delta;
	for(int i = 0; i < numSteps; i++)
	{
		CalculateMinMax(result, EvaluateDoubleIntegrated(acc));
		acc += delta;
	}
	return result;
}

Vector2f PolynomialCurve::FindMinMaxIntegrated() const
{
	Vector2f result = Vector2f::zero;
	
	float prevTimeValue = 0.0f;
	for(int i = 0; i < segmentCount; i++)
	{
		// Differentiate coefficients
		float a = 4.0f*segments[i].coeff[0];
		float b = 3.0f*segments[i].coeff[1];
		float c = 2.0f*segments[i].coeff[2];
		float d = 1.0f*segments[i].coeff[3];

		float roots[3];
		int numRoots = CubicPolynomialRootsGeneric(roots, a, b, c, d);
		for(int r = 0; r < numRoots; r++)
		{
			float root = roots[r] + prevTimeValue;
			if((root >= prevTimeValue) && (root < times[i]))
				CalculateMinMax(result, EvaluateIntegrated(root));
		}

		// TODO: Don't use eval integrated, use eval segment (and integrate in loop)
		CalculateMinMax(result, EvaluateIntegrated(times[i]));
		prevTimeValue = times[i];
	}
	return result;
}

bool PolynomialCurve::IsValidCurve(const AnimationCurve& editorCurve)
{
	int keyCount = editorCurve.GetKeyCount();
	int segmentCount = keyCount - 1;
	if(editorCurve.GetKey(0).time != 0.0f)
		segmentCount++;
	if(editorCurve.GetKey(keyCount-1).time != 1.0f)
		segmentCount++;
	return segmentCount <= kMaxNumSegments;
}

bool PolynomialCurve::BuildCurve(const AnimationCurve& editorCurve, float scale)
{
	int keyCount = editorCurve.GetKeyCount();
	segmentCount = 1;

	const float kMaxTime = 1.01f;

	memset(segments, 0, sizeof(segments));
	memset(integrationCache, 0, sizeof(integrationCache));
	memset(doubleIntegrationCache, 0, sizeof(doubleIntegrationCache));
	memset(times, 0, sizeof(times));
	times[0] = kMaxTime;
	
	// Handle corner case 1 & 0 keyframes
	if (keyCount == 0)
		;
	else if (keyCount == 1)
	{	
		// Set constant value coefficient
		segments[0].coeff[3] = editorCurve.GetKey(0).value * scale;
	}
	else
	{
		segmentCount = keyCount - 1;	
		int segmentOffset = 0;

		// Add extra key to start if it doesn't match up
		if(editorCurve.GetKey(0).time != 0.0f)
		{
			segments[0].coeff[3] = editorCurve.GetKey(0).value;
			times[0] = editorCurve.GetKey(0).time;
			segmentOffset = 1;
		}

		for (int i = 0;i<segmentCount;i++)
		{
			AnimationCurve::Cache cache;
			editorCurve.CalculateCacheData(cache, i, i + 1, 0.0F);
			memcpy(segments[i+segmentOffset].coeff, cache.coeff, 4 * sizeof(float));
			times[i+segmentOffset] = editorCurve.GetKey(i+1).time;
		}
		segmentCount += segmentOffset;

		// Add extra key to start if it doesn't match up
		if(editorCurve.GetKey(keyCount-1).time != 1.0f)
		{
			segments[segmentCount].coeff[3] = editorCurve.GetKey(keyCount-1).value;
			segmentCount++;
		}
			
		// Fixup last key time value
		times[segmentCount-1] = kMaxTime;

		for (int i = 0;i<segmentCount;i++)
		{
			segments[i].coeff[0] *= scale;
			segments[i].coeff[1] *= scale;
			segments[i].coeff[2] *= scale;
			segments[i].coeff[3] *= scale;
		}
	}
	
	return true;
}

void GenerateIntegrationCache(PolynomialCurve& curve)
{
	curve.integrationCache[0] = 0.0f;
	float prevTimeValue0 = curve.times[0];
	float prevTimeValue1 = 0.0f;	
	for (int i=1;i<curve.segmentCount;i++)
	{
		float coeff[4];
		memcpy(coeff, curve.segments[i-1].coeff, 4*sizeof(float));
		IntegrateSegment (coeff);
		float time = prevTimeValue0 - prevTimeValue1;
		curve.integrationCache[i] = curve.integrationCache[i-1] + Polynomial::EvalSegment(time, coeff) * time;
		prevTimeValue1 = prevTimeValue0;
		prevTimeValue0 = curve.times[i];
	}
}

// Expects double integrated segments and valid integration cache
void GenerateDoubleIntegrationCache(PolynomialCurve& curve)
{
	float sum = 0.0f;
	float prevTimeValue = 0.0f;
	for(int i = 0; i < curve.segmentCount; i++)
	{
		curve.doubleIntegrationCache[i] = sum;
		float time = curve.times[i] - prevTimeValue;
		time = std::max(time, 0.0f);
		sum += Polynomial::EvalSegment(time, curve.segments[i].coeff) * time * time + curve.integrationCache[i] * time;
		prevTimeValue = curve.times[i];
	}
}

void PolynomialCurve::Integrate ()
{
	GenerateIntegrationCache(*this);
	for (int i=0;i<segmentCount;i++)
		IntegrateSegment(segments[i].coeff);
}

void PolynomialCurve::DoubleIntegrate ()
{
	GenerateIntegrationCache(*this);
	for (int i=0;i<segmentCount;i++)
		DoubleIntegrateSegment(segments[i].coeff);
	GenerateDoubleIntegrationCache(*this);
}
