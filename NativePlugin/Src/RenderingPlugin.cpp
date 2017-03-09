#include "UnityPluginInterface.h"

static void DoRender();

extern "C" void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
	DoRender();
}

extern "C" void EXPORT_API UnityRenderEvent(int eventID)
{

}

void DoRender()
{

}