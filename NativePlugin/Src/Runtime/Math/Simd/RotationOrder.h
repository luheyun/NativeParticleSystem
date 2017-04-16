#ifndef _MATH_SIMD2_ROTATION_ORDER_H_
#define _MATH_SIMD2_ROTATION_ORDER_H_

namespace math
{
	// This Enum needs to stay synchronized with the one in the bindings Runtime\Export\UnityEngineTransform.bindings
	enum RotationOrder
	{
		kOrderXYZ,
		kOrderXZY,
		kOrderYZX,
		kOrderYXZ,
		kOrderZXY,
		kOrderZYX,
		kRotationOrderLast = kOrderZYX,
		kOrderUnityDefault = kOrderZXY
	};

	const int kRotationOrderCount = kRotationOrderLast + 1;
}

#endif
