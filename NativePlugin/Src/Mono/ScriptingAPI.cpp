#include "ScriptingAPI.h"

void* GetScriptingObjectFieldValue(ScriptingObject* monoObj, char* fieldName)
{
	ScriptingClass* mclass = script_object_get_class(monoObj);
	ScriptingField *field = script_class_get_field_from_name(mclass, fieldName);
	guint32 offset = script_field_get_offset(field);
	void* value = (void*)((char*)monoObj + offset);
	return value;
}

int mono_array_length_safe(ScriptingArray* array)
{
	if (array)
	{
		char* raw = sizeof(uintptr_t)* 3 + (char*)array;
		return *reinterpret_cast<uintptr_t*> (raw);
	}
	else
	{
		return 0;
	}
}