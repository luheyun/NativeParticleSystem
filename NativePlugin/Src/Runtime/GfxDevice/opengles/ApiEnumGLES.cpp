#include "PluginPrefix.h"

#include "ApiEnumGLES.h"
#include "ApiGLES.h"
#include "ApiTranslateGLES.h"

namespace gl{

bool IsFormatDepthStencil(gl::TexFormat texFormat)
{
	const GLuint formatFlags = gGL->translate.GetFormatDesc(texFormat).flags;
	return (formatFlags & kTextureCapDepth) && (formatFlags & kTextureCapStencil);
}

bool IsFormatFloat(gl::TexFormat texFormat)
{
	return gGL->translate.GetFormatDesc(texFormat).flags & kTextureCapFloat;
}

bool IsFormatHalf(gl::TexFormat texFormat)
{
	return gGL->translate.GetFormatDesc(texFormat).flags & kTextureCapHalf;
}

bool IsFormatIEEE754(gl::TexFormat texFormat)
{
	return gGL->translate.GetFormatDesc(texFormat).flags & (kTextureCapFloat | kTextureCapHalf);
}

bool IsFormatInteger(gl::TexFormat texFormat)
{
	return gGL->translate.GetFormatDesc(texFormat).flags & kTextureCapInteger;
}

}//namespace gl
