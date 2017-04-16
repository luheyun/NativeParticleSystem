#pragma once

#include "Utilities/dynamic_array.h"
#include "Math/FloatConversion.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

enum { kDefaultWrapMode = 0, kClamp = 1 << 0, kRepeat = 1 << 1, kPingPong = 1 << 2, kClampForever = 1 << 3 };
#define kCurveTimeEpsilon 0.00001F

/*
	AnimationCurves in Maya are represented as time/value keys with 2D tangents which are always of normalized length.
	From the tangents a slope is calculated (tangent.y / tangent.x) -> (thus length of the tangent doesn't matter)
	
	When the slope is multiplied by the time range of the curve (rhs.time - lhs.time) it can be evaluated using
	a standard hermite interpolator.
	
	In the Unity AnimationCurve the slopes are directly stored in the keyframe instead of the 2D tangent vectors.
*/

///@TODO: Curve templates suck.
///       Lets make some implementation where they share the same data structure and only Evaluate is specialized.

template<class T>
struct KeyframeTpl
{
	// DECLARE_SERIALIZE_OPTIMIZE_TRANSFER (Keyframe)
	inline static const char* GetTypeString ()	{ return "Keyframe"; }
	inline static bool IsAnimationChannel ()	{ return false; }
	inline static bool MightContainPPtr ()	{ return false; }
	// Disable transfer optimization in Editor because tangentMode optimized serialization when reading AssetBundles will corrupt data
	inline static bool AllowTransferOptimization ()
	{
		return true;
	} 
	template<class TransferFunction>
	void Transfer (TransferFunction& transfer);

	float time;
	T value;
	T inSlope;
	T outSlope; 
	
	KeyframeTpl () {}
	KeyframeTpl (float t, const T& v);
	
			
	friend bool operator < (const KeyframeTpl& lhs, const KeyframeTpl& rhs) { return lhs.time < rhs.time; }
};

/*
	Use simple, non-generic struct to pass value to managed code.
	Templated struct can cause issues, because on a native side a simple one is used.
	I.e. on ARM64 the different registers might be used to pass value.
 */
struct KeyframeTplFloat
{
	float time;
	float value;
	float inSlope;
	float outSlope;

	KeyframeTplFloat()
	{
	}

	KeyframeTplFloat(const KeyframeTpl<float>& frame)
	{
		time = frame.time;
		value = frame.value;
		inSlope = frame.inSlope;
		outSlope = frame.outSlope;
	}

	operator KeyframeTpl<float>() const
	{
		KeyframeTpl<float> frame;
		frame.time = time;
		frame.value = value;
		frame.inSlope = inSlope;
		frame.outSlope = outSlope;
		return frame;
	}
};


enum AnimationCurveType {
	kFloatCurve = 0,
	kVector3Curve = 1,
	kQuaternionCurve = 2
};

template<class T>
class EXPORT_COREMODULE AnimationCurveTpl
{
	public: 
	
	/// Stores the curve as a pure cubic function with 4 coefficients
	struct Cache
	{
		int index;
		float time;
		float timeEnd;
		T coeff[4];

		Cache () { time = std::numeric_limits<float>::infinity (); index=0; timeEnd = 0.0f; memset(&coeff, 0, sizeof(coeff)); }
		void Invalidate () { time = std::numeric_limits<float>::infinity (); index=0; }
	};
	
	typedef KeyframeTpl<T> Keyframe;
	
	typedef dynamic_array<Keyframe> KeyframeContainer;
	typedef typename KeyframeContainer::iterator iterator;
	typedef typename KeyframeContainer::const_iterator const_iterator;
	
public:
	AnimationCurveTpl ()
	{
		m_PreInfinity = m_PostInfinity = kInternalClamp;
	}

	/// Evaluates the AnimationCurve caching the segment.
	T Evaluate (float curveT) const;
	T EvaluateClamp (float curveT) const;
	
	bool IsValid () const { return m_Curve.size () >= 2; }
	
	int AddKey (const Keyframe& key);

	/// Performs no error checking. And doesn't invalidate the cache!
	void AddKeyBackFast (const Keyframe& key) { m_Curve.push_back (key); }
	
	const Keyframe& GetKey (int index) const { return m_Curve[index]; }

	/// When changing the keyframe using GetKey you are not allowed to change the time!
	/// After modifying a key you have to call InvalidateCache
	Keyframe& GetKey (int index) { return const_cast<Keyframe&> (m_Curve[index]); }
	
	iterator begin () { return m_Curve.begin (); }
	iterator end () { return m_Curve.end (); }
	const_iterator begin () const { return m_Curve.begin (); }
	const_iterator end () const { return m_Curve.end (); }

	void InvalidateCache ();
	
	int GetKeyCount () const { return m_Curve.size (); }
		
	void RemoveKeys (iterator begin, iterator end);

	/// Returns the first and last keyframe time
	std::pair<float, float> GetRange () const;
	
	enum { kInternalPingPong = 0, kInternalRepeat = 1, kInternalClamp = 2 };

	// How does the curve before the first keyframe
	void SetPreInfinity (int pre);
	int GetPreInfinity () const;
	// How does the curve behave after the last keyframe
	void SetPostInfinity (int post);
	int GetPostInfinity () const;

	// How does the curve before the first keyframe
	void SetPreInfinityInternal (int pre) { m_PreInfinity = pre; InvalidateCache (); }
	int GetPreInfinityInternal () const { return m_PreInfinity; }
	// How does the curve behave after the last keyframe
	void SetPostInfinityInternal (int post) { m_PostInfinity = post; InvalidateCache (); }
	int GetPostInfinityInternal () const { return m_PostInfinity; }
	
	void Assign (const Keyframe* begin, const Keyframe* end) { m_Curve.assign (begin, end); InvalidateCache(); }
	void Swap (KeyframeContainer& newArray) { m_Curve.swap(newArray); InvalidateCache(); }
	void Sort () { std::sort(m_Curve.begin(), m_Curve.end()); InvalidateCache(); }

	void ResizeUninitialized (int size) { m_Curve.resize_uninitialized(size); }

	///@TODO: Cleanup old code to completely get rid of this
	int FindIndex (const Cache& cache, float curveT) const;
	
	///@TODO: Cleanup old code to completely get rid of this
	/// Returns the closest keyframe index that is less than time.
	/// Returns -1 if time is outside the range of the curve
	int FindIndex (float time) const;

	void CalculateCacheData (Cache& cache, int lhs, int rhs, float timeOffset) const;
	
	private:
	
	void FindIndexForSampling (const Cache& cache, float curveT, int& lhs, int& rhs) const;
	
	/// Evaluates the AnimationCurve directly.
	void EvaluateWithoutCache (float curveT, T& output)const;

	float WrapTime (float curveT) const;

	mutable Cache m_Cache;
	mutable Cache m_ClampCache;

	KeyframeContainer m_Curve;
	int   m_PreInfinity;
	int   m_PostInfinity;
};

typedef AnimationCurveTpl<float> AnimationCurveBase;
typedef AnimationCurveTpl<float>      AnimationCurve;
typedef AnimationCurveTpl<Quaternionf> AnimationCurveQuat;
typedef AnimationCurveTpl<Vector3f>   AnimationCurveVec3;

inline float FrameToTime (int frame, float sampleRate)
{
	return (float)frame / sampleRate;
}

inline float FloatFrameToTime (float frame, float sampleRate)
{
	return frame / sampleRate;
}


void HandleSteppedCurve (const KeyframeTpl<float>& lhs, const KeyframeTpl<float>& rhs, float& value);
void HandleSteppedTangent (const KeyframeTpl<float>& lhs, const KeyframeTpl<float>& rhs, float& value);

void HandleSteppedCurve (const KeyframeTpl<Vector3f>& lhs, const KeyframeTpl<Vector3f>& rhs, Vector3f& value);
void HandleSteppedTangent (const KeyframeTpl<Vector3f>& lhs, const KeyframeTpl<Vector3f>& rhs, Vector3f& tangent);

void HandleSteppedCurve (const KeyframeTpl<Quaternionf>& lhs, const KeyframeTpl<Quaternionf>& rhs, Quaternionf& tangent);
void HandleSteppedTangent (const KeyframeTpl<Quaternionf>& lhs, const KeyframeTpl<Quaternionf>& rhs, Quaternionf& tangent);

inline float PingPong (float t, float length)
{
	t = Repeat (t, length * 2.0F);
	t = length - Abs (t - length);
	return t;
}


inline float Repeat (float t, float begin, float end)
{
	return Repeat (t - begin, end - begin) + begin;
}

inline double RepeatD (double t, double begin, double end)
{
	return RepeatD (t - begin, end - begin) + begin;
}

inline float PingPong (float t, float begin, float end)
{
	return PingPong (t - begin, end - begin) + begin;
}

#if (defined(__GNUC__) && (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)) || defined(__clang__)
	// in GCC 4.3 and above the explicit template specialization cannot have a storage class
	#define SPEC_STORAGE_CLASS inline
#else
	#define SPEC_STORAGE_CLASS static
#endif

#define kMaxTan 5729577.9485111479F

template<class T>
static T MaxTan () { return kMaxTan; }

template<>
SPEC_STORAGE_CLASS Quaternionf MaxTan<Quaternionf> () { return Quaternionf(kMaxTan, kMaxTan, kMaxTan, kMaxTan); }

template<>
SPEC_STORAGE_CLASS Vector3f MaxTan<Vector3f> () { return Vector3f(kMaxTan, kMaxTan, kMaxTan); }

#undef kMaxTan

template<class T>
static T Zero () { return T (); }

template<>
SPEC_STORAGE_CLASS Quaternionf Zero<Quaternionf> () { return Quaternionf(0.0F, 0.0F, 0.0F, 0.0F); }

template<>
SPEC_STORAGE_CLASS Vector3f Zero<Vector3f> () { return Vector3f(0.0F, 0.0F, 0.0F); }

#undef SPEC_STORAGE_CLASS

void ScaleCurveValue (AnimationCurve& curve, float scale);
void OffsetCurveValue (AnimationCurve& curve, float offset);
void ScaleCurveTime (AnimationCurve& curve, float scale);
void OffsetCurveTime (AnimationCurve& curve, float offset);

template<class T>
inline T HermiteInterpolate (float t, T p0, T m0, T m1, T p1)
{
	float t2 = t * t;
	float t3 = t2 * t;

	float a = 2.0F * t3 - 3.0F * t2 + 1.0F;
	float b = t3 - 2.0F * t2 + t;
	float c = t3 - t2;
	float d = -2.0F * t3 +  3.0F * t2;

	return a * p0 + b * m0 + c * m1 + d * p1;
}

struct KeyframeCompare
{
	template<class T>
	bool operator ()(KeyframeTpl<T> const& k, float t) { return k.time < t; }
	// These are necessary for debug STL (validation of predicates)
	template<class T>
	bool operator ()(KeyframeTpl<T> const& k1, KeyframeTpl<T> const& k2) { return k1.time < k2.time; }
	template<class T>
	bool operator ()(float t, KeyframeTpl<T> const& k) { return !operator() (k, t); }
};
