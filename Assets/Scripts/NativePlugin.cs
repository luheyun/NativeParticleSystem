using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;

public class NativePlugin : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    [DllImport(PluginName)]
    private static extern void StartUp([MarshalAs(UnmanagedType.FunctionPtr)]IntPtr debugCal);

    [DllImport(PluginName)]
    private static extern void ShutDown();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate void DebugLog(string log);

    private static readonly DebugLog debugLog = DebugWrapper;
    private static readonly IntPtr functionPointer = Marshal.GetFunctionPointerForDelegate(debugLog);

    private static void DebugWrapper(string log) { Debug.Log(log); }

	void Awake ()
    {
        StartUp(functionPointer);	
	}

    void OnDestroy()
    {
        ShutDown();
    }
}
