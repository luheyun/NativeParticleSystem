#include "PluginPrefix.h"
#include "MemoryUtilities.h"

void memset32 (void *dst, UInt32 value, UInt64 bytecount)
{
	UInt64 i;
	for( i = 0; i < (bytecount & (~3)); i+=4 )
	{
		*((UInt32*)((char*)dst + i)) = value;
	}
	for( ; i < bytecount; i++ )
	{
		((char*)dst)[i] = ((char*)&value)[i&4];
	}
}
