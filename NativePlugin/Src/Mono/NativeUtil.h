//////////////////////////////////////////////////////////////
//     FileName:    NativeUtil.h
//     Author:      taoxu
//     Date:        2017-2-14 
//     Description: native代码与C#通讯的工具
/////////////////////////////////////////////////////////////
#pragma once

#include "Mono/ScriptingTypes.h"

class NativeUtil
{
private:
	ScriptingObject* m_scriptObj;
	ScriptingMethod* m_LogScriptMethod;
	bool m_bEnableLog;
	std::string m_strLibMonoPath;

public:
	static NativeUtil* Instance();
	NativeUtil();
	~NativeUtil();
	void GeneralLog(char* msg);
	void SetScriptingObj(ScriptingObject* obj) { m_scriptObj = obj; }
	bool EnableLog() { return m_bEnableLog; }
	void SetEnableLog(bool set) { m_bEnableLog = set; }
	void SetLibMonoPath(char* strPath) { m_strLibMonoPath = strPath; }
	const char* GetLibMonoPath() { return m_strLibMonoPath.c_str(); }
};

