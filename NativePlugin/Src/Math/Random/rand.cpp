#include "PluginPrefix.h"
#include "rand.h"
#include <ctime>

Rand gScriptingRand (std::time(NULL));

Rand& GetScriptingRand()
{
	return gScriptingRand;
}
