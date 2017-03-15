#pragma once

#include "GfxDeviceTypes.h"
#include "Allocator/MemoryMacros.h"

template<typename T, typename CmpT=T>
struct memcmp_less
{
	bool operator () (const T& lhs, const T& rhs) const
	{
		return memcmp(&lhs, &rhs, sizeof(CmpT)) < 0;
	}
};


struct GfxBlendState
{
	UInt32	renderTargetWriteMask;

	UInt8	srcBlend;		// BlendMode
	UInt8	dstBlend;		// BlendMode
	UInt8	srcBlendAlpha;	// BlendMode
	UInt8	dstBlendAlpha;	// BlendMode
	UInt8	blendOp;		// BlendOp
	UInt8	blendOpAlpha;	// BlendOp
	bool	alphaToMask;
	UInt8	__pad;			// explicit pad, zero it out, avoid random bytes

	GfxBlendState()
	:	renderTargetWriteMask(kColorWriteAll),
		srcBlend(kBlendOne), dstBlend(kBlendZero), srcBlendAlpha(kBlendOne), dstBlendAlpha(kBlendZero),
		blendOp(kBlendOpAdd), blendOpAlpha(kBlendOpAdd),
		alphaToMask(false), __pad(0)
	{
	}
};

struct GfxRasterState
{
	CullMode	cullMode;
	int			depthBias;
	float		slopeScaledDepthBias;

	GfxRasterState()
		: cullMode(kCullBack)
		, depthBias(0)
		, slopeScaledDepthBias(0.0f)
	{}
};


// we do not align this struct to be able to use 16bit padding in derived platform-specific struct
struct GfxDepthState
{
	bool	depthWrite;
	SInt8	depthFunc;	// CompareFunction

	GfxDepthState() : depthWrite(true), depthFunc(kFuncLess) { }
	GfxDepthState(bool write, CompareFunction func) : depthWrite(write), depthFunc(func) { }
};


struct ALIGN_TYPE(4) GfxStencilState
{
	bool	stencilEnable;
	UInt8	readMask;
	UInt8	writeMask;
	UInt8	__pad;
	UInt8	stencilFuncFront;		// CompareFunction
	UInt8	stencilPassOpFront;		// StencilOp: stencil and depth pass
	UInt8	stencilFailOpFront;		// StencilOp: stencil fail (depth irrelevant)
	UInt8	stencilZFailOpFront;	// StencilOp: stencil pass, depth fail
	UInt8	stencilFuncBack;		// CompareFunction
	UInt8	stencilPassOpBack;		// StencilOp: stencil and depth pass
	UInt8	stencilFailOpBack;		// StencilOp: stencil fail (depth irrelevant)
	UInt8	stencilZFailOpBack;		// StencilOp: stencil pass, depth fail

	GfxStencilState()
	:	stencilEnable(false), readMask(0xFF), writeMask(0xFF), __pad(0),
		stencilFuncFront(kFuncAlways), stencilPassOpFront(kStencilOpKeep), stencilFailOpFront(kStencilOpKeep), stencilZFailOpFront(kStencilOpKeep),
		stencilFuncBack(kFuncAlways), stencilPassOpBack(kStencilOpKeep), stencilFailOpBack(kStencilOpKeep), stencilZFailOpBack(kStencilOpKeep)
	{
	}
};

struct DeviceBlendState
{
	DeviceBlendState(const GfxBlendState& src) : sourceState(src) {}
	DeviceBlendState() {}
	GfxBlendState sourceState;
};

struct DeviceDepthState
{
	DeviceDepthState(GfxDepthState src) : sourceState(src) {}
	DeviceDepthState() {}
	GfxDepthState sourceState;
};

struct DeviceStencilState
{
	DeviceStencilState(const GfxStencilState& src) : sourceState(src) {}
	DeviceStencilState() {}
	GfxStencilState sourceState;
};

struct DeviceRasterState
{
	DeviceRasterState(const GfxRasterState& src) : sourceState(src) {}
	DeviceRasterState() {}
	GfxRasterState sourceState;
};
