#pragma once

class GfxDevice;

GfxDevice* ReserveClientDevice();
void ReleaseClientDevice(GfxDevice* device);
void ReleaseThreadedDevices(GfxDevice* device);
