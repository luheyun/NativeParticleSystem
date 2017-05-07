#pragma once

#include "ApiGLES.h"
#include "ApiTranslateGLES.h"
#include "Runtime/GfxDevice/GfxDeviceTypes.h"
#include "Runtime/GfxDevice/GfxDeviceResources.h"

struct ALIGN_TYPE(4) DeviceDepthStateGLES : DeviceDepthState
{
	// in parent we have just GfxDepthState = bool + u8
	// on top all gl/gles depth func constants are 16 bit
	// this way we can fit all we need in just one integer
	UInt16		glFunc;

	DeviceDepthStateGLES(GfxDepthState state)
		: DeviceDepthState(state)
		, glFunc(static_cast<UInt16>(gGL->translate.Func(static_cast<CompareFunction>(state.depthFunc))))
	{}
};

const UInt16 kBlendFlagMinMax = 1;
const UInt16 kBlendFlagAdvanced = 1 << 1;

struct DeviceBlendStateGLES : DeviceBlendState
{
	// all gl/gles blend constants are 16 bit
	UInt16	glSrcBlend;
	UInt16	glDstBlend;
	UInt16	glSrcBlendAlpha;
	UInt16	glDstBlendAlpha;
	UInt16	glBlendOp;
	UInt16	glBlendOpAlpha;
	UInt16	blendFlags;

	DeviceBlendStateGLES(const GfxBlendState& state);
};

struct DeviceStencilStateGLES : DeviceStencilState
{
	// all gl/gles stencil constants are 16 bit
	UInt16	glFuncFront;
	UInt16	glPassOpFront;
	UInt16	glFailOpFront;
	UInt16	glZFailOpFront;
	UInt16	glFuncBack;
	UInt16	glPassOpBack;
	UInt16	glFailOpBack;
	UInt16	glZFailOpBack;

	DeviceStencilStateGLES(const GfxStencilState& state);
};

typedef DeviceRasterState DeviceRasterStateGLES;
