#include "PluginPrefix.h"
#include "GfxDevice.h"
#include "Utilities/LinkedList.h"
#include "GfxDevice/GfxBuffer.h"
#include "Shaders/GraphicsCaps.h"

static GfxDevice* gfxDevice = nullptr;

class GfxBufferList
{
public:
	List<GfxBuffer> m_List;
};

GfxDevice::GfxDevice()
{
	OnCreate();
}

GfxDevice::~GfxDevice()
{
	OnDelete();
}

void GfxDevice::OnCreate()
{
	m_InvertProjectionMatrix = false;

	for (int i = 0; i < kGfxBufferTargetCount; ++i)
		m_BufferList[i] = new GfxBufferList();
}

void GfxDevice::OnDelete()
{
	for (int i = 0; i < kGfxBufferTargetCount; ++i)
	{
		delete m_BufferList[i];
		m_BufferList[i] = nullptr;
	}
}

void GfxDevice::OnCreateBuffer(GfxBuffer* buffer)
{
	m_BufferList[buffer->GetTarget()]->m_List.push_back(*buffer);
}

void GfxDevice::OnDeleteBuffer(GfxBuffer* buffer)
{
	m_BufferList[buffer->GetTarget()]->m_List.erase(buffer);
}

void GfxDevice::SetWorldMatrix(const Matrix4x4f& matrix)
{
	m_TransformState.worldMatrix = matrix;
	m_TransformState.dirtyFlags |= TransformState::kWorldDirty;
}

void GfxDevice::SetViewMatrix(const Matrix4x4f& matrix)
{
	m_TransformState.SetViewMatrix(matrix, m_BuiltinParamValues);
	GfxDevice::UpdateViewProjectionMatrix();
}

void GfxDevice::SetProjectionMatrix(const Matrix4x4f& matrix)
{
	Matrix4x4f& m = m_BuiltinParamValues.GetWritableMatrixParam(kShaderMatProj);
	m = matrix;
	m_TransformState.projectionMatrixOriginal = matrix;
	GetGfxDevice().CalculateDeviceProjectionMatrix(m, GetGraphicsCaps().usesOpenGLTextureCoords, m_InvertProjectionMatrix);
	m_TransformState.dirtyFlags |= TransformState::kProjDirty;
}

void GfxDevice::UpdateViewProjectionMatrix()
{
	const Matrix4x4f& viewMat = m_BuiltinParamValues.GetMatrixParam(kShaderMatView);
	const Matrix4x4f& projMat = m_BuiltinParamValues.GetMatrixParam(kShaderMatProj);
	Matrix4x4f& viewProjMat = m_BuiltinParamValues.GetWritableMatrixParam(kShaderMatViewProj);
	MultiplyMatrices4x4(&projMat, &viewMat, &viewProjMat);
}

void GfxDevice::CalculateDeviceProjectionMatrix(Matrix4x4f& m, bool usesOpenGLTextureCoords, bool invertY) const
{
	if (usesOpenGLTextureCoords)
		return; // nothing to do on OpenGL-like devices

	// Otherwise, the matrix is OpenGL style, and we have to convert it to
	// D3D-like projection matrix

	if (invertY)
	{
		m.Get(1, 0) = -m.Get(1, 0);
		m.Get(1, 1) = -m.Get(1, 1);
		m.Get(1, 2) = -m.Get(1, 2);
		m.Get(1, 3) = -m.Get(1, 3);
	}


	// Now scale&bias to get Z range from -1..1 to 0..1:
	// matrix = scaleBias * matrix
	//	1   0   0   0
	//	0   1   0   0
	//	0   0 0.5 0.5
	//	0   0   0   1
	m.Get(2, 0) = m.Get(2, 0) * 0.5f + m.Get(3, 0) * 0.5f;
	m.Get(2, 1) = m.Get(2, 1) * 0.5f + m.Get(3, 1) * 0.5f;
	m.Get(2, 2) = m.Get(2, 2) * 0.5f + m.Get(3, 2) * 0.5f;
	m.Get(2, 3) = m.Get(2, 3) * 0.5f + m.Get(3, 3) * 0.5f;
}

DynamicVBO& GfxDevice::GetDynamicVBO()
{
	if (!m_DynamicVBO)
	{
		m_DynamicVBO = CreateDynamicVBO();
	}
	return *m_DynamicVBO;
}

//VertexStreamSource GfxDevice::GetDefaultVertexBuffer(GfxDefaultVertexBufferType type, size_t size)
//{
//	if (!m_DefaultVertexBuffers[type])
//		m_DefaultVertexBuffers[type] = CreateVertexBuffer();
//
//	// Make the size to be at least 1
//	size = std::max((size_t)1, size);
//
//	GfxBuffer* buffer = m_DefaultVertexBuffers[type];
//	const size_t kColorsPerVertex = 2;
//	const size_t kStride = sizeof(UInt32) * kColorsPerVertex;
//	if (buffer->GetBufferSize() < size * kStride)
//	{
//		size_t newSize = NextPowerOfTwo(size);
//		dynamic_array<UInt32> colorArray(newSize * kColorsPerVertex, kMemTempAlloc);
//		UInt32* destPtr = colorArray.data();
//		UInt32 colA = 0xFFFFFFFF;
//		UInt32 colB = 0x00000000;
//		if (type == kGfxDefaultVertexBufferRedBlue)
//		{
//			colA = ConvertToDeviceVertexColor(ColorRGBA32(0, 0, 255, 0)).AsUInt32();
//			colB = ConvertToDeviceVertexColor(ColorRGBA32(255, 0, 0, 0)).AsUInt32();
//		}
//		for (int i = 0; i < newSize; i++)
//		{
//			*destPtr++ = colA;
//			*destPtr++ = colB;
//		}
//		UpdateBuffer(buffer, kGfxBufferModeImmutable, kGfxBufferLabelInternal, newSize * kStride, colorArray.data(), 0);
//	}
//	VertexStreamSource stream = { buffer, kStride };
//	return stream;
//}

void GfxDevice::DeleteDynamicVBO()
{
	delete m_DynamicVBO;
	m_DynamicVBO = NULL;
}

GfxDevice& GetGfxDevice()
{
	return *gfxDevice;
}

void SetGfxDevice(GfxDevice* device)
{
	if (gfxDevice != nullptr)
		delete gfxDevice;

	gfxDevice = device;
}