#pragma once

#include "BuiltinShaderParamsNames.h"
#include "Allocator/MemoryMacros.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"


struct BuiltinShaderParamIndices
{
	struct MatrixParamData {
		MatrixParamData() : gpuIndex(-1), rows(0), cols(0)
		#if GFX_SUPPORTS_CONSTANT_BUFFERS
			, cbKey(0)
		#endif
		#if GFX_SUPPORTS_OPENGL_UNIFIED
			, isVectorized(false)
		#endif
		{ }
		int     gpuIndex;
		UInt16  rows;
		UInt16  cols;
		#if GFX_SUPPORTS_CONSTANT_BUFFERS
		int		cbKey;
		#endif
		#if GFX_SUPPORTS_OPENGL_UNIFIED
		bool	isVectorized; // If true, is really a vec4 array
		#endif
	};

	MatrixParamData	mat[kShaderInstanceMatCount];

	bool CheckMatrixParam (const char* name, int index, int rowCount, int colCount, int cbKey, bool isVectorized = false);
};


class EXPORT_COREMODULE BuiltinShaderParamValues
{
public:
	BuiltinShaderParamValues ();

	FORCE_INLINE const Vector4f&			GetVectorParam(BuiltinShaderVectorParam param) const	 { return vectorParamValues[param]; }
	FORCE_INLINE const Matrix4x4f&			GetMatrixParam(BuiltinShaderMatrixParam param) const	 { return matrixParamValues[param]; }
	//FORCE_INLINE const ShaderLab::TexEnv&	GetTexEnvParam(BuiltinShaderTexEnvParam param) const	 { return texEnvParamValues[param]; }

	FORCE_INLINE Vector4f&			GetWritableVectorParam(BuiltinShaderVectorParam param)			 { return vectorParamValues[param]; }
	FORCE_INLINE Matrix4x4f&		GetWritableMatrixParam(BuiltinShaderMatrixParam param)			 { return matrixParamValues[param]; }
	//FORCE_INLINE ShaderLab::TexEnv&	GetWritableTexEnvParam(BuiltinShaderTexEnvParam param)			 { return texEnvParamValues[param]; }

	FORCE_INLINE void	SetVectorParam(BuiltinShaderVectorParam param, const Vector4f& val)          { GetWritableVectorParam(param) = val; }
	FORCE_INLINE void	SetMatrixParam(BuiltinShaderMatrixParam param, const Matrix4x4f& val)        { GetWritableMatrixParam(param) = val; }
	//FORCE_INLINE void	SetTexEnvParam(BuiltinShaderTexEnvParam param, const ShaderLab::TexEnv& val) { GetWritableTexEnvParam(param) = val; }

private:
	Vector4f			vectorParamValues[kShaderVecCount];
	Matrix4x4f			matrixParamValues[kShaderMatCount];
	//ShaderLab::TexEnv	texEnvParamValues[kShaderTexEnvCount];
};
