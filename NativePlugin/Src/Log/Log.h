#pragma once

void SetDebugLog(void(_stdcall*debugLog)(char*)); 
void DebugLog(char* str);