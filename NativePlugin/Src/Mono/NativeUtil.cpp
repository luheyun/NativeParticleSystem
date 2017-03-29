#include "PluginPrefix.h"
#include "ScriptingAPI.h"
#include "NativeUtil.h"
#include "Log/Log.h"
#include "Input/TimeManager.h"
#include "ParticleSystem/ParticleSystem.h"

extern void DebugLog(char* str);

NativeUtil::NativeUtil()
{
	m_bEnableLog = false;
	m_scriptObj = nullptr;
	m_LogScriptMethod = nullptr;
#if _MSC_VER
	m_strLibMonoPath = "mono";
#endif
}

NativeUtil::~NativeUtil()
{

}

NativeUtil* NativeUtil::Instance()
{
	static NativeUtil s_sa;
	return &s_sa;
}

void NativeUtil::GeneralLog(char* msg)
{
	if (m_bEnableLog)
	{
		if (m_LogScriptMethod == nullptr)
		{
			ScriptingClass* mclass = script_object_get_class(m_scriptObj);
			m_LogScriptMethod = script_class_get_method_from_name(mclass, "GeneralLog", 1);
		}
		ScriptingString *new_name = script_string_new_wrapper(msg);
		void *args[] = { new_name };
		script_runtime_invoke(m_LogScriptMethod, m_scriptObj, args, nullptr);
	}
}

void Internal_CreateNativeUtil_Native(ScriptingObject* self)
{
	NativeUtil::Instance()->SetScriptingObj(self);
	GetLogicObjectMemoryLayout(self)->cachedPtr = NativeUtil::Instance();
}

void Internal_CreateParticleSystem(ScriptingObject* self, ScriptingObject* initState)
{
	DebugLog("Internal_CreateParticleSystem");
    //ParticleSystem::CreateParticleSystrem(initState);
}

void Internal_Update(ScriptingObject* self, float frameTime, float deltaTime)
{
    DebugLog("Internal_Update");
    SetFrameTime(frameTime);
    SetDeltaTime(deltaTime);
}

ScriptingBool NativeUtil_get_EnableLog(ScriptingObject* self)
{
	return NativeUtil::Instance()->EnableLog();
}
void NativeUtil_set_EnableLog(ScriptingObject* self, ScriptingBool value)
{
	NativeUtil::Instance()->SetEnableLog(value);
}
static const char* s_NativeUtil_IcallNames[] =
{
	//"WNEngine.NativeUtil::Internal_CreateNativeUtil",
	//"WNEngine.NativeUtil::get_EnableLog",
	//"WNEngine.NativeUtil::set_EnableLog",
	"NativeParticleSystem::Internal_CreateParticleSystem",
    "NativePlugin::Internal_Update",
	NULL
};

static const void* s_NativeUtil_IcallFuncs[] =
{
	//(const void*)&Internal_CreateNativeUtil_Native,
	//(const void*)&NativeUtil_get_EnableLog,
	//(const void*)&NativeUtil_set_EnableLog,
	(const void*)&Internal_CreateParticleSystem,
    (const void*)&Internal_Update,
	NULL
};
void RegistNativeUtilBindings()
{
	for (int i = 0; s_NativeUtil_IcallNames[i] != NULL; ++i)
	{
		script_add_internal_call(s_NativeUtil_IcallNames[i], s_NativeUtil_IcallFuncs[i]);
	}
}