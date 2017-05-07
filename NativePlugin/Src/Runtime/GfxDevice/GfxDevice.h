#pragma once

#include "Precomplie/PrefixConfigure.h"
#include "GfxDeviceTypes.h"
#include "GfxDeviceResources.h"
#include "Math/Matrix4x4.h"
#include "BuiltinShaderParams.h"
#include "TransformState.h"

//@TODO: remove me
#define GFX_API virtual
#define GFX_PURE = 0

class GfxBuffer;
class GfxBufferList;
class DynamicVBO;
class VertexDeclaration;
class ChannelAssigns;
struct VertexChannelsInfo;

class EXPORT_COREMODULE GfxDevice
{
public:
	GfxDevice();
	GFX_API ~GfxDevice();

	GFX_API GfxBuffer* CreateIndexBuffer() { return nullptr; }
	GFX_API GfxBuffer* CreateVertexBuffer() { return nullptr; }

	GFX_API void UpdateBuffer(GfxBuffer* buffer, GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data, UInt32 flags) { }

	GFX_API const DeviceBlendState* CreateBlendState(const GfxBlendState& state) GFX_PURE;
	GFX_API const DeviceDepthState* CreateDepthState(const GfxDepthState& state) GFX_PURE;
	GFX_API const DeviceStencilState* CreateStencilState(const GfxStencilState& state) GFX_PURE;
	GFX_API const DeviceRasterState* CreateRasterState(const GfxRasterState& state) GFX_PURE;

	GFX_API void SetBlendState(const DeviceBlendState* state) GFX_PURE;
	GFX_API void SetRasterState(const DeviceRasterState* state) GFX_PURE;
	GFX_API void SetDepthState(const DeviceDepthState* state) GFX_PURE;
	GFX_API void SetStencilState(const DeviceStencilState* state, int stencilRef) GFX_PURE;

	GFX_API void	SetWorldMatrix(const Matrix4x4f& matrix);
	GFX_API void	SetViewMatrix(const Matrix4x4f& matrix);
	GFX_API void	SetProjectionMatrix(const Matrix4x4f& matrix);

	GFX_API	const Matrix4x4f& GetViewMatrix() const;

	// VP matrix (kShaderMatViewProj) is updated when setting view matrix but not when setting projection.
	// Call UpdateViewProjectionMatrix() explicitly if you only change projection matrix.
	GFX_API void	UpdateViewProjectionMatrix();

	// Calculate a device projection matrix given a Unity projection matrix, this needs to be virtual so that platforms can override, e.g. The PSP2 GfxDeviceGXM class needs different behavior.
	GFX_API void CalculateDeviceProjectionMatrix(Matrix4x4f& m, bool usesOpenGLTextureCoords, bool invertY) const;

	DynamicVBO&	GetDynamicVBO();

	GFX_API void* BeginBufferWrite(GfxBuffer* buffer, size_t offset = 0, size_t size = 0) { return NULL; }
	GFX_API void EndBufferWrite(GfxBuffer* buffer, size_t bytesWritten) { }
	GFX_API void DeleteBuffer(GfxBuffer* buffer) {  }

	GFX_API VertexDeclaration* GetVertexDeclaration(const VertexChannelsInfo& declKey) { return NULL; }

	GFX_API void DrawBuffers(GfxBuffer* indexBuf,
		const VertexStreamSource* vertexStreams, int vertexStreamCount,
		const DrawBuffersRange* drawRanges, int drawRangeCount,
		VertexDeclaration* vertexDecl, const ChannelAssigns& channels) {}

	// Any housekeeping around draw calls
	GFX_API void BeforeDrawCall() GFX_PURE;
	GFX_API void AfterDrawCall() {};

	GFX_API void InvalidateState() GFX_PURE;

	//VertexStreamSource GetDefaultVertexBuffer(GfxDefaultVertexBufferType type, size_t size);

protected:
	GFX_API DynamicVBO*	CreateDynamicVBO() GFX_PURE;
	void DeleteDynamicVBO();

	void OnCreate();
	void OnDelete();
	void OnCreateBuffer(GfxBuffer* buffer);
	void OnDeleteBuffer(GfxBuffer* buffer);

	// Mutable state
	BuiltinShaderParamValues	m_BuiltinParamValues;
	TransformState		m_TransformState;
	bool				m_InvertProjectionMatrix;
    // Immutable data
    GfxDeviceRenderer	m_Renderer;

private:
	DynamicVBO*			m_DynamicVBO;
	GfxBufferList*		m_BufferList[kGfxBufferTargetCount];
};

class GfxThreadableDevice : public GfxDevice
{
public:
};

EXPORT_COREMODULE GfxDevice& GetGfxDevice();
EXPORT_COREMODULE void SetGfxDevice(GfxDevice* device);