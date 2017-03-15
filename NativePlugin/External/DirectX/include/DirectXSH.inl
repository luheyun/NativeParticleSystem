//-------------------------------------------------------------------------------------
// DirectXSH.inl -- Inline spherical harmonics reference implementation
//
// This is a stripped down version of DirectXSH.cpp/.h & DirectXMath.h needed to run
// Unity's spherical harmonics tests against a reference implementation.
//
// See https://github.com/walbourn/directxmathsh and DirectXMath.h in the windows SDK
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//  
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

/*
                               The MIT License (MIT)

Copyright (c) 2015 Microsoft Corp

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, 
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to the following 
conditions: 

The above copyright notice and this permission notice shall be included in all copies 
or substantial portions of the Software.  

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#if defined(_MSC_VER) && !defined(_M_ARM) && (!_MANAGED) && (!_M_CEE) && (!defined(_M_IX86_FP) || (_M_IX86_FP > 1)) && !defined(_XM_NO_INTRINSICS_) && !defined(_XM_VECTORCALL_)
#if ((_MSC_FULL_VER >= 170065501) && (_MSC_VER < 1800)) || (_MSC_FULL_VER >= 180020418)
#define _XM_VECTORCALL_ 1
#endif
#endif

#if !defined(_XM_ARM_NEON_INTRINSICS_) && !defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
#if defined(_M_IX86) || defined(_M_X64)
#define _XM_SSE_INTRINSICS_
#elif defined(_M_ARM)
#define _XM_ARM_NEON_INTRINSICS_
#elif !defined(_XM_NO_INTRINSICS_)
#error DirectX Math does not support this target
#endif
#endif // !_XM_ARM_NEON_INTRINSICS_ && !_XM_SSE_INTRINSICS_ && !_XM_NO_INTRINSICS_

#pragma warning(push)
#pragma warning(disable:4514 4820 4985)
#include <math.h>
#include <float.h>
#include <malloc.h>
#pragma warning(pop)


#if defined(_XM_SSE_INTRINSICS_)
#ifndef _XM_NO_INTRINSICS_
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
#elif defined(_XM_ARM_NEON_INTRINSICS_)
#ifndef _XM_NO_INTRINSICS_
#pragma warning(push)
#pragma warning(disable : 4987)
#include <intrin.h>
#pragma warning(pop)
#include <arm_neon.h>
#endif
#endif

#include <sal.h>
#include <assert.h>

#ifndef _XM_NO_ROUNDF_
#ifdef _MSC_VER
#include <yvals.h>
#if defined(_CPPLIB_VER) && ( _CPPLIB_VER < 610 )
#define _XM_NO_ROUNDF_
#endif
#endif
#endif

#pragma warning(push)
#pragma warning(disable : 4005 4668)
#include <stdint.h>
#pragma warning(pop)

#if defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)

#define XM_PERMUTE_PS( v, c ) _mm_shuffle_ps( v, v, c )

#endif // _XM_SSE_INTRINSICS_ && !_XM_NO_INTRINSICS_

namespace DirectX
{
const float XM_PI           = 3.141592654f;

#pragma warning(push)
#pragma warning(disable:4068 4201 4365 4324 4820)

//------------------------------------------------------------------------------
#if defined(_XM_NO_INTRINSICS_)
// The __vector4 structure is an intrinsic on Xbox but must be separately defined
// for x86/x64
struct __vector4
{
    union
    {
        float       vector4_f32[4];
        uint32_t    vector4_u32[4];
    };
};
#endif // _XM_NO_INTRINSICS_

//------------------------------------------------------------------------------
// Vector intrinsic: Four 32 bit floating point components aligned on a 16 byte 
// boundary and mapped to hardware vector registers
#if defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
typedef __m128 XMVECTOR;
#elif defined(_XM_ARM_NEON_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
typedef float32x4_t XMVECTOR;
#else
typedef __vector4 XMVECTOR;
#endif

// Fix-up for (1st-3rd) XMVECTOR parameters that are pass-in-register for x86, ARM, and vector call; by reference otherwise
#if ( defined(_M_IX86) || defined(_M_ARM) || _XM_VECTORCALL_ ) && !defined(_XM_NO_INTRINSICS_)
typedef const XMVECTOR FXMVECTOR;
#else
typedef const XMVECTOR& FXMVECTOR;
#endif

//------------------------------------------------------------------------------
// Conversion types for constants
__declspec(align(16)) struct XMVECTORF32
{
    union
    {
        float f[4];
        XMVECTOR v;
    };

    inline operator XMVECTOR() const { return v; }
    inline operator const float*() const { return f; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return _mm_castps_si128(v); }
    inline operator __m128d() const { return _mm_castps_pd(v); }
#endif
};

//------------------------------------------------------------------------------
// 3D Vector; 32 bit floating point components
struct XMFLOAT3
{
    float x;
    float y;
    float z;

    XMFLOAT3() {}
    XMFLOAT3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    explicit XMFLOAT3(const float *pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}

    XMFLOAT3& operator= (const XMFLOAT3& Float3) { x = Float3.x; y = Float3.y; z = Float3.z; return *this; }
};

// 3D Vector; 32 bit floating point components aligned on a 16 byte boundary
__declspec(align(16)) struct XMFLOAT3A : public XMFLOAT3
{
    XMFLOAT3A() : XMFLOAT3() {}
    XMFLOAT3A(float _x, float _y, float _z) : XMFLOAT3(_x, _y, _z) {}
    explicit XMFLOAT3A(const float *pArray) : XMFLOAT3(pArray) {}

    XMFLOAT3A& operator= (const XMFLOAT3A& Float3) { x = Float3.x; y = Float3.y; z = Float3.z; return *this; }
};

//------------------------------------------------------------------------------
// 4D Vector; 32 bit floating point components
struct XMFLOAT4
{
    float x;
    float y;
    float z;
    float w;

    XMFLOAT4() {}
    XMFLOAT4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    explicit XMFLOAT4(const float *pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}

    XMFLOAT4& operator= (const XMFLOAT4& Float4) { x = Float4.x; y = Float4.y; z = Float4.z; w = Float4.w; return *this; }
};

// 4D Vector; 32 bit floating point components aligned on a 16 byte boundary
__declspec(align(16)) struct XMFLOAT4A : public XMFLOAT4
{
    XMFLOAT4A() : XMFLOAT4() {}
    XMFLOAT4A(float _x, float _y, float _z, float _w) : XMFLOAT4(_x, _y, _z, _w) {}
    explicit XMFLOAT4A(const float *pArray) : XMFLOAT4(pArray) {}

    XMFLOAT4A& operator= (const XMFLOAT4A& Float4) { x = Float4.x; y = Float4.y; z = Float4.z; w = Float4.w; return *this; }
};

////////////////////////////////////////////////////////////////////////////////

#pragma warning(pop)

//------------------------------------------------------------------------------
// Initialize a vector with four floating point values
inline XMVECTOR  XMVectorSet
(
    float x, 
    float y, 
    float z, 
    float w
)
{
#if defined(_XM_NO_INTRINSICS_)
    XMVECTORF32 vResult = {x,y,z,w};
    return vResult.v;
#elif defined(_XM_ARM_NEON_INTRINSICS_)
    float32x2_t V0 = vcreate_f32(((uint64_t)*(const uint32_t *)&x) | ((uint64_t)(*(const uint32_t *)&y) << 32));
    float32x2_t V1 = vcreate_f32(((uint64_t)*(const uint32_t *)&z) | ((uint64_t)(*(const uint32_t *)&w) << 32));
    return vcombine_f32(V0, V1);
#elif defined(_XM_SSE_INTRINSICS_)
    return _mm_set_ps( w, z, y, x );
#endif
}


//------------------------------------------------------------------------------
inline void  XMStoreFloat4A
(
    XMFLOAT4A*   pDestination, 
    FXMVECTOR     V
)
{
    assert(pDestination);
    assert(((uintptr_t)pDestination & 0xF) == 0);
#if defined(_XM_NO_INTRINSICS_)
    pDestination->x = V.vector4_f32[0];
    pDestination->y = V.vector4_f32[1];
    pDestination->z = V.vector4_f32[2];
    pDestination->w = V.vector4_f32[3];
#elif defined(_XM_ARM_NEON_INTRINSICS_)
    vst1q_f32_ex( reinterpret_cast<float*>(pDestination), V, 128 );
#elif defined(_XM_SSE_INTRINSICS_)
    _mm_store_ps( &pDestination->x, V );
#endif
}

//------------------------------------------------------------------------------
inline void  XMStoreFloat3A
(
    XMFLOAT3A*   pDestination, 
    FXMVECTOR     V
)
{
    assert(pDestination);
    assert(((uintptr_t)pDestination & 0xF) == 0);
#if defined(_XM_NO_INTRINSICS_)
    pDestination->x = V.vector4_f32[0];
    pDestination->y = V.vector4_f32[1];
    pDestination->z = V.vector4_f32[2];
#elif defined(_XM_ARM_NEON_INTRINSICS_)
    float32x2_t VL = vget_low_f32(V);
    vst1_f32_ex( reinterpret_cast<float*>(pDestination), VL, 64 );
    vst1q_lane_f32( reinterpret_cast<float*>(pDestination)+2, V, 2 );
#elif defined(_XM_SSE_INTRINSICS_)
    XMVECTOR T = XM_PERMUTE_PS(V,_MM_SHUFFLE(2,2,2,2));
    _mm_storel_epi64( reinterpret_cast<__m128i*>(pDestination), _mm_castps_si128(V) );
    _mm_store_ss( &pDestination->z, T );
#endif
}

const size_t XM_SH_MINORDER = 2;
const size_t XM_SH_MAXORDER = 6;

// routine generated programmatically for evaluating SH basis for degree 1
// inputs (x,y,z) are a point on the sphere (i.e., must be unit length)
// output is vector b with SH basis evaluated at (x,y,z).
//
inline static void sh_eval_basis_1(float x,float y,float z,float b[4])
{
	/* m=0 */

	// l=0
	const float p_0_0 = 0.282094791773878140f;
	b[  0] = p_0_0; // l=0,m=0
	// l=1
	const float p_1_0 = 0.488602511902919920f*z;
	b[  2] = p_1_0; // l=1,m=0


	/* m=1 */

	const float s1 = y;
	const float c1 = x;

	// l=1
	const float p_1_1 = -0.488602511902919920f;
	b[  1] = p_1_1*s1; // l=1,m=-1
	b[  3] = p_1_1*c1; // l=1,m=+1
}

// routine generated programmatically for evaluating SH basis for degree 2
// inputs (x,y,z) are a point on the sphere (i.e., must be unit length)
// output is vector b with SH basis evaluated at (x,y,z).
//
inline static void sh_eval_basis_2(float x,float y,float z,float b[9])
{
	const float z2 = z*z;


	/* m=0 */

	// l=0
	const float p_0_0 = 0.282094791773878140f;
	b[  0] = p_0_0; // l=0,m=0
	// l=1
	const float p_1_0 = 0.488602511902919920f*z;
	b[  2] = p_1_0; // l=1,m=0
	// l=2
	const float p_2_0 = 0.946174695757560080f*z2 + -0.315391565252520050f;
	b[  6] = p_2_0; // l=2,m=0


	/* m=1 */

	const float s1 = y;
	const float c1 = x;

	// l=1
	const float p_1_1 = -0.488602511902919920f;
	b[  1] = p_1_1*s1; // l=1,m=-1
	b[  3] = p_1_1*c1; // l=1,m=+1
	// l=2
	const float p_2_1 = -1.092548430592079200f*z;
	b[  5] = p_2_1*s1; // l=2,m=-1
	b[  7] = p_2_1*c1; // l=2,m=+1


	/* m=2 */

	const float s2 = x*s1 + y*c1;
	const float c2 = x*c1 - y*s1;

	// l=2
	const float p_2_2 = 0.546274215296039590f;
	b[  4] = p_2_2*s2; // l=2,m=-2
	b[  8] = p_2_2*c2; // l=2,m=+2
}

// routine generated programmatically for evaluating SH basis for degree 3
// inputs (x,y,z) are a point on the sphere (i.e., must be unit length)
// output is vector b with SH basis evaluated at (x,y,z).
//
static void sh_eval_basis_3(float x,float y,float z,float b[16])
{
	const float z2 = z*z;


	/* m=0 */

	// l=0
	const float p_0_0 = 0.282094791773878140f;
	b[  0] = p_0_0; // l=0,m=0
	// l=1
	const float p_1_0 = 0.488602511902919920f*z;
	b[  2] = p_1_0; // l=1,m=0
	// l=2
	const float p_2_0 = 0.946174695757560080f*z2 + -0.315391565252520050f;
	b[  6] = p_2_0; // l=2,m=0
	// l=3
	const float p_3_0 = z*(1.865881662950577000f*z2 + -1.119528997770346200f);
	b[ 12] = p_3_0; // l=3,m=0


	/* m=1 */

	const float s1 = y;
	const float c1 = x;

	// l=1
	const float p_1_1 = -0.488602511902919920f;
	b[  1] = p_1_1*s1; // l=1,m=-1
	b[  3] = p_1_1*c1; // l=1,m=+1
	// l=2
	const float p_2_1 = -1.092548430592079200f*z;
	b[  5] = p_2_1*s1; // l=2,m=-1
	b[  7] = p_2_1*c1; // l=2,m=+1
	// l=3
	const float p_3_1 = -2.285228997322328800f*z2 + 0.457045799464465770f;
	b[ 11] = p_3_1*s1; // l=3,m=-1
	b[ 13] = p_3_1*c1; // l=3,m=+1


	/* m=2 */

	const float s2 = x*s1 + y*c1;
	const float c2 = x*c1 - y*s1;

	// l=2
	const float p_2_2 = 0.546274215296039590f;
	b[  4] = p_2_2*s2; // l=2,m=-2
	b[  8] = p_2_2*c2; // l=2,m=+2
	// l=3
	const float p_3_2 = 1.445305721320277100f*z;
	b[ 10] = p_3_2*s2; // l=3,m=-2
	b[ 14] = p_3_2*c2; // l=3,m=+2


	/* m=3 */

	const float s3 = x*s2 + y*c2;
	const float c3 = x*c2 - y*s2;

	// l=3
	const float p_3_3 = -0.590043589926643520f;
	b[  9] = p_3_3*s3; // l=3,m=-3
	b[ 15] = p_3_3*c3; // l=3,m=+3
}

// routine generated programmatically for evaluating SH basis for degree 4
// inputs (x,y,z) are a point on the sphere (i.e., must be unit length)
// output is vector b with SH basis evaluated at (x,y,z).
//
static void sh_eval_basis_4(float x,float y,float z,float b[25])
{
	const float z2 = z*z;


	/* m=0 */

	// l=0
	const float p_0_0 = 0.282094791773878140f;
	b[  0] = p_0_0; // l=0,m=0
	// l=1
	const float p_1_0 = 0.488602511902919920f*z;
	b[  2] = p_1_0; // l=1,m=0
	// l=2
	const float p_2_0 = 0.946174695757560080f*z2 + -0.315391565252520050f;
	b[  6] = p_2_0; // l=2,m=0
	// l=3
	const float p_3_0 = z*(1.865881662950577000f*z2 + -1.119528997770346200f);
	b[ 12] = p_3_0; // l=3,m=0
	// l=4
	const float p_4_0 = 1.984313483298443000f*z*p_3_0 + -1.006230589874905300f*p_2_0;
	b[ 20] = p_4_0; // l=4,m=0


	/* m=1 */

	const float s1 = y;
	const float c1 = x;

	// l=1
	const float p_1_1 = -0.488602511902919920f;
	b[  1] = p_1_1*s1; // l=1,m=-1
	b[  3] = p_1_1*c1; // l=1,m=+1
	// l=2
	const float p_2_1 = -1.092548430592079200f*z;
	b[  5] = p_2_1*s1; // l=2,m=-1
	b[  7] = p_2_1*c1; // l=2,m=+1
	// l=3
	const float p_3_1 = -2.285228997322328800f*z2 + 0.457045799464465770f;
	b[ 11] = p_3_1*s1; // l=3,m=-1
	b[ 13] = p_3_1*c1; // l=3,m=+1
	// l=4
	const float p_4_1 = z*(-4.683325804901024000f*z2 + 2.007139630671867200f);
	b[ 19] = p_4_1*s1; // l=4,m=-1
	b[ 21] = p_4_1*c1; // l=4,m=+1


	/* m=2 */

	const float s2 = x*s1 + y*c1;
	const float c2 = x*c1 - y*s1;

	// l=2
	const float p_2_2 = 0.546274215296039590f;
	b[  4] = p_2_2*s2; // l=2,m=-2
	b[  8] = p_2_2*c2; // l=2,m=+2
	// l=3
	const float p_3_2 = 1.445305721320277100f*z;
	b[ 10] = p_3_2*s2; // l=3,m=-2
	b[ 14] = p_3_2*c2; // l=3,m=+2
	// l=4
	const float p_4_2 = 3.311611435151459800f*z2 + -0.473087347878779980f;
	b[ 18] = p_4_2*s2; // l=4,m=-2
	b[ 22] = p_4_2*c2; // l=4,m=+2


	/* m=3 */

	const float s3 = x*s2 + y*c2;
	const float c3 = x*c2 - y*s2;

	// l=3
	const float p_3_3 = -0.590043589926643520f;
	b[  9] = p_3_3*s3; // l=3,m=-3
	b[ 15] = p_3_3*c3; // l=3,m=+3
	// l=4
	const float p_4_3 = -1.770130769779930200f*z;
	b[ 17] = p_4_3*s3; // l=4,m=-3
	b[ 23] = p_4_3*c3; // l=4,m=+3


	/* m=4 */

	const float s4 = x*s3 + y*c3;
	const float c4 = x*c3 - y*s3;

	// l=4
	const float p_4_4 = 0.625835735449176030f;
	b[ 16] = p_4_4*s4; // l=4,m=-4
	b[ 24] = p_4_4*c4; // l=4,m=+4
}

// routine generated programmatically for evaluating SH basis for degree 5
// inputs (x,y,z) are a point on the sphere (i.e., must be unit length)
// output is vector b with SH basis evaluated at (x,y,z).
//
static void sh_eval_basis_5(float x,float y,float z,float b[36])
{
	const float z2 = z*z;


	/* m=0 */

	// l=0
	const float p_0_0 = 0.282094791773878140f;
	b[  0] = p_0_0; // l=0,m=0
	// l=1
	const float p_1_0 = 0.488602511902919920f*z;
	b[  2] = p_1_0; // l=1,m=0
	// l=2
	const float p_2_0 = 0.946174695757560080f*z2 + -0.315391565252520050f;
	b[  6] = p_2_0; // l=2,m=0
	// l=3
	const float p_3_0 = z*(1.865881662950577000f*z2 + -1.119528997770346200f);
	b[ 12] = p_3_0; // l=3,m=0
	// l=4
	const float p_4_0 = 1.984313483298443000f*z*p_3_0 + -1.006230589874905300f*p_2_0;
	b[ 20] = p_4_0; // l=4,m=0
	// l=5
	const float p_5_0 = 1.989974874213239700f*z*p_4_0 + -1.002853072844814000f*p_3_0;
	b[ 30] = p_5_0; // l=5,m=0


	/* m=1 */

	const float s1 = y;
	const float c1 = x;

	// l=1
	const float p_1_1 = -0.488602511902919920f;
	b[  1] = p_1_1*s1; // l=1,m=-1
	b[  3] = p_1_1*c1; // l=1,m=+1
	// l=2
	const float p_2_1 = -1.092548430592079200f*z;
	b[  5] = p_2_1*s1; // l=2,m=-1
	b[  7] = p_2_1*c1; // l=2,m=+1
	// l=3
	const float p_3_1 = -2.285228997322328800f*z2 + 0.457045799464465770f;
	b[ 11] = p_3_1*s1; // l=3,m=-1
	b[ 13] = p_3_1*c1; // l=3,m=+1
	// l=4
	const float p_4_1 = z*(-4.683325804901024000f*z2 + 2.007139630671867200f);
	b[ 19] = p_4_1*s1; // l=4,m=-1
	b[ 21] = p_4_1*c1; // l=4,m=+1
	// l=5
	const float p_5_1 = 2.031009601158990200f*z*p_4_1 + -0.991031208965114650f*p_3_1;
	b[ 29] = p_5_1*s1; // l=5,m=-1
	b[ 31] = p_5_1*c1; // l=5,m=+1


	/* m=2 */

	const float s2 = x*s1 + y*c1;
	const float c2 = x*c1 - y*s1;

	// l=2
	const float p_2_2 = 0.546274215296039590f;
	b[  4] = p_2_2*s2; // l=2,m=-2
	b[  8] = p_2_2*c2; // l=2,m=+2
	// l=3
	const float p_3_2 = 1.445305721320277100f*z;
	b[ 10] = p_3_2*s2; // l=3,m=-2
	b[ 14] = p_3_2*c2; // l=3,m=+2
	// l=4
	const float p_4_2 = 3.311611435151459800f*z2 + -0.473087347878779980f;
	b[ 18] = p_4_2*s2; // l=4,m=-2
	b[ 22] = p_4_2*c2; // l=4,m=+2
	// l=5
	const float p_5_2 = z*(7.190305177459987500f*z2 + -2.396768392486662100f);
	b[ 28] = p_5_2*s2; // l=5,m=-2
	b[ 32] = p_5_2*c2; // l=5,m=+2


	/* m=3 */

	const float s3 = x*s2 + y*c2;
	const float c3 = x*c2 - y*s2;

	// l=3
	const float p_3_3 = -0.590043589926643520f;
	b[  9] = p_3_3*s3; // l=3,m=-3
	b[ 15] = p_3_3*c3; // l=3,m=+3
	// l=4
	const float p_4_3 = -1.770130769779930200f*z;
	b[ 17] = p_4_3*s3; // l=4,m=-3
	b[ 23] = p_4_3*c3; // l=4,m=+3
	// l=5
	const float p_5_3 = -4.403144694917253700f*z2 + 0.489238299435250430f;
	b[ 27] = p_5_3*s3; // l=5,m=-3
	b[ 33] = p_5_3*c3; // l=5,m=+3


	/* m=4 */

	const float s4 = x*s3 + y*c3;
	const float c4 = x*c3 - y*s3;

	// l=4
	const float p_4_4 = 0.625835735449176030f;
	b[ 16] = p_4_4*s4; // l=4,m=-4
	b[ 24] = p_4_4*c4; // l=4,m=+4
	// l=5
	const float p_5_4 = 2.075662314881041100f*z;
	b[ 26] = p_5_4*s4; // l=5,m=-4
	b[ 34] = p_5_4*c4; // l=5,m=+4


	/* m=5 */

	const float s5 = x*s4 + y*c4;
	const float c5 = x*c4 - y*s4;

	// l=5
	const float p_5_5 = -0.656382056840170150f;
	b[ 25] = p_5_5*s5; // l=5,m=-5
	b[ 35] = p_5_5*c5; // l=5,m=+5
}

//-------------------------------------------------------------------------------------
// Evaluates the Spherical Harmonic basis functions
//
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb205448.aspx
//-------------------------------------------------------------------------------------
float*  XMSHEvalDirection( float *result,
                           size_t order,
                           FXMVECTOR dir )
{
    if ( !result )
        return nullptr;

    XMFLOAT4A dv;
    XMStoreFloat4A( &dv, dir );

    const float fX = dv.x;
    const float fY = dv.y;
    const float fZ = dv.z;

    switch( order )
    {
    case 2:
        sh_eval_basis_1(fX,fY,fZ,result);
        break;

    case 3:
        sh_eval_basis_2(fX,fY,fZ,result);
        break;

    case 4:
        sh_eval_basis_3(fX,fY,fZ,result);
        break;

    case 5:
        sh_eval_basis_4(fX,fY,fZ,result);
        break;

    case 6:
        sh_eval_basis_5(fX,fY,fZ,result);
        break;

    default:
        assert( order < XM_SH_MINORDER || order > XM_SH_MAXORDER );
        return nullptr;
    }

    return result;
}

//-------------------------------------------------------------------------------------
// Evaluates a directional light and returns spectral SH data.  The output 
// vector is computed so that if the intensity of R/G/B is unit the resulting
// exit radiance of a point directly under the light on a diffuse object with
// an albedo of 1 would be 1.0.  This will compute 3 spectral samples, resultR
// has to be specified, while resultG and resultB are optional.
//
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb204988.aspx
//-------------------------------------------------------------------------------------
bool  XMSHEvalDirectionalLight( size_t order,
                                FXMVECTOR dir,
                                FXMVECTOR color,
                                float *resultR,
                                float *resultG,
                                float *resultB )
{
    if ( !resultR )
        return false;

    if ( order < XM_SH_MINORDER || order > XM_SH_MAXORDER )
        return false;

    XMFLOAT3A clr;
    XMStoreFloat3A( &clr, color );

    float fTmp[ XM_SH_MAXORDER * XM_SH_MAXORDER ];

    XMSHEvalDirection(fTmp,order,dir); // evaluate the BF in this direction...

	float CosWtInt;

    // input pF only consists of Yl0 values, normalizes coefficients for directional
    // lights.
    {
        const float fCW0 = 0.25f;
        const float fCW1 = 0.5f;
        const float fCW2 = 5.0f/16.0f;
        //const float fCW3 = 0.0f;
        const float fCW4 = -3.0f/32.0f;
        //const float fCW5 = 0.0f;

        // order has to be at least linear...

        float fRet = fCW0 + fCW1;

        if (order > 2) fRet += fCW2;
        if (order > 4) fRet += fCW4;

        // odd degrees >= 3 evaluate to zero integrated against cosine...

        CosWtInt = fRet;
    }

    // now compute "normalization" and scale vector for each valid spectral band
    const float fNorm = XM_PI / CosWtInt;

    const size_t numcoeff = order*order;

    const float fRScale = fNorm * clr.x;

    for( size_t i=0; i < numcoeff; ++i)
    {
        resultR[i] = fTmp[i] * fRScale;
    }

    if (resultG)
    {
        const float fGScale = fNorm * clr.y;

        for( size_t i=0; i < numcoeff; ++i)
        {
            resultG[i] = fTmp[i] * fGScale;
        }
    }

    if (resultB)
    {
        const float fBScale = fNorm * clr.z;

        for( size_t i=0; i < numcoeff; ++i)
        {
            resultB[i] = fTmp[i]*fBScale;
        }
    }

    return true;
}

}; // namespace DirectX
