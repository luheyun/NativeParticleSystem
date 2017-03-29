using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
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

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private static extern void Internal_Update(float frameTime, float deltaTime);

    private static readonly DebugLog debugLog = DebugWrapper;
    private static readonly IntPtr functionPointer = Marshal.GetFunctionPointerForDelegate(debugLog);

    private static void DebugWrapper(string log) { Debug.Log(log); }

	void Awake ()
    {
        StartUp(functionPointer);	
	}

    void Update()
    {
        Internal_Update(Time.time, Time.deltaTime);
    }

    void OnDestroy()
    {
        ShutDown();
    }
}
