#include "PluginPrefix.h"
#include "GraphicsCaps.h"

static GraphicsCaps gGraphicsCaps;
GraphicsCaps& GetGraphicsCaps()
{
	return gGraphicsCaps;
}

GraphicsCaps::GraphicsCaps()
{
	::memset(this, 0x00, sizeof(GraphicsCaps));
	usesOpenGLTextureCoords = false;
}