#pragma once

#include "ScriptingTypes.h"
#include <string>
using namespace std;

void script_add_internal_call(const char *name, gconstpointer method);

//ScriptingDomain* script_domain_get();

ScriptingString* script_string_new_wrapper(const char *text);

string script_string_to_utf8(ScriptingString *string_obj);

ScriptingImage* script_get_corlib();

//ScriptingAssembly* script_assembly_loaded(const char* name);

//ScriptingImage* script_assembly_get_image(const ScriptingAssembly *assembly);

//ScriptingClass* script_class_from_name(ScriptingImage *image, const char* name_space, const char *name);

ScriptingClass* script_object_get_class(ScriptingObject* obj);

ScriptingMethod* script_class_get_method_from_name(ScriptingClass *klass, const char *name, int param_count);

ScriptingField* script_class_get_field_from_name(ScriptingClass *klass, const char *name);

int script_field_get_offset(ScriptingField *field);

ScriptingObject* script_runtime_invoke(ScriptingMethod *method, void *obj, void **params, ScriptingObject **exc);

//ScriptingObject* script_object_new(ScriptingClass* klass);

//ScriptingArray* script_array_new(ScriptingClass *eclass, guint32 count);

//guint32 script_gchandle_new(ScriptingObject *obj, gboolean pinned);

//ScriptingObject* script_gchandle_get_target(guint32 gchandle);

//void script_gchandle_free(guint32 gchandle);

//void script_raise_exception(ScriptingException *ex);

//gpointer script_object_unbox(ScriptingObject* o);

#ifdef _WIN32
#define MYLIBAPI  __declspec(dllexport)
#else
#define MYLIBAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

	void MYLIBAPI InitMonoSystem();

#ifdef __cplusplus
}
#endif

#if _MSC_VER
#define snprintf _snprintf
#endif

void* GetScriptingObjectFieldValue(ScriptingObject* monoObj, char* fieldName);

inline void* scripting_array_element_ptr(ScriptingArrayPtr array, int i, size_t element_size)
{
	return kMonoArrayOffset + i * element_size + (char*)array;
}
template<class T>
inline T* GetScriptingArrayStart(ScriptingArrayPtr array)
{
	return (T*)scripting_array_element_ptr(array, 0, sizeof(T));
}

int mono_array_length_safe(ScriptingArray* array);


inline int GetScriptingArraySize(ScriptingArrayPtr a)
{
	return mono_array_length_safe(a);
}

inline LogicObjectMemoryLayout* GetLogicObjectMemoryLayout(ScriptingObjectPtr object)
{
	return reinterpret_cast<LogicObjectMemoryLayout*>(((char*)object) + kMonoObjectOffset);
}
inline void* GetLogicObjectCachedPtrFromScriptingWrapper(ScriptingObjectPtr object)
{
	return GetLogicObjectMemoryLayout(object)->cachedPtr;
}

void RegistNativeUtilBindings();
void RigistEquipItemDataCSVBindgs();
