#include "PluginPrefix.h"
#include "AnimationCurve.h"
using namespace std;

#define kOneThird (1.0F / 3.0F)
#define kMaxTan 5729577.9485111479F

int ToInternalInfinity (int pre);
int FromInternalInfinity (int pre);

int ToInternalInfinity (int pre)
{
	if (pre == kRepeat)
		return AnimationCurve::kInternalRepeat;
	else if (pre == kPingPong)
		return AnimationCurve::kInternalPingPong;
	else
		return AnimationCurve::kInternalClamp;
}

int FromInternalInfinity (int pre)
{
	if (pre == AnimationCurve::kInternalRepeat)
		return kRepeat;
	else if (pre == AnimationCurve::kInternalPingPong)
		return kPingPong;
	else
		return kClampForever;
}

template<class T>
KeyframeTpl<T>::KeyframeTpl (float t, const T& v)
{
	time = t;
	value = v;
	inSlope = Zero<T>();
	outSlope = Zero<T>();

#if UNITY_EDITOR
	tangentMode = 0;
#endif
}


template<class T>
void AnimationCurveTpl<T>::InvalidateCache ()
{
	m_Cache.time = std::numeric_limits<float>::infinity ();
	m_Cache.index = 0;
	m_ClampCache.time = std::numeric_limits<float>::infinity ();
	m_ClampCache.index = 0;
}

template<class T>
pair<float, float> AnimationCurveTpl<T>::GetRange () const
{
	if (!m_Curve.empty ())
		return make_pair (m_Curve[0].time, m_Curve.back ().time);
	else
		return make_pair (std::numeric_limits<float>::infinity (), -std::numeric_limits<float>::infinity ());
}



///@TODO: Handle step curves correctly
template<class T>
void AnimationCurveTpl<T>::EvaluateWithoutCache (float curveT, T& output)const
{
	curveT = WrapTime (curveT);

	int lhsIndex, rhsIndex;
	FindIndexForSampling (m_Cache, curveT, lhsIndex, rhsIndex);
	const Keyframe& lhs = m_Curve[lhsIndex];
	const Keyframe& rhs = m_Curve[rhsIndex];

	float dx = rhs.time - lhs.time;
	T m1;
	T m2;
	float t;
	if (dx != 0.0F)
	{
		t = (curveT - lhs.time) / dx;
		m1 = lhs.outSlope * dx;
		m2 = rhs.inSlope * dx;
	}
	else
	{
		t = 0.0F;
		m1 = Zero<T>();
		m2 = Zero<T>();
	}

	output = HermiteInterpolate (t, lhs.value, m1, m2, rhs.value);
	HandleSteppedCurve(lhs, rhs, output);
}

template<class T>
inline void EvaluateCache (const typename AnimationCurveTpl<T>::Cache& cache, float curveT, T& output)
{
//	DebugAssertIf (curveT < cache.time - kCurveTimeEpsilon || curveT > cache.timeEnd + kCurveTimeEpsilon);
	float t = curveT - cache.time;
	output = (t * (t * (t * cache.coeff[0] + cache.coeff[1]) + cache.coeff[2])) + cache.coeff[3];
}

void SetupStepped (float* coeff, const KeyframeTpl<float>& lhs, const KeyframeTpl<float>& rhs)
{
	// If either of the tangents in the segment are set to stepped, make the constant value equal the value of the left key
	if (lhs.outSlope == std::numeric_limits<float>::infinity() || rhs.inSlope == std::numeric_limits<float>::infinity())
	{
		coeff[0] = 0.0F;
		coeff[1] = 0.0F;
		coeff[2] = 0.0F;
		coeff[3] = lhs.value;
	}
}

void HandleSteppedCurve (const KeyframeTpl<float>& lhs, const KeyframeTpl<float>& rhs, float& value)
{
	if (lhs.outSlope == std::numeric_limits<float>::infinity() || rhs.inSlope == std::numeric_limits<float>::infinity())
		value = lhs.value;
}

void HandleSteppedTangent (const KeyframeTpl<float>& lhs, const KeyframeTpl<float>& rhs, float& tangent)
{
	if (lhs.outSlope == std::numeric_limits<float>::infinity() || rhs.inSlope == std::numeric_limits<float>::infinity())
		tangent = std::numeric_limits<float>::infinity();
}


void SetupStepped (Vector3f* coeff, const KeyframeTpl<Vector3f>& lhs, const KeyframeTpl<Vector3f>& rhs)
{
	for (int i=0;i<3;i++)
	{
		// If either of the tangents in the segment are set to stepped, make the constant value equal the value of the left key
		if (lhs.outSlope[i] == std::numeric_limits<float>::infinity() || rhs.inSlope[i] == std::numeric_limits<float>::infinity())
		{
			coeff[0][i] = 0.0F;
			coeff[1][i] = 0.0F;
			coeff[2][i] = 0.0F;
			coeff[3][i] = lhs.value[i];
		}
	}
}

void HandleSteppedCurve (const KeyframeTpl<Vector3f>& lhs, const KeyframeTpl<Vector3f>& rhs, Vector3f& value)
{
	for (int i=0;i<3;i++)
	{
		if (lhs.outSlope[i] == std::numeric_limits<float>::infinity() || rhs.inSlope[i] == std::numeric_limits<float>::infinity())
			value[i] = lhs.value[i];
	}
}

void HandleSteppedTangent (const KeyframeTpl<Vector3f>& lhs, const KeyframeTpl<Vector3f>& rhs, Vector3f& value)
{
	for (int i=0;i<3;i++)
	{
		if (lhs.outSlope[i] == std::numeric_limits<float>::infinity() || rhs.inSlope[i] == std::numeric_limits<float>::infinity())
			value[i] = std::numeric_limits<float>::infinity();
	}
}

void SetupStepped (Quaternionf* coeff, const KeyframeTpl<Quaternionf>& lhs, const KeyframeTpl<Quaternionf>& rhs)
{
	// If either of the tangents in the segment are set to stepped, make the constant value equal the value of the left key
	if (lhs.outSlope[0] == std::numeric_limits<float>::infinity() || rhs.inSlope[0] == std::numeric_limits<float>::infinity() ||
		lhs.outSlope[1] == std::numeric_limits<float>::infinity() || rhs.inSlope[1] == std::numeric_limits<float>::infinity() ||
		lhs.outSlope[2] == std::numeric_limits<float>::infinity() || rhs.inSlope[2] == std::numeric_limits<float>::infinity() ||
		lhs.outSlope[3] == std::numeric_limits<float>::infinity() || rhs.inSlope[3] == std::numeric_limits<float>::infinity() )
	{
		for (int i=0;i<4;i++)
		{
			coeff[0][i] = 0.0F;
			coeff[1][i] = 0.0F;
			coeff[2][i] = 0.0F;
			coeff[3][i] = lhs.value[i];
		}
	}
}

void HandleSteppedCurve (const KeyframeTpl<Quaternionf>& lhs, const KeyframeTpl<Quaternionf>& rhs, Quaternionf& value)
{
	if (lhs.outSlope[0] == std::numeric_limits<float>::infinity() || rhs.inSlope[0] == std::numeric_limits<float>::infinity() ||
		lhs.outSlope[1] == std::numeric_limits<float>::infinity() || rhs.inSlope[1] == std::numeric_limits<float>::infinity() ||
		lhs.outSlope[2] == std::numeric_limits<float>::infinity() || rhs.inSlope[2] == std::numeric_limits<float>::infinity() ||
		lhs.outSlope[3] == std::numeric_limits<float>::infinity() || rhs.inSlope[3] == std::numeric_limits<float>::infinity() )
	{
		value = lhs.value;
	}
}

void HandleSteppedTangent (const KeyframeTpl<Quaternionf>& lhs, const KeyframeTpl<Quaternionf>& rhs, Quaternionf& tangent)
{
	for (int i=0;i<4;i++)
	{
		if (lhs.outSlope[i] == std::numeric_limits<float>::infinity() || rhs.inSlope[i] == std::numeric_limits<float>::infinity())
			tangent[i] = std::numeric_limits<float>::infinity();
	}
}

template<class T>
void AnimationCurveTpl<T>::CalculateCacheData (Cache& cache, int lhsIndex, int rhsIndex, float timeOffset) const
{
	const Keyframe& lhs = m_Curve[lhsIndex];
	const Keyframe& rhs = m_Curve[rhsIndex];
//	DebugAssertIf (timeOffset < -0.001F || timeOffset - 0.001F > rhs.time - lhs.time);
	cache.index = lhsIndex;
	cache.time = lhs.time + timeOffset;
	cache.timeEnd = rhs.time + timeOffset;
	cache.index = lhsIndex;

	float dx, length;
	T dy;
	T m1, m2, d1, d2;

	dx = rhs.time - lhs.time;
	dx = max(dx, 0.0001F);
	dy = rhs.value - lhs.value;
	length = 1.0F / (dx * dx);

	m1 = lhs.outSlope;
	m2 = rhs.inSlope;
	d1 = m1 * dx;
	d2 = m2 * dx;

	cache.coeff[0] = (d1 + d2 - dy - dy) * length / dx;
	cache.coeff[1] = (dy + dy + dy - d1 - d1 - d2) * length;
	cache.coeff[2] = m1;
	cache.coeff[3] = lhs.value;
	SetupStepped(cache.coeff, lhs, rhs);
}

// When we look for the next index, how many keyframes do we just loop ahead instead of binary searching?
#define SEARCH_AHEAD 3
///@TODO: Cleanup old code to completely get rid of this
template<class T>
int AnimationCurveTpl<T>::FindIndex (const Cache& cache, float curveT) const
{
	#if SEARCH_AHEAD >= 0
	int cacheIndex = cache.index;
	if (cacheIndex != -1)
	{
		// We can not use the cache time or time end since that is in unwrapped time space!
		float time = m_Curve[cacheIndex].time;

		if (curveT > time)
		{
			if (cacheIndex + SEARCH_AHEAD < static_cast<int>(m_Curve.size()))
			{
				for (int i=0;i<SEARCH_AHEAD;i++)
				{
					if (curveT < m_Curve[cacheIndex + i + 1].time)
						return cacheIndex + i;
				}
			}
		}
		else
		{
			if (cacheIndex - SEARCH_AHEAD >= 0)
			{
				for (int i=0;i<SEARCH_AHEAD;i++)
				{
					if (curveT > m_Curve[cacheIndex - i - 1].time)
						return cacheIndex - i - 1;
				}
			}
		}
	}

	#endif

	///@ use cache to index into next if not possible use binary search
	const_iterator i = std::lower_bound (m_Curve.begin (), m_Curve.end (), curveT, KeyframeCompare());
	int index = distance (m_Curve.begin (), i);
	index--;
	index = min<int> (m_Curve.size () - 2, index);
	index = max<int> (0, index);

	return index;
}

///@TODO: Cleanup old code to completely get rid of this
template<class T>
int AnimationCurveTpl<T>::FindIndex (float curveT) const
{
	pair<float, float> range = GetRange ();
	if (curveT <= range.first || curveT >= range.second)
		return -1;

	const_iterator i = std::lower_bound (m_Curve.begin (), m_Curve.end (), curveT, KeyframeCompare());
	int index = distance (m_Curve.begin (), i);
	index--;
	index = min<int> (m_Curve.size () - 2, index);
	index = max<int> (0, index);

	return index;
}

template<class T>
void AnimationCurveTpl<T>::FindIndexForSampling (const Cache& cache, float curveT, int& lhs, int& rhs) const
{
	int actualSize = m_Curve.size();
	const Keyframe* frames = &m_Curve[0];

	// Reference implementation:
	// (index is the last value that is equal to or smaller than curveT)
	#if 0
	int foundIndex = 0;
	for (int i=0;i<actualSize;i++)
	{
		if (frames[i].time <= curveT)
			foundIndex = i;
	}

	lhs = foundIndex;
	rhs = min<int>(lhs + 1, actualSize - 1);
	AssertIf (curveT < m_Curve[lhs].time || curveT > m_Curve[rhs].time);
	AssertIf(frames[rhs].time == curveT && frames[lhs].time != curveT)
	return;
	#endif


	#if SEARCH_AHEAD > 0
	int cacheIndex = cache.index;
	if (cacheIndex != -1)
	{
		// We can not use the cache time or time end since that is in unwrapped time space!
		float time = m_Curve[cacheIndex].time;

		if (curveT > time)
		{
			for (int i=0;i<SEARCH_AHEAD;i++)
			{
				int index = cacheIndex + i;
				if (index + 1 < actualSize && frames[index + 1].time > curveT)
				{
					lhs = index;

					rhs = min<int>(lhs + 1, actualSize - 1);
					return;
				}
			}
		}
		else
		{
			for (int i=0;i<SEARCH_AHEAD;i++)
			{
				int index = cacheIndex - i;
				if (index >= 0 && curveT >= frames[index].time)
				{
					lhs = index;
					rhs = min<int>(lhs + 1, actualSize - 1);
					return;
				}
			}
		}
	}

	#endif

	// Fall back to using binary search
	// upper bound (first value larger than curveT)
	int __len = actualSize;
	int __half;
	int __middle;
	int __first = 0;
	while (__len > 0)
	{
		__half = __len >> 1;
		__middle = __first + __half;

		if (curveT < frames[__middle].time)
			__len = __half;
		else
	    {
			__first = __middle;
			++__first;
			__len = __len - __half - 1;
	    }
	}

	// If not within range, we pick the last element twice
	lhs = __first - 1;
	rhs = min(actualSize - 1, __first);
}

template<class T>
void AnimationCurveTpl<T>::SetPreInfinity (int pre)
{
	m_PreInfinity = ToInternalInfinity(pre);
	InvalidateCache ();
}

template<class T>
void AnimationCurveTpl<T>::SetPostInfinity (int post)
{
	m_PostInfinity = ToInternalInfinity(post);
	InvalidateCache ();
}

template<class T>
int AnimationCurveTpl<T>::GetPreInfinity () const
{
	return FromInternalInfinity(m_PreInfinity);
}

template<class T>
int AnimationCurveTpl<T>::GetPostInfinity () const
{
	return FromInternalInfinity(m_PostInfinity);
}

template<class T>
T AnimationCurveTpl<T>::EvaluateClamp (float curveT) const
{
	T output;
	if (curveT >= m_ClampCache.time && curveT < m_ClampCache.timeEnd)
	{
//		AssertIf (!CompareApproximately (EvaluateCache (m_Cache, curveT), EvaluateWithoutCache (curveT), 0.001F));
		EvaluateCache<T> (m_ClampCache, curveT, output);
		return output;
	}
	else
	{
		float begTime = m_Curve[0].time;
		float endTime = m_Curve.back().time;

		if (curveT > endTime)
		{
			m_ClampCache.time = endTime;
			m_ClampCache.timeEnd = std::numeric_limits<float>::infinity ();
			m_ClampCache.coeff[0] = m_ClampCache.coeff[1] = m_ClampCache.coeff[2] = Zero<T>();
			m_ClampCache.coeff[3] = m_Curve[m_Curve.size()-1].value;
		}
		else if (curveT < begTime)
		{
			m_ClampCache.time = curveT - 1000.0F;
			m_ClampCache.timeEnd = begTime;
			m_ClampCache.coeff[0] = m_ClampCache.coeff[1] = m_ClampCache.coeff[2] = Zero<T>();
			m_ClampCache.coeff[3] = m_Curve[0].value;
		}
		else
		{
			int lhs, rhs;
			FindIndexForSampling (m_ClampCache, curveT, lhs, rhs);
			CalculateCacheData (m_ClampCache, lhs, rhs, 0.0F);
		}

//		AssertIf (!CompareApproximately (EvaluateCache (m_Cache, curveT), EvaluateWithoutCache (curveT), 0.001F));
		EvaluateCache<T> (m_ClampCache, curveT, output);
		return output;
	}
}

template<class T>
T AnimationCurveTpl<T>::Evaluate (float curveT) const
{
	int lhs, rhs;
	T output;
	if (curveT >= m_Cache.time && curveT < m_Cache.timeEnd)
	{
//		AssertIf (!CompareApproximately (EvaluateCache (m_Cache, curveT), EvaluateWithoutCache (curveT), 0.001F));
		EvaluateCache<T> (m_Cache, curveT, output);
		return output;
	}
	// @TODO: Optimize IsValid () away if by making the non-valid case always use the m_Cache codepath
	else if (IsValid ())
	{
		float begTime = m_Curve[0].time;
		float endTime = m_Curve.back().time;
		float wrappedTime;

		if (curveT >= endTime)
		{
			if (m_PostInfinity == kInternalClamp)
			{
				m_Cache.time = endTime;
				m_Cache.timeEnd = std::numeric_limits<float>::infinity ();
				m_Cache.coeff[0] = m_Cache.coeff[1] = m_Cache.coeff[2] = Zero<T>();
				m_Cache.coeff[3] = m_Curve[m_Curve.size()-1].value;
			}
			else if (m_PostInfinity == kInternalRepeat)
			{
				wrappedTime = Repeat (curveT, begTime, endTime);

				FindIndexForSampling (m_Cache, wrappedTime, lhs, rhs);
				CalculateCacheData (m_Cache, lhs, rhs, curveT - wrappedTime);
			}
			///@todo optimize pingpong by making it generate a cache too
			else
			{
				EvaluateWithoutCache (curveT, output);
				return output;
			}
		}
		else if (curveT < begTime)
		{
			if (m_PreInfinity == kInternalClamp)
			{
				m_Cache.time = curveT - 1000.0F;
				m_Cache.timeEnd = begTime;
				m_Cache.coeff[0] = m_Cache.coeff[1] = m_Cache.coeff[2] = Zero<T>();
				m_Cache.coeff[3] = m_Curve[0].value;
			}
			else if (m_PreInfinity == kInternalRepeat)
			{
				wrappedTime = Repeat (curveT, begTime, endTime);
				FindIndexForSampling (m_Cache, wrappedTime, lhs, rhs);
				CalculateCacheData (m_Cache, lhs, rhs, curveT - wrappedTime);
			}
			///@todo optimize pingpong by making it generate a cache too
			else
			{
				EvaluateWithoutCache (curveT, output);
				return output;
			}
		}
		else
		{
			FindIndexForSampling (m_Cache, curveT, lhs, rhs);
			CalculateCacheData (m_Cache, lhs, rhs, 0.0F);
		}

		//		AssertIf (!CompareApproximately (EvaluateCache (m_Cache, curveT), EvaluateWithoutCache (curveT), 0.001F));
		EvaluateCache<T> (m_Cache, curveT, output);
		return output;
	}
	else
	{
		if (m_Curve.size () == 1)
			return m_Curve.begin()->value;
		else
			return Zero<T> ();
	}
}

template<class T>
float AnimationCurveTpl<T>::WrapTime (float curveT) const
{
	float begTime = m_Curve[0].time;
	float endTime = m_Curve.back().time;

	if (curveT < begTime)
	{
		if (m_PreInfinity == kInternalClamp)
			curveT = begTime;
		else if (m_PreInfinity == kInternalPingPong)
			curveT = PingPong (curveT, begTime, endTime);
		else
			curveT = Repeat (curveT, begTime, endTime);
	}
	else if (curveT > endTime)
	{
		if (m_PostInfinity == kInternalClamp)
			curveT = endTime;
		else if (m_PostInfinity == kInternalPingPong)
			curveT = PingPong (curveT, begTime, endTime);
		else
			curveT = Repeat (curveT, begTime, endTime);
	}
	return curveT;
}

template<class T>
int AnimationCurveTpl<T>::AddKey (const Keyframe& key)
{
	InvalidateCache ();

    if (CompareApproximately(key.time, 0.0F, 0.0001F))
    {
        m_Curve.front() = key;
        return 0;
    }
    else if (CompareApproximately(key.time, 1.0F, 0.0001F))
    {
        m_Curve.back() = key;
        return m_Curve.size() - 1;
    }
    else
    {
        iterator i = std::lower_bound(m_Curve.begin(), m_Curve.end(), key);

        // is not included in container and value is not a duplicate
        if (i == end() || key < *i)
        {
            iterator ii = m_Curve.insert(i, key);
            return std::distance(m_Curve.begin(), ii);
        }
        else
            return -1;
    }
}

template<class T>
void AnimationCurveTpl<T>::RemoveKeys (iterator begin, iterator end)
{
	InvalidateCache ();
	m_Curve.erase (begin, end);
}

void ScaleCurveValue (AnimationCurve& curve, float scale)
{
	for (int i=0;i<curve.GetKeyCount ();i++)
	{
		curve.GetKey (i).value *= scale;
		curve.GetKey (i).inSlope *= scale;
		curve.GetKey (i).outSlope *= scale;
	}
	
	curve.InvalidateCache();
}

void OffsetCurveValue (AnimationCurve& curve, float offset)
{
	for (int i=0;i<curve.GetKeyCount ();i++)
		curve.GetKey (i).value += offset;
	
	curve.InvalidateCache();
}

void ScaleCurveTime (AnimationCurve& curve, float scale)
{
	for (int i=0;i<curve.GetKeyCount ();i++)
	{
		curve.GetKey (i).time *= scale;
		curve.GetKey (i).inSlope /= scale;
		curve.GetKey (i).outSlope /= scale;
	}
	curve.InvalidateCache();
}

void OffsetCurveTime (AnimationCurve& curve, float offset)
{
	for (int i=0;i<curve.GetKeyCount ();i++)
		curve.GetKey (i).time += offset;
	curve.InvalidateCache();
}


/*

Calculating tangents from a hermite spline () Realtime rendering page 56



> On this first pass we're stuck with linear keyframing, because we just
> don't have the cycles in game to go to splines. I know this would help a
> lot, but it isn't an option.

In Granny I do successive least-squares approximations do the data.  I
take the array of samples for a given channel (ie., position) which is 2x
oversampled or better from the art tool.  I then start by doing a
least-square solve for a spline of arbitrary degree with knots at either
end.  I compute the error over the spline from the original samples, and
add knots in the areas of highest error.  Repeat and salt to taste.

Since it's fairly easy to write a single solver that solves for any degree
of spline, I use the same solver for linear keyframes as I do for
quadratic keyframes, cubic keyframes, or even "0th order keframes", which
is to say if you don't want to interpolate _at all_, the solver can still
place the "stop-motion" key frames in the best locations.  But hopefully
no one is still doing that kind of animation.

If I were doing this over again (which I probably will at some point), I
would probably use some kind of weird waveletty scheme now.  I decided not
to do that originally, because I had about a month to do the entire
spline/solver/reduction thing the first time, and it had to be highly
optimized.  So I didn't want to have to learn wavelets and spline solvers
at the same time.  Next time I'll spend some time on wavelets and probably
use some sort of hierachical reduction scheme instead, primarily so you
can change how densely your keyframes are placed at run-time, and so you
can get hard edges easier (motions which are intended to have sharp
discontinuities aren't handled well at all by my current scheme).

- Casey



--

> Is anybody looking at the way errors add up?  eg. if you do some
> reduction on the hip, and the thigh, and the ankle bone channels, the
> result is a foot that moves quite a bit differently than it should.

This has been on my list for a long time.  I don't think it's a simple
case of error analysis though.  I think it's more a case for using
discrete skeletal changes or integrated IK, because hey, if what you care
about is having a particular thing stay in one place, then it seems to me
that the best way to compress that is by just saying "this stays in one
place", rather than trying to spend a lot of data on the joint curves
necessary to make that so.

> Also, is anybody doing things like using IK in the reducer, to make sure
> that even in the reduced version the feet stay in exactly the same spot?

That doesn't work with splines, unfortunately, because the splines are
continuous, and you will always have the feet slipping as a result (if not
at the keyframes, then in between for sure).  So you end up with the
problem that the IK reducer would need to shove a metric assload of keys
into the streams, which defeats the compression.

From Ian:

> You said you are doing this on linear data as well. I can see that this
> would work, but do you find you get a fairly minimal result? I can
> envisage cases where you'd get many unneeded keyframes from initial
> 'breaks' at points of large error.

I'm not sure what you mean by this.  The error is controllable, so you say
how much you are willing to accept.  You get a minimal spline for the
error that you ask for, but no more - obviously in the areas of high
error, it has to add more keys, but that's what you want.  The objective
of compression, at least in my opinion, is not to remove detail, but
rather to more efficiently store places where there is less detail.

> I think I may be missing the point. Do you do least squares on the
> already fitted cures (ie, a chi squared test) then curve fit separately,
> or do you use least squares to fit your actual spline to the data?

The latter.  The incremental knot addition sets up the t_n's of an
arbitrary degree spline.  You have a matrix A that has sample-count rows
and knot-count columns.  The vector you're looking for is x, the spline
vector, which has knot-count rows.  You're producing b, the vector of
samples, which has sample-count rows.  So it's Ax = b, but A is
rectangular.  You solve via A^T Ax = A^Tb, a simple least squares problem.
You can solve it any way you like.  I chose a straightforward
implementation, because there's no numerical difficulties with these
things.

The version that comes with Granny is a highly optimized A^T Ax = A^Tb
solver, which constructs A^T A and A^T b directly and sparsely (so it is
O(n) in the number of samples, instead of O(n^3)).  The A^T A is band
diagonal, because of the sparsity pattern of A, so I use a sparse banded
cholesky solver on the back end.  It's _extremely_ fast.  In fact, the
stupid part of Granny's solver is actually the other part (the error
analysis + knot addition), because it adds very few knots per cycle, so on
a really long animation it can call the least-squares Ax = b solver many
hundreds of times for a single animation, and it _still_ never takes more
than 10 seconds or so even on animations with many hundred frames.  So
really, the right place to fix currently is the stupid knot addition
alogirithm, which really should be improved quite a bit.

> I have not used least squared before and from the reading I have done
> (numerical recipes, last night), it seem that the equation of the
> line/spline are inherent in the method and it needs to be reformulated
> to use a different line/spline. (The version I read was for fitting a
> straight line to a data-set, which I know won't work for quaternions
> slerps anyway)

Quaternion lerps are great, and work great with splines.  I use them
throughout all of Granny - there is no slerping anywhere.  Slerping is not
a very useful thing in a run-time engine, in my opinion, unless you are
dealing with orientations that are greater the 90 degrees apart.  It
allows me to use the same solver for position, rotation, AND scale/shear,
with no modifications.

- Casey


-----
> When is it best to use quaternions and when to use normal matrix
> rotations?

In Granny, I use quaternions for everything except the final composite
phase where rotations (and everything else) are multiplied by their
parents to produce the final world-space results.  Since we support
scale/shear, orientation, and position, it tends to be fastest at the
composition stage to convert the orientation from quaternion to matrix,
and then do all the matrix concatenation together.  I'm not sure I would
do the same if I didn't support scale/shear.  It might be a bit more fun
and efficient to go ahead and do the whole pipe in quaternion and only
convert to matrices at the very end (maybe somebody else has played with
that and can comment?)

> And for character animation with moving joints quaternions is usually
> used, but why?

There's lots of nice things about quaternions:

1) You can treat the linearly if you want to (so you can blend animations
quickly and easily)

2) You can treat them non-linearly if you want to (so you can do exact
geodesic interpolation at a constant speed)

3) You can convert them to a matrix with no transcendentals

4) They are compact, and can be easily converted to a 3-element
representation when necessary (ie., any one of the four values can be
easily re-generated from the other 3)

5) They cover 720 degrees of rotation, not 360 (if you do a lot of work
with character animation, you will appreciate this!), and the
neighborhooding operator is extremely simple and fast (inner product and a
negation)

6) They are easy to visualize (they're not much more complicated than
angle/axis) and manipulate (ie., the axis of rotation is obvious, you can
transform the axis of rotation without changing the amount of rotation,
and even just use a regular 3-vector transform, etc.)

7) You can easily transform vectors with them without converting to a
matrix if you want to

8) They are easy to renormalize, and they can never become "unorthogonal"

9) No gimbal lock

10) There is a simple, fast, and relevant distance metric (the inner
product)

> Is it just by trial and error that we decide when we have to use
> quarternions?

Well, I can't speak for everyone, but I have been extremely careful in my
selection of quaternions.  With Granny 1 I did quaternions to get some
practice with them, but when I did 2 I did a lot of research and wrote
code to visualize the actions of various rotational representations and
operations, and I can very confidently say there's no better choice for
general character animation than quaternions, at least among the
representations that I know (euler, angle/axis, matrix, quaternion, exp
map).  There are some other mappings that I've come across since I worked
everything out (like the Rational Map), that I have not experimented with,
because so far quaternions have been working superbly so I haven't had
much cause to go hunting.

> "Quarternions have the ability to have smooth interpollated rotations",
> I've made smooth interpollated rotations with normal matrix rotations
> though.

Well, it's not so much what you can do as how easy it is to do it.
Quaternions can be interpolated directly as a smooth geodesic (via slerp)
or via a nonlinear geodesic (lerp).  The latter is particularly powerful
because it distributes - it's a linear operator.  So you can literally
plug it in to anything that would work, like splines, multi-point blends,
etc., and it will "just work" (well, that's a bit of an exaggeration,
because there is a neighborhooding concern, but it's always easy to do).

> "They take less room, 4 elements versus 9 and some operations
> are cheaper in terms of CPU cycles". I accept this reason except someone
> said that quarternions use more CPU cycles, so I'm not sure who to
> believe.

It depends what you're doing.  Quaternions take more CPU cycles to
transform a vector than a matrix does.  But quaternions are MUCH less
expensive for lots of other operations, like, for example, splining.
This is why it's very useful to use quaternions for everything except the
final stage of your pipe, where matrices are more appropriate.

> "Quarternions are not susceptible to gimbal lock. Gimbal lock shows its
> face when two axes point in the same direction." So if no axes face in
> the same direction then this is not a reason to use quarternions.
> ...
> Am I right in saying that if no axes face the same direction it is
> possible to represent all rotations with normal matrix rotations?

Matrix rotations are not subject to gimbal lock.  That's Euler angles.

- Casey


Least squares is very simple.  Given a set of data points (2x oversampled in caseys case) and a knot vector, compute the coefficients for the control points of the spline that minimize squared error.  This involves solving a linear system where each row corresponds to a data point (di - they are uniformly space so each point has a corresponding ti which is in the domain of the curve) and each column corresponds to a control point for the curve (cj the unkowns).  So matrix element Aij is Bj(ti) where Bj is the basis function for the jth control point.  The right hand side is just a vector of the di and the x's are the unkown control points.  Sine you are compressing things there will be many more rows then columns in this linear system so you don't have to worry about over fitting problems (which you would if there were more DOF...)

-Peter-Pike

	-----Original Message-----
	From: Ian Elsley [mailto:ielsley@kushgames.com]
	Sent: Tue 12/17/2002 10:35 AM
	To: gdalgorithms-list@lists.sourceforge.net
	Cc:
	Subject: RE: [Algorithms] Re: Keyframe reduction



	Casey

	I think I may be missing the point. Do you do least squares on the
	already fitted cures (ie, a chi squared test) then curve fit separately,
	or do you use least squares to fit your actual spline to the data?

	I have not used least squared before and from the reading I have done
	(numerical recipes, last night), it seem that the equation of the
	line/spline are inherent in the method and it needs to be reformulated
	to use a different line/spline. (The version I read was for fitting a
	straight line to a data-set, which I know won't work for quaternions
	slerps anyway)

	I see the elegance of this method and would love to work it out, but
	I've got a few blanks here.

	Any pointers?

	Thanks in advance,

	Ian






         q
  q' = -----
        |q|

For renormalizing, since you are very close to 1, I usually use the
tangent-line approximation, which was suggested by Checker a long long
time ago for 3D vectors and works just peachy for 4D ones as well:

inline void NormalizeCloseToOne4(float *Dest)
{
    float const Sum = (Dest[0] * Dest[0] +
                       Dest[1] * Dest[1] +
                       Dest[2] * Dest[2] +
                       Dest[3] * Dest[3]);

    float const ApproximateOneOverRoot = (3.0f - Sum) * 0.5f;

    Dest[0] *= ApproximateOneOverRoot;
    Dest[1] *= ApproximateOneOverRoot;
    Dest[2] *= ApproximateOneOverRoot;
    Dest[3] *= ApproximateOneOverRoot;
}


*/



#define INSTANTIATE(T) \
template EXPORT_COREMODULE std::pair<float, float> AnimationCurveTpl<T>::GetRange() const; \
template EXPORT_COREMODULE T AnimationCurveTpl<T>::Evaluate (float curveT) const; \
template EXPORT_COREMODULE void AnimationCurveTpl<T>::RemoveKeys (iterator begin, iterator end);\
template EXPORT_COREMODULE int AnimationCurveTpl<T>::AddKey (const Keyframe& key);\
template EXPORT_COREMODULE int AnimationCurveTpl<T>::FindIndex (float time) const; \
template EXPORT_COREMODULE int AnimationCurveTpl<T>::FindIndex (const Cache& cache, float time) const; \
template EXPORT_COREMODULE void AnimationCurveTpl<T>::InvalidateCache (); \
template EXPORT_COREMODULE void AnimationCurveTpl<T>::SetPreInfinity (int mode); \
template EXPORT_COREMODULE void AnimationCurveTpl<T>::SetPostInfinity (int mode); \
template EXPORT_COREMODULE int AnimationCurveTpl<T>::GetPreInfinity () const; \
template EXPORT_COREMODULE int AnimationCurveTpl<T>::GetPostInfinity () const; \
template EXPORT_COREMODULE KeyframeTpl<T>::KeyframeTpl (float time, const T& value);

INSTANTIATE(float)
INSTANTIATE(Vector3f)
INSTANTIATE(Quaternionf)

template EXPORT_COREMODULE void AnimationCurveTpl<float>::CalculateCacheData (Cache& cache, int lhs, int rhs, float timeOffset) const;
template EXPORT_COREMODULE float AnimationCurveTpl<float>::EvaluateClamp (float curveT) const;
template EXPORT_COREMODULE Quaternionf AnimationCurveTpl<Quaternionf>::EvaluateClamp (float curveT) const;
template EXPORT_COREMODULE Vector3f AnimationCurveTpl<Vector3f>::EvaluateClamp (float curveT) const;
