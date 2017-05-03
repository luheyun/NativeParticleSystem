#include "PluginPrefix.h"
#include "GfxDeviceD3D9.h"
#include "D3D9Utils.h"
#include "D3D9Context.h"
#include "VertexBufferD3D9.h"
#include "IndexBufferD3D9.h"
#include "Utilities/algorithm_utility.h"
#include "Graphics/Mesh/GenericDynamicVBO.h"
#include "GfxDevice/d3d/VertexDeclarationD3D9.h"
#include "Graphics/Mesh/VertexData.h"
#include "Shaders/GraphicsCaps.h"

static const D3DPRIMITIVETYPE kTopologyD3D9[] = // Expected size is kPrimitiveTypeCount
{
	D3DPT_TRIANGLELIST,
	D3DPT_TRIANGLESTRIP,
	D3DPT_TRIANGLELIST,
	D3DPT_LINELIST,
	D3DPT_LINESTRIP,
	D3DPT_POINTLIST,
};

static const D3DBLEND kBlendModeD3D9[] =
{
	D3DBLEND_ZERO,
	D3DBLEND_ONE,
	D3DBLEND_DESTCOLOR,
	D3DBLEND_SRCCOLOR,
	D3DBLEND_INVDESTCOLOR,
	D3DBLEND_SRCALPHA,
	D3DBLEND_INVSRCCOLOR,
	D3DBLEND_DESTALPHA,
	D3DBLEND_INVDESTALPHA,
	D3DBLEND_SRCALPHASAT,
	D3DBLEND_INVSRCALPHA,
};

static const D3DBLENDOP kBlendOpD3D9[] =
{
	D3DBLENDOP_ADD,
	D3DBLENDOP_SUBTRACT,
	D3DBLENDOP_REVSUBTRACT,
	D3DBLENDOP_MIN,
	D3DBLENDOP_MAX,
	// this and later all not supported; ADD used as fallback
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
	D3DBLENDOP_ADD,
};

static const D3DSTENCILOP kStencilOpD3D9[] =
{
	D3DSTENCILOP_KEEP,
	D3DSTENCILOP_ZERO,
	D3DSTENCILOP_REPLACE,
	D3DSTENCILOP_INCRSAT,
	D3DSTENCILOP_DECRSAT,
	D3DSTENCILOP_INVERT,
	D3DSTENCILOP_INCR,
	D3DSTENCILOP_DECR,
};

static const D3DCMPFUNC kCmpFuncD3D9[] =
{
	D3DCMP_ALWAYS,
	D3DCMP_NEVER,
	D3DCMP_LESS,
	D3DCMP_EQUAL,
	D3DCMP_LESSEQUAL,
	D3DCMP_GREATER,
	D3DCMP_NOTEQUAL,
	D3DCMP_GREATEREQUAL,
	D3DCMP_ALWAYS,
};

GfxThreadableDevice* CreateD3D9GfxDevice(bool forceREF)
{
	if (!InitializeD3D(forceREF ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL))
		return NULL;

	GfxDeviceD3D9* device = new GfxDeviceD3D9();

	//ScreenManagerWin& screenMgr = GetScreenManager();
	//HWND window = screenMgr.GetWindow();
	//int width = screenMgr.GetWidth();
	//int height = screenMgr.GetHeight();
	//int dummy;
	//if (!InitializeOrResetD3DDevice(device, window, width, height, width, height, 0, false, 0, 0, dummy, dummy, dummy, dummy))
	//{
	//	UNITY_DELETE(device, kMemGfxDevice);
	//	device = NULL;
	//}

	//PluginsSetGraphicsDevice(GetD3DDevice(), kGfxRendererD3D9, kGfxDeviceEventInitialize);

	return device;
}

GfxDeviceD3D9::GfxDeviceD3D9()
{

}

GfxDeviceD3D9::~GfxDeviceD3D9()
{
	m_VertDeclCache.Clear();
}

GfxBuffer* GfxDeviceD3D9::CreateIndexBuffer()
{
	IndexBufferD3D9* indexBuffer = new IndexBufferD3D9();
	OnCreateBuffer(indexBuffer);
	return indexBuffer;
}

GfxBuffer* GfxDeviceD3D9::CreateVertexBuffer()
{
	VertexBufferD3D9* vertexBuffer = new VertexBufferD3D9();
	OnCreateBuffer(vertexBuffer);
	return vertexBuffer;
}

void GfxDeviceD3D9::UpdateBuffer(GfxBuffer* buffer, GfxBufferMode mode, GfxBufferLabel label, size_t size, const void* data, UInt32 flags)
{
	if (buffer->GetTarget() == kGfxBufferTargetIndex)
		static_cast<IndexBufferD3D9*>(buffer)->UpdateIndexBuffer(mode, label, size, data, flags);
	else
		static_cast<VertexBufferD3D9*>(buffer)->Update(mode, label, size, data);
}

VertexDeclaration* GfxDeviceD3D9::GetVertexDeclaration(const VertexChannelsInfo& declKey)
{
	return m_VertDeclCache.GetVertexDecl(declKey);
}

const DeviceBlendState* GfxDeviceD3D9::CreateBlendState(const GfxBlendState& state)
{
	std::pair<CachedBlendStates::iterator, bool> result = find_or_insert(m_CachedBlendStates, state, DeviceBlendStateD3D9());
	if (!result.second)
		return &result.first->second;

	DeviceBlendStateD3D9& d3dstate = result.first->second;
	memcpy(&d3dstate.sourceState, &state, sizeof(GfxBlendState));
	DWORD d3dmask = 0;
	const UInt8 mask = state.renderTargetWriteMask;
	if (mask & kColorWriteR) d3dmask |= D3DCOLORWRITEENABLE_RED;
	if (mask & kColorWriteG) d3dmask |= D3DCOLORWRITEENABLE_GREEN;
	if (mask & kColorWriteB) d3dmask |= D3DCOLORWRITEENABLE_BLUE;
	if (mask & kColorWriteA) d3dmask |= D3DCOLORWRITEENABLE_ALPHA;
	d3dstate.renderTargetWriteMask = d3dmask;

	return &result.first->second;
}

const DeviceDepthState* GfxDeviceD3D9::CreateDepthState(const GfxDepthState& state)
{
	std::pair<CachedDepthStates::iterator, bool> result = find_or_insert(m_CachedDepthStates, state, DeviceDepthStateD3D9());
	if (!result.second)
		return &result.first->second;

	DeviceDepthStateD3D9& d3dstate = result.first->second;
	memcpy(&d3dstate.sourceState, &state, sizeof(GfxDepthState));
	d3dstate.depthFunc = kCmpFuncD3D9[state.depthFunc];
	return &result.first->second;
}

const DeviceStencilState* GfxDeviceD3D9::CreateStencilState(const GfxStencilState& state)
{
	std::pair<CachedStencilStates::iterator, bool> result = find_or_insert(m_CachedStencilStates, state, DeviceStencilStateD3D9());
	if (!result.second)
		return &result.first->second;

	DeviceStencilStateD3D9& st = result.first->second;
	memcpy(&st.sourceState, &state, sizeof(state));
	st.stencilFuncFront = kCmpFuncD3D9[state.stencilFuncFront];
	st.stencilFailOpFront = kStencilOpD3D9[state.stencilFailOpFront];
	st.depthFailOpFront = kStencilOpD3D9[state.stencilZFailOpFront];
	st.depthPassOpFront = kStencilOpD3D9[state.stencilPassOpFront];
	st.stencilFuncBack = kCmpFuncD3D9[state.stencilFuncBack];
	st.stencilFailOpBack = kStencilOpD3D9[state.stencilFailOpBack];
	st.depthFailOpBack = kStencilOpD3D9[state.stencilZFailOpBack];
	st.depthPassOpBack = kStencilOpD3D9[state.stencilPassOpBack];
	return &result.first->second;
}

const DeviceRasterState* GfxDeviceD3D9::CreateRasterState(const GfxRasterState& state)
{
	std::pair<CachedRasterStates::iterator, bool> result = find_or_insert(m_CachedRasterStates, state, DeviceRasterState());
	if (!result.second)
		return &result.first->second;

	DeviceRasterState& d3dstate = result.first->second;
	memcpy(&d3dstate.sourceState, &state, sizeof(DeviceRasterState));

	return &result.first->second;
}

void GfxDeviceD3D9::SetBlendState(const DeviceBlendState* state)
{
	DeviceBlendStateD3D9* devState = (DeviceBlendStateD3D9*)state;

	const GfxBlendState& desc = devState->sourceState;
	const D3DBLEND d3dsrc = kBlendModeD3D9[desc.srcBlend];
	const D3DBLEND d3ddst = kBlendModeD3D9[desc.dstBlend];
	const D3DBLEND d3dsrca = kBlendModeD3D9[desc.srcBlendAlpha];
	const D3DBLEND d3ddsta = kBlendModeD3D9[desc.dstBlendAlpha];
	const D3DBLENDOP d3dop = kBlendOpD3D9[desc.blendOp];
	const D3DBLENDOP d3dopa = kBlendOpD3D9[desc.blendOpAlpha];

	const bool blendDisabled = (d3dsrc == D3DBLEND_ONE && d3ddst == D3DBLEND_ZERO && d3dsrca == D3DBLEND_ONE && d3ddsta == D3DBLEND_ZERO);

	IDirect3DDevice9* dev = GetD3DDevice();

	if (blendDisabled)
	{
		D3D9_CALL(dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
	}
	else
	{
		D3D9_CALL(dev->SetRenderState(D3DRS_SRCBLEND, d3dsrc));
		D3D9_CALL(dev->SetRenderState(D3DRS_DESTBLEND, d3ddst));
	}
}

void GfxDeviceD3D9::SetRasterState(const DeviceRasterState* state)
{
}

void GfxDeviceD3D9::SetDepthState(const DeviceDepthState* state) 
{
	IDirect3DDevice9* dev = GetD3DDevice();
	DeviceDepthStateD3D9* devState = (DeviceDepthStateD3D9*)state;
	D3D9_CALL(dev->SetRenderState(D3DRS_ZFUNC, devState->depthFunc));
	D3D9_CALL(dev->SetRenderState(D3DRS_ZWRITEENABLE, devState->sourceState.depthWrite ? 1 : 0));
}

void GfxDeviceD3D9::SetStencilState(const DeviceStencilState* state, int stencilRef)
{
}

// Compute/Update any deferred state before each draw call
void GfxDeviceD3D9::BeforeDrawCall()
{
	//ShaderConstantCacheD3D9& vscache = GetVertexShaderConstantCache();
//	ShaderConstantCacheD3D9& pscache = GetPixelShaderConstantCache();
//	DeviceStateD3D& state = m_State;
	//IDirect3DDevice9* dev = GetD3DDevice();

	//m_TransformState.UpdateWorldViewMatrix(m_BuiltinParamValues);

	// update GL equivalents of built-in shader state

//	const BuiltinShaderParamIndices& paramsVS = *m_BuiltinParamIndices[kShaderVertex];
//	const BuiltinShaderParamIndices& paramsPS = *m_BuiltinParamIndices[kShaderFragment];
//	int gpuIndexVS, gpuIndexPS;
//	int rowsVS, rowsPS;
//
//#define SET_BUILTIN_MATRIX_BEGIN(idx) \
//	gpuIndexVS = paramsVS.mat[idx].gpuIndex; rowsVS = paramsVS.mat[idx].rows; gpuIndexPS = paramsPS.mat[idx].gpuIndex; rowsPS = paramsPS.mat[idx].rows; if (gpuIndexVS >= 0 || gpuIndexPS >= 0)
//
//#define SET_BUILTIN_MATRIX_END(name) \
//	if (gpuIndexVS >= 0) vscache.SetValues(gpuIndexVS, name.GetPtr(), rowsVS); \
//	if (gpuIndexPS >= 0) pscache.SetValues(gpuIndexPS, name.GetPtr(), rowsPS)
//
//	// MVP matrix
//	SET_BUILTIN_MATRIX_BEGIN(kShaderInstanceMatMVP)
//	{
//		Matrix4x4f matMul;
//		MultiplyMatrices4x4(&m_BuiltinParamValues.GetMatrixParam(kShaderMatProj), &m_TransformState.worldViewMatrix, &matMul);
//		Matrix4x4f mat;
//		TransposeMatrix4x4(&matMul, &mat);
//		SET_BUILTIN_MATRIX_END(mat);
//	}
//	// MV matrix
//	SET_BUILTIN_MATRIX_BEGIN(kShaderInstanceMatMV)
//	{
//		Matrix4x4f mat;
//		TransposeMatrix4x4(&m_TransformState.worldViewMatrix, &mat);
//		SET_BUILTIN_MATRIX_END(mat);
//	}
//	// Transpose MV matrix
//	SET_BUILTIN_MATRIX_BEGIN(kShaderInstanceMatTransMV)
//	{
//		const Matrix4x4f& mat = m_TransformState.worldViewMatrix;
//		SET_BUILTIN_MATRIX_END(mat);
//	}
//	// Inverse transpose of MV matrix
//	SET_BUILTIN_MATRIX_BEGIN(kShaderInstanceMatInvTransMV)
//	{
//		Matrix4x4f mat;
//		Matrix4x4f::Invert_Full(m_TransformState.worldViewMatrix, mat);
//		SET_BUILTIN_MATRIX_END(mat);
//	}
//	// M matrix
//	SET_BUILTIN_MATRIX_BEGIN(kShaderInstanceMatM)
//	{
//		Matrix4x4f mat;
//		TransposeMatrix4x4(&m_TransformState.worldMatrix, &mat);
//		SET_BUILTIN_MATRIX_END(mat);
//	}
//	// Inverse M matrix
//	SET_BUILTIN_MATRIX_BEGIN(kShaderInstanceMatInvM)
//	{
//		Matrix4x4f inverseMat, transposeMat;
//		Matrix4x4f::Invert_General3D(m_TransformState.worldMatrix, inverseMat);
//		TransposeMatrix4x4(&inverseMat, &transposeMat);
//		SET_BUILTIN_MATRIX_END(transposeMat);
//	}
//
//	vscache.CommitVertexConstants();
//	pscache.CommitPixelConstants();
}

void GfxDeviceD3D9::InvalidateState()
{
	IDirect3DDevice9* dev = GetD3DDevice();
	D3D9_CALL(dev->SetRenderState(D3DRS_LOCALVIEWER, true));
	//D3D9_CALL(dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));
}

void* GfxDeviceD3D9::BeginBufferWrite(GfxBuffer* buffer, size_t offset, size_t size)
{
	if (buffer->GetTarget() == kGfxBufferTargetIndex)
		return static_cast<IndexBufferD3D9*>(buffer)->BeginWriteIndices(offset, size);
	else
		return static_cast<VertexBufferD3D9*>(buffer)->BeginWriteVertices(offset, size);
}

void GfxDeviceD3D9::EndBufferWrite(GfxBuffer* buffer, size_t bytesWritten)
{
	if (buffer->GetTarget() == kGfxBufferTargetIndex)
		static_cast<IndexBufferD3D9*>(buffer)->EndWriteIndices();
	else
		static_cast<VertexBufferD3D9*>(buffer)->EndWriteVertices();
}

void GfxDeviceD3D9::DeleteBuffer(GfxBuffer* buffer)
{
	OnDeleteBuffer(buffer);
	delete buffer;
	buffer = nullptr;
}

void GfxDeviceD3D9::DrawBuffers(
	GfxBuffer* indexBuf, const VertexStreamSource* vertexStreams, int vertexStreamCount,
	const DrawBuffersRange* drawRanges, int drawRangeCount,
	VertexDeclaration* vertexDecl, const ChannelAssigns& channels)
{
	VertexDeclarationD3D9* vertexDeclaration = static_cast<VertexDeclarationD3D9*>(vertexDecl);

	IDirect3DDevice9* dev = GetD3DDevice();
	for (int s = 0; s < vertexStreamCount; s++)
	{
		const VertexStreamSource& vertexStream = vertexStreams[s];
		VertexBufferD3D9* vertexBuffer = static_cast<VertexBufferD3D9*>(vertexStream.buffer);
		if (!vertexBuffer->GetD3DVB())
			return;
		D3D9_CALL(dev->SetStreamSource(s, vertexBuffer->GetD3DVB(), 0, vertexStream.stride));
	}

	IndexBufferD3D9* indexBuffer = static_cast<IndexBufferD3D9*>(indexBuf);
	if (indexBuffer != NULL && indexBuffer->GetD3DIB() == NULL)
		return;

	D3D9_CALL(dev->SetVertexDeclaration(vertexDeclaration->GetD3DDecl()));

	BeforeDrawCall();

	for (int r = 0; r < drawRangeCount; r++)
	{
		const DrawBuffersRange& range = drawRanges[r];

		if (indexBuffer != NULL)
			D3D9_CALL(dev->SetIndices(indexBuffer->GetD3DIB()));

		D3DPRIMITIVETYPE primType = kTopologyD3D9[range.topology];
		UInt32 indexCount = indexBuffer != NULL ? range.indexCount : range.vertexCount;
		UInt32 primCount = GetPrimitiveCount(indexCount, range.topology, false);
		if (indexBuffer != NULL)
			D3D9_CALL(dev->DrawIndexedPrimitive(primType, range.baseVertex, range.firstVertex, range.vertexCount, range.firstIndexByte / 2, primCount));
		else
			D3D9_CALL(dev->DrawPrimitive(primType, range.firstVertex, primCount));
	}
}

DynamicVBO*	GfxDeviceD3D9::CreateDynamicVBO()
{
	const UInt32 kInitialVBSize = 4 * 1024 * 1024;
	const UInt32 kInitialIBSize = 64 * 1024;
	return new GenericDynamicVBO(*this, kGfxBufferModeCircular, kInitialVBSize, kInitialIBSize);
}