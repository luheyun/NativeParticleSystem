#include "PluginPrefix.h"

#include "Runtime/GfxDevice/GfxDeviceConfigure.h"
#include "Runtime/GfxDevice/GfxDevice.h"
#include "Runtime/GfxDevice/ChannelAssigns.h"
#include "Runtime/Graphics/Mesh/MeshVertexFormat.h"
#include "ApiGLES.h"
#include "ApiConstantsGLES.h"
#include "AssertGLES.h"
#include "VertexDeclarationGLES.h"

static int GetGLESDimension(const ChannelInfo& source)
{
	return (source.format == kChannelFormatColor) ? 4 : source.dimension;
}

static void SetVertexComponentData( VertexComponent comp, GLuint bufferName, int size, VertexChannelFormat format, int stride, const void *pointer, unsigned int & attribEnabledMask)
{
	GLuint attribIndex;
	GLboolean normalize;
	switch (comp) {
		case kVertexCompColor:
			attribIndex = kColorArrayGLES;
			normalize = GL_TRUE;
			break;
		case kVertexCompVertex:
			attribIndex = kVertexArrayGLES;
			normalize = GL_FALSE;
			break;
		case kVertexCompNormal:
			//GLES_ASSERT(gGL, size >= 3, "Invalid component count");
			attribIndex = kNormalArrayGLES;
			normalize = GL_FALSE;
			break;
		case kVertexCompTexCoord0:
		case kVertexCompTexCoord1:
		case kVertexCompTexCoord2:
		case kVertexCompTexCoord3:
		case kVertexCompTexCoord4:
		case kVertexCompTexCoord5:
		case kVertexCompTexCoord6:
		case kVertexCompTexCoord7:
			attribIndex = kTexCoord0ArrayGLES + (comp - kVertexCompTexCoord0);
			normalize = GL_FALSE;
			break;
		case kVertexCompTexCoord:
			//printf_console( "Warning: unspecified texcoord bound\n" );
			return;
		case kVertexCompAttrib0: case kVertexCompAttrib1: case kVertexCompAttrib2: case kVertexCompAttrib3:
		case kVertexCompAttrib4: case kVertexCompAttrib5: case kVertexCompAttrib6: case kVertexCompAttrib7:
		case kVertexCompAttrib8: case kVertexCompAttrib9: case kVertexCompAttrib10: case kVertexCompAttrib11:
		case kVertexCompAttrib12: case kVertexCompAttrib13: case kVertexCompAttrib14: case kVertexCompAttrib15:
			attribIndex = comp - kVertexCompAttrib0;
			normalize = GL_TRUE;
			break;
		default:
			return;
	}

	if (attribIndex >= GetGraphicsCaps().gles.maxAttributes)
	{
		//printf_console( "Warning: Exceeds maximum vertex attributes.\n" );
		return;
	}

	//Assert(attribIndex < sizeof(attribIndex) * 8);

	attribEnabledMask |= (1 << attribIndex);

	gl::VertexArrayAttribKind kind = format == kChannelFormatUInt32 ? gl::kVertexArrayAttribInteger : (normalize ? gl::kVertexArrayAttribSNormNormalize : gl::kVertexArrayAttribSNorm);
	gGL->EnableVertexArrayAttrib(attribIndex, bufferName, kind, size, format, stride, pointer);
}

void SetVertexStateGLES(const ChannelAssigns& channels, VertexDeclarationGLES* decl, const VertexStreamSource* vertexStreams, UInt32 firstVertex, size_t vertexStreamCount, size_t vertexCount)
{
	UInt32 targetMap = channels.GetTargetMap();

	unsigned int attribEnabledMask = 0;

	for (int i = 0; i < kVertexCompCount; ++i)
	{
		// A shortcut once we've gone through all channels
		if ((targetMap >> i) == 0)
			break;

		if (!(targetMap & (1 << i)))
			continue;
		VertexComponent comp = (VertexComponent)i;
		ShaderChannel  src = channels.GetSourceForTarget(comp);
		ChannelInfo*   info = &decl->m_Channels[src];
		if (!info->IsValid())
			continue;

		UInt8  offs = info->offset;
		UInt8  stream = info->stream;
		if (stream > vertexStreamCount)
			continue;

		bool needsDummyVB = stream == vertexStreamCount;
		if (needsDummyVB && !vertexCount)
			continue;

		//const VertexStreamSource& vertexStream = needsDummyVB ? GetRealGfxDevice().GetDefaultVertexBuffer(GetDefaultVertexBufferTypeForChannel(src), vertexCount) : vertexStreams[stream];
		const VertexStreamSource& vertexStream = vertexStreams[stream];
		VertexBufferGLES* vb = static_cast<VertexBufferGLES*>(vertexStream.buffer);
		if (vb == NULL)
			continue;

		SetVertexComponentData(comp, vb->GetGLName(), GetGLESDimension(*info), static_cast<VertexChannelFormat>(info->format), vertexStream.stride, vb->GetBindPointer((firstVertex * vertexStream.stride) + offs), attribEnabledMask);
	}

	for (GLuint attribIndex = 1; attribIndex < GetGraphicsCaps().gles.maxAttributes; ++attribIndex)
	{
		if(!(attribEnabledMask & (1 << attribIndex)))
			gGL->DisableVertexArrayAttrib(attribIndex);
	}
}
