#include "GfxDevice.h"

static GfxDevice* gfxDevice = nullptr;

GfxDevice::GfxDevice()
{

}

GfxDevice::~GfxDevice()
{

}

GfxDevice& GetGfxDevice()
{
	return gfxDevice;
}

void SetGfxDevice(GfxDevice* device)
{
	gfxDevice = device;
}