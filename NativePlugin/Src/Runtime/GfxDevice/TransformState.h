#pragma once

#include "GfxDeviceTypes.h"
#include "Math/Matrix4x4.h"
#include "BuiltinShaderParams.h"

struct TransformState
{
public:
	void Invalidate(BuiltinShaderParamValues& builtins);
	void UpdateWorldViewMatrix (const BuiltinShaderParamValues& builtins) const;
	void SetViewMatrix (const Matrix4x4f& matrix, BuiltinShaderParamValues& builtins);

	enum
	{
		kWorldDirty			= (1<<0),
		kViewDirty			= (1<<1),
		kProjDirty			= (1<<2),

		kWorldViewDirty		= (kWorldDirty | kViewDirty),
		kViewProjDirty		= (kViewDirty | kProjDirty),
		kWorldViewProjDirty	= (kWorldDirty | kViewDirty | kProjDirty),
	};

	Matrix4x4f		worldMatrix;
	Matrix4x4f		projectionMatrixOriginal;	// Originally set from Unity code

	mutable Matrix4x4f		worldViewMatrix; // Lazily updated in UpdateWorldViewMatrix()

	mutable UInt32	dirtyFlags;
};

inline void TransformState::Invalidate(BuiltinShaderParamValues& builtins)
{
	worldViewMatrix.SetIdentity();
	worldMatrix.SetIdentity();
	builtins.GetWritableMatrixParam(kShaderMatView).SetIdentity();
	builtins.GetWritableMatrixParam(kShaderMatProj).SetIdentity();
	builtins.GetWritableMatrixParam(kShaderMatViewProj).SetIdentity();
	projectionMatrixOriginal.SetIdentity();
	dirtyFlags = kWorldViewProjDirty;
}

inline void TransformState::UpdateWorldViewMatrix (const BuiltinShaderParamValues& builtins) const
{
	if (dirtyFlags & kWorldViewDirty)
	{
		MultiplyMatrices4x4 (&builtins.GetMatrixParam(kShaderMatView), &worldMatrix, &worldViewMatrix);
		dirtyFlags &= ~kWorldViewDirty;
	}
}

inline void TransformState::SetViewMatrix (const Matrix4x4f& matrix, BuiltinShaderParamValues& builtins)
{
	dirtyFlags |= TransformState::kWorldViewDirty;
	Matrix4x4f& viewMat = builtins.GetWritableMatrixParam(kShaderMatView);
	Matrix4x4f& invViewMat = builtins.GetWritableMatrixParam(kShaderMatInvView);
	viewMat = matrix;
	InvertMatrix4x4_General3D(matrix.GetPtr(), invViewMat.GetPtr());
	worldMatrix.SetIdentity();
}
