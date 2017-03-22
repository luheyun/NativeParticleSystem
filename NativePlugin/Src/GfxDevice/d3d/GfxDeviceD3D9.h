#pragma once

#include "GfxDevice/GfxDevice.h"
#include <map>
#include "D3D9Includes.h"
#include "VertexDeclarationD3D9.h"

class ChannelAssigns;

struct DeviceStateD3D
{
	CompareFunction	depthFunc;
	int				depthWrite; // 0/1 or -1

	int				blending;
	int				alphaToMask;
	int				srcBlend, destBlend, srcBlendAlpha, destBlendAlpha; // D3D modes
	int				blendOp, blendOpAlpha; // D3D modes

	CullMode		culling;
	D3DCULL			d3dculling;
	bool			appBackfaceMode;
	bool			wireframe;
	int				scissor;

	bool			requestedSRGBWrite;
	int				actualSRGBWrite; // 0/1 or -1
	int				renderTargetsAreLinear; // 0/1 or -1

	// [0] is front, [1] is back, unless invertProjMatrix is true
	D3DCMPFUNC		stencilFunc[2];
	D3DSTENCILOP	stencilFailOp[2], depthFailOp[2], depthPassOp[2];

	float offsetFactor, offsetUnits;

	int				colorWriteMask; // ColorWriteMask combinations

	int		m_StencilRef;

	bool		m_DeviceLost;
};

class EXPORT_COREMODULE GfxDeviceD3D9 : public GfxThreadableDevice
{
public:
	struct DeviceBlendStateD3D9 : public DeviceBlendState
	{
		UInt8		renderTargetWriteMask;
	};

	struct DeviceDepthStateD3D9 : public DeviceDepthState
	{
		D3DCMPFUNC depthFunc;
	};

	struct DeviceStencilStateD3D9 : public DeviceStencilState
	{
		D3DCMPFUNC		stencilFuncFront;
		D3DSTENCILOP	stencilFailOpFront;
		D3DSTENCILOP	depthFailOpFront;
		D3DSTENCILOP	depthPassOpFront;
		D3DCMPFUNC		stencilFuncBack;
		D3DSTENCILOP	stencilFailOpBack;
		D3DSTENCILOP	depthFailOpBack;
		D3DSTENCILOP	depthPassOpBack;
	};


	typedef std::map< GfxBlendState, DeviceBlendStateD3D9, memcmp_less<GfxBlendState> > CachedBlendStates;
	typedef std::map< GfxDepthState, DeviceDepthStateD3D9, memcmp_less<GfxDepthState> > CachedDepthStates;
	typedef std::map< GfxStencilState, DeviceStencilStateD3D9, memcmp_less<GfxStencilState> > CachedStencilStates;
	typedef std::map< GfxRasterState, DeviceRasterState, memcmp_less<GfxRasterState> > CachedRasterStates;

public:
	GfxDeviceD3D9();
	GFX_API ~GfxDeviceD3D9();

	GFX_API GfxBuffer* CreateIndexBuffer();
	GFX_API GfxBuffer* CreateVertexBuffer();
	GFX_API void UpdateBuffer(GfxBuffer* buffer, GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data, UInt32 flags);

	GFX_API const DeviceBlendState* CreateBlendState(const GfxBlendState& state);
	GFX_API const DeviceDepthState* CreateDepthState(const GfxDepthState& state);
	GFX_API const DeviceStencilState* CreateStencilState(const GfxStencilState& state);
	GFX_API const DeviceRasterState* CreateRasterState(const GfxRasterState& state);

	GFX_API void SetBlendState(const DeviceBlendState* state);
	GFX_API void SetRasterState(const DeviceRasterState* state);
	GFX_API void SetDepthState(const DeviceDepthState* state);
	GFX_API void SetStencilState(const DeviceStencilState* state, int stencilRef);

	GFX_API void BeforeDrawCall();

	GFX_API void* BeginBufferWrite(GfxBuffer* buffer, size_t offset = 0, size_t size = 0);
	GFX_API void EndBufferWrite(GfxBuffer* buffer, size_t bytesWritten);
	GFX_API void DeleteBuffer(GfxBuffer* buffer);

	GFX_API VertexDeclaration* GetVertexDeclaration(const VertexChannelsInfo& declKey);

	GFX_API void DrawBuffers(GfxBuffer* indexBuf,
		const VertexStreamSource* vertexStreams, int vertexStreamCount,
		const DrawBuffersRange* drawRanges, int drawRangeCount,
		VertexDeclaration* vertexDecl, const ChannelAssigns& channels);

protected:
	GFX_API DynamicVBO*	CreateDynamicVBO();

private:
	//DeviceStateD3D		m_State;
	DeviceBlendStateD3D9*	m_CurrBlendState;
	//DeviceDepthStateD3D9*	m_CurrDepthState;
	const DeviceStencilStateD3D9*	m_CurrStencilState;
	DeviceRasterState*		m_CurrRasterState;
	VertexDeclarationCacheD3D9 m_VertDeclCache;

	CachedBlendStates	m_CachedBlendStates;
	CachedDepthStates	m_CachedDepthStates;
	CachedStencilStates	m_CachedStencilStates;
	CachedRasterStates	m_CachedRasterStates;
};