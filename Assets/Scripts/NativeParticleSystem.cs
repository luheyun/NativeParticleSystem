using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;

public class NativeParticleSystem : MonoBehaviour
{
    private const string PluginName = "NativeParticleSystem";

    [DllImport(PluginName)]
    private static extern void StartUp([MarshalAs(UnmanagedType.FunctionPtr)]IntPtr debugCal);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate void DebugLog(string log);

    private static readonly DebugLog debugLog = DebugWrapper;
    private static readonly IntPtr functionPointer = Marshal.GetFunctionPointerForDelegate(debugLog);

    private static void DebugWrapper(string log) { Debug.Log(log); }

	// Use this for initialization
	void Awake ()
    {
        StartUp(functionPointer);
	}

    int count = 0;

	// Update is called once per frame
	void Update () 
    {
        if (count <= 0)
            GL.IssuePluginEvent(1);

        count = 1;
	}
}
