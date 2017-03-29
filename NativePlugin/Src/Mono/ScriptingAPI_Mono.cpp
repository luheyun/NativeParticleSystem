#include "PluginPrefix.h"
#include "ScriptingAPI.h"
#include "NativeUtil.h"
#ifdef USE_MONO
#if _MSC_VER
#include <windows.h>
#endif

#define DO_API(r,n,p)	typedef r (*fp##n##Type) p;\
	fp##n##Type n = nullptr;

#define FIND_API(n) n = (fp##n##Type)LoadPluginFunction(hModule, #n);


DO_API(void, g_free, (void* p))

DO_API(void, mono_add_internal_call, (const char *name, gconstpointer method))
DO_API(char*, mono_string_to_utf8, (MonoString *string_obj))
DO_API(MonoClass*, mono_object_get_class, (MonoObject *obj))
DO_API(MonoClassField*, mono_class_get_field_from_name, (MonoClass *klass, const char *name))
DO_API(int, mono_field_get_offset, (MonoClassField *field))
DO_API(MonoString*, mono_string_new_wrapper, (const char* text))
DO_API(MonoMethod*, mono_class_get_method_from_name, (MonoClass *klass, const char *name, int param_count))
DO_API(MonoObject*, mono_runtime_invoke, (MonoMethod *method, void *obj, void **params, MonoException **exc))
DO_API(MonoImage*, mono_get_corlib, ())
//DO_API(MonoObject*, mono_object_new, (MonoDomain *domain, MonoClass *klass))
//DO_API(MonoAssembly*, mono_assembly_loaded, (MonoAssemblyName *aname))

void script_add_internal_call(const char *name, gconstpointer method)
{
	mono_add_internal_call(name, method);
}

//ScriptingDomain* script_domain_get();

ScriptingString* script_string_new_wrapper(const char *textUTF8)
{
	ScriptingString* mono = mono_string_new_wrapper(textUTF8);
	if (mono != NULL)
		return mono;
	else
	{
		// This can happen when conversion fails eg. converting utf8 to ascii or something i guess.
		mono = mono_string_new_wrapper("");
		return mono;
	}
}

string script_string_to_utf8(ScriptingString *string_obj)
{
	char* buf = mono_string_to_utf8(string_obj);
	string temp(buf);
	g_free(buf);
	return temp;
}

ScriptingImage* script_get_corlib()
{
	return mono_get_corlib();
}

//ScriptingAssembly* script_assembly_loaded(const char* name);

//ScriptingImage* script_assembly_get_image(const ScriptingAssembly *assembly);

//ScriptingClass* script_class_from_name(ScriptingImage *image, const char* name_space, const char *name);

ScriptingClass* script_object_get_class(ScriptingObject* obj)
{
	return mono_object_get_class(obj);
}

ScriptingMethod* script_class_get_method_from_name(ScriptingClass *klass, const char *name, int param_count)
{
	return mono_class_get_method_from_name(klass, name, param_count);
}

ScriptingField* script_class_get_field_from_name(ScriptingClass *klass, const char *name)
{
	return mono_class_get_field_from_name(klass, name);
}

int script_field_get_offset(ScriptingField *field)
{
	return mono_field_get_offset(field);
}
ScriptingObject* script_runtime_invoke(ScriptingMethod *method, void *obj, void **params, ScriptingObject **exc)
{
	return mono_runtime_invoke(method, obj, params, nullptr);
}

//ScriptingObject* script_object_new(ScriptingClass* klass);

//ScriptingArray* script_array_new(ScriptingClass *eclass, guint32 count);

//guint32 script_gchandle_new(ScriptingObject *obj, gboolean pinned);
//
//ScriptingObject* script_gchandle_get_target(guint32 gchandle);
//
//void script_gchandle_free(guint32 gchandle);
//
//void script_raise_exception(ScriptingException *ex);
//
//gpointer script_object_unbox(ScriptingObject* o);

static void* LoadPluginExecutable(const char* pluginPath)
{
#if _MSC_VER
	HMODULE hMono = ::GetModuleHandleA(pluginPath);
	if (hMono == NULL)
	{
		hMono = LoadLibraryA(pluginPath);
	}
	return hMono;

#else
	return dlopen(pluginPath, RTLD_NOW);
#endif
}

static void* LoadPluginFunction(void* pluginHandle, const char* name)
{
#if _MSC_VER
	return GetProcAddress((HMODULE)pluginHandle, name);

#else
	return dlsym(pluginHandle, name);
#endif
}
static void UnloadPluginExecutable(void* pluginHandle)
{
#if _MSC_VER
	FreeLibrary((HMODULE)pluginHandle);
#else
	dlclose(pluginHandle);
#endif
}

void InitMonoSystem()
{
	void* hModule = nullptr;
	hModule = LoadPluginExecutable(NativeUtil::Instance()->GetLibMonoPath());

	if (hModule != nullptr)
	{
		FIND_API(g_free)
			FIND_API(mono_add_internal_call)
			FIND_API(mono_string_to_utf8)
			FIND_API(mono_object_get_class)
			FIND_API(mono_class_get_field_from_name)
			FIND_API(mono_field_get_offset)
			FIND_API(mono_string_new_wrapper)
			FIND_API(mono_class_get_method_from_name)
			FIND_API(mono_runtime_invoke)
			FIND_API(mono_get_corlib)

			if (mono_add_internal_call != NULL)
			{
				RegistNativeUtilBindings();
			}
	}
}

#if defined(USE_MONO) && !_MSC_VER
void SetLibMonoPath(JNIEnv * env, jclass clazz, jstring nativeLibraryDir)
{
	jsize str_len = env->GetStringUTFLength(nativeLibraryDir) + 1;
	char* libdir = (char*)malloc(str_len);
	const char* dir = env->GetStringUTFChars(nativeLibraryDir, 0);
	memcpy(libdir, dir, str_len);
	env->ReleaseStringUTFChars(nativeLibraryDir, dir);
	//	LOGD("nativeLibraryDir '%s' (%i)", libdir, str_len);


	char libMonoPath[2048] = { 0 };
	snprintf(libMonoPath, sizeof(libMonoPath)-1, "%s/%s", libdir, "libmono.so");
	NativeUtil::Instance()->SetLibMonoPath(libMonoPath);
	free(libdir);
}
#endif
#endif