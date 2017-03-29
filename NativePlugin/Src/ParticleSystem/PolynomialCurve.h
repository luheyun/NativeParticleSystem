#ifndef POLYONOMIAL_CURVE_H
#define POLYONOMIAL_CURVE_H

template<class T>
class AnimationCurveTpl;
typedef AnimationCurveTpl<float> AnimationCurve;
class Vector2f;

struct Polynomial
{
	static float EvalSegment (float t, const float* coeff)
	{
		return (t * (t * (t * coeff[0] + coeff[1]) + coeff[2])) + coeff[3];
	}

	float coeff[4];
};

// Smaller, optimized version
struct OptimizedPolynomialCurve
{
	enum { kMaxPolynomialKeyframeCount = 3, kSegmentCount = kMaxPolynomialKeyframeCount-1, };

	Polynomial segments[kSegmentCount];

	float timeValue;
	float velocityValue;

	// Evaluate double integrated Polynomial curve.
	// Example: position = EvaluateDoubleIntegrated (normalizedTime) * startEnergy^2
	// Use DoubleIntegrate function to for example turn a force curve into a position curve.
	// Expects that t is in the 0...1 range.
	float EvaluateDoubleIntegrated (float t) const
	{
		float res0, res1;

		// All segments are added together. At t = 0, the integrated curve is always zero.

		// 0 segment is sampled up to the 1 keyframe
		// First key is always assumed to be at 0 time
		float t1 = std::min(t, timeValue);

		// 1 segment is sampled from 1 key to 2 key
		// Last key is always assumed to be at 1 time
		float t2 = std::max(0.0F, t - timeValue);

		res0 = Polynomial::EvalSegment(t1, segments[0].coeff) * t1 * t1;
		res1 = Polynomial::EvalSegment(t2, segments[1].coeff) * t2 * t2;

		float finalResult = res0 + res1;

		// Add velocity of previous segments
		finalResult += velocityValue * std::max(t - timeValue, 0.0F);

		return finalResult;
	}

	// Evaluate integrated Polynomial curve.
	// Example: position = EvaluateIntegrated (normalizedTime) * startEnergy
	// Use Integrate function to for example turn a velocity curve into a position curve.
	// Expects that t is in the 0...1 range.
	float EvaluateIntegrated (float t) const
	{
		float res0, res1;

		// All segments are added together. At t = 0, the integrated curve is always zero.

		// 0 segment is sampled up to the 1 keyframe
		// First key is always assumed to be at 0 time
		float t1 = std::min(t, timeValue);

		// 1 segment is sampled from 1 key to 2 key
		// Last key is always assumed to be at 1 time
		float t2 = std::max(0.0F, t - timeValue);

		res0 = Polynomial::EvalSegment(t1, segments[0].coeff) * t1;
		res1 = Polynomial::EvalSegment(t2, segments[1].coeff) * t2;

		return (res0 + res1);
	}

	// Evaluate the curve
	// extects that t is in the 0...1 range
	float Evaluate (float t) const
	{
		float res0 = Polynomial::EvalSegment(t,                segments[0].coeff);
		float res1 = Polynomial::EvalSegment(t - timeValue, segments[1].coeff);

		float result;
		if (t > timeValue)
			result = res1;
		else
			result = res0;

		return result;
	}

	// Find the maximum of a double integrated curve (x: min, y: max)
	Vector2f FindMinMaxDoubleIntegrated() const;

	// Find the maximum of the integrated curve (x: min, y: max)
	Vector2f FindMinMaxIntegrated() const;

	// Precalculates polynomials from the animation curve and a scale factor
	bool BuildOptimizedCurve (const AnimationCurve& editorCurve, float scale);

	// Integrates a velocity curve to be a position curve.
	// You have to call EvaluateIntegrated to evaluate the curve
	void Integrate ();

	// Integrates a velocity curve to be a position curve.
	// You have to call EvaluateDoubleIntegrated to evaluate the curve
	void DoubleIntegrate ();

	// Add a constant force to a velocity curve
	// Assumes that you have already called Integrate on the velocity curve.
	void AddConstantForceToVelocityCurve (float gravity)
	{
		for (int i=0;i<kSegmentCount;i++)
			segments[i].coeff[1] += 0.5F * gravity;
	}
};

// Bigger, not so optimized version
struct PolynomialCurve
{
	enum{ kMaxNumSegments = 8 };

	Polynomial segments[kMaxNumSegments];			// Cached polynomial coefficients
	float integrationCache[kMaxNumSegments];		// Cache of integrated values up until start of segments
	float doubleIntegrationCache[kMaxNumSegments];	// Cache of double integrated values up until start of segments
	float times[kMaxNumSegments];					// Time value for end of segment

	int segmentCount;

	// Find the maximum of a double integrated curve (x: min, y: max)
	Vector2f FindMinMaxDoubleIntegrated() const;

	// Find the maximum of the integrated curve (x: min, y: max)
	Vector2f FindMinMaxIntegrated() const;

	// Precalculates polynomials from the animation curve and a scale factor
	bool BuildCurve(const AnimationCurve& editorCurve, float scale);

	// Integrates a velocity curve to be a position curve.
	// You have to call EvaluateIntegrated to evaluate the curve
	void Integrate ();

	// Integrates a velocity curve to be a position curve.
	// You have to call EvaluateDoubleIntegrated to evaluate the curve
	void DoubleIntegrate ();

	// Evaluates if it is possible to represent animation curve as PolynomialCurve
	static bool IsValidCurve(const AnimationCurve& editorCurve);

	// Evaluate double integrated Polynomial curve.
	// Example: position = EvaluateDoubleIntegrated (normalizedTime) * startEnergy^2
	// Use DoubleIntegrate function to for example turn a force curve into a position curve.
	// Expects that t is in the 0...1 range.
	float EvaluateDoubleIntegrated (float t) const
	{
		float prevTimeValue = 0.0f;
		for(int i = 0; i < segmentCount; i++)
		{
			if(t <= times[i])
			{
				const float time = t - prevTimeValue;
				return doubleIntegrationCache[i] + integrationCache[i] * time + Polynomial::EvalSegment(time, segments[i].coeff) * time * time;
			}
			prevTimeValue = times[i];
		}
		return 1.0f;
	}

	// Evaluate integrated Polynomial curve.
	// Example: position = EvaluateIntegrated (normalizedTime) * startEnergy
	// Use Integrate function to for example turn a velocity curve into a position curve.
	// Expects that t is in the 0...1 range.
	float EvaluateIntegrated (float t) const
	{
		float prevTimeValue = 0.0f;
		for(int i = 0; i < segmentCount; i++)
		{
			if(t <= times[i])
			{
				const float time = t - prevTimeValue;
				return integrationCache[i] + Polynomial::EvalSegment(time, segments[i].coeff) * time;
			}
			prevTimeValue = times[i];
		}
		return 1.0f;
	}

	// Evaluate the curve
	// extects that t is in the 0...1 range
	float Evaluate(float t) const
	{
		float prevTimeValue = 0.0f;
		for(int i = 0; i < segmentCount; i++)
		{
			if(t <= times[i])
				return Polynomial::EvalSegment(t - prevTimeValue, segments[i].coeff);
			prevTimeValue = times[i];
		}
		return 1.0f;
	}
};


void SetPolynomialCurveToValue (AnimationCurve& a, OptimizedPolynomialCurve& c, float value);
void SetPolynomialCurveToLinear (AnimationCurve& a, OptimizedPolynomialCurve& c);
void ConstrainToPolynomialCurve (AnimationCurve& curve);
bool IsValidPolynomialCurve (const AnimationCurve& curve);
void CalculateMinMax(Vector2f& minmax, float value);

#endif // POLYONOMIAL_CURVE_H
