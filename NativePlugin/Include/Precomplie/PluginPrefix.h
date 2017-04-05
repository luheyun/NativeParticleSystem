#ifndef PLUGIN_PREFIX_H
#define PLUGIN_PREFIX_H

// Platform dependent configuration before we do anything.
// Can't depend on unity defines because they are not there yet!
#ifdef _MSC_VER
	// Visual Studio
	#include "VisualStudioPrefix.h"
#elif defined(ANDROID)
	#include "AndroidPrefix.h"
#elif defined(linux) || defined(__linux__)
	#include "LinuxPrefix.h"
#elif defined(__APPLE__)
	#include "OSXPrefix.h"
#else
	// TODO
#endif

#include "PrefixConfigure.h"

#ifdef __cplusplus
// Pull other frequently used stl headers
#include <vector>
#include <list>
#include <map>
#include <set>
#include <limits.h>
#include <functional>
#include <stdlib.h>
#include <cmath>
#include <algorithm>
#include <string>

#if defined(__GNUC__) && !defined(__ARMCC_VERSION) && !defined(__ghs__)
using std::isfinite;
#endif


#ifdef _MSC_VER
// Visual Studio
//#include "VisualStudioPostPrefix.h"
#define D_SCL_SECURE_NO_WARNINGS
#elif UNITY_IPHONE || UNITY_TVOS
#include "iPhonePostPrefix.h"
#endif

#endif // __cplusplus

#endif
