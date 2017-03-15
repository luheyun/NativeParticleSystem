#include "PluginPrefix.h"
#include "BuiltinShaderParams.h"
#include "GfxDeviceTypes.h"

BuiltinShaderParamValues::BuiltinShaderParamValues ()
{
	memset (vectorParamValues, 0, sizeof(vectorParamValues));
	memset (matrixParamValues, 0, sizeof(matrixParamValues));

	// Initialize default light directions to (1,0,0,0), to avoid the case
	// when a shader with uninitialized value gets "tolight" vector of zero,
	// which returns NaN when doing normalize() on it, on GeForce FX/6/7.
	for (int i = 0; i < kMaxSupportedVertexLights; ++i)
		vectorParamValues[kShaderVecLight0Position+i].x = 1.0f;
}
