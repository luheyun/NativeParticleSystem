#ifndef PREFIX_CONFIGURE_H
#define PREFIX_CONFIGURE_H

// ADD_NEW_PLATFORM_HERE: review this file

typedef signed short SInt16;
typedef unsigned short UInt16;
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef signed int SInt32;
typedef unsigned int UInt32;

typedef unsigned long long UInt64;
typedef signed long long SInt64;

#if UNITY_64
typedef UInt64 UIntPtr;
#else
typedef UInt32 UIntPtr;
#endif

#define BUILDING_COREMODULE

#ifdef BUILDING_COREMODULE
#pragma warning(disable:4275)
#pragma warning(disable:4251)
#pragma warning(disable:4996)
#define EXPORT_COREMODULE __declspec(dllexport)
#else
#define EXPORT_COREMODULE __declspec(dllimport)
#endif

#endif
