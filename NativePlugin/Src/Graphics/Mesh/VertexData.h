#ifndef VERTEX_DATA_H_
#define VERTEX_DATA_H_

#include "GfxDevice/GfxDeviceTypes.h"

class VertexData
{
public:
	VertexData() { }
	~VertexData();
	void Deallocate();

private:
	UInt8* m_Data;
};



inline int GetPrimitiveCount(int indexCount, GfxPrimitiveType topology, bool nativeQuads)
{
	switch (topology) {
	case kPrimitiveTriangles: return indexCount / 3;
	case kPrimitiveTriangleStrip: return indexCount - 2;
	case kPrimitiveQuads: return nativeQuads ? indexCount / 4 : indexCount / 4 * 2;
	case kPrimitiveLines: return indexCount / 2;
	case kPrimitiveLineStrip: return indexCount - 1;
	case kPrimitivePoints: return indexCount;
	default:  return 0;
	};
}

#endif