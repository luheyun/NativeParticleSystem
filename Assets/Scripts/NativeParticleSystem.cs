using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;

public class NativeParticleSystem : MonoBehaviour
{
    private const string PluginName = "NativeParticleSystem";

    [DllImport(PluginName)]
    private static extern void LinkDebug([MarshalAs(UnmanagedType.FunctionPtr)]IntPtr debugCal);

    [DllImport(PluginName)]
    private static extern void StartUp();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate void DebugLog(string log);

    private static readonly DebugLog debugLog = DebugWrapper;
    private static readonly IntPtr functionPointer = Marshal.GetFunctionPointerForDelegate(debugLog);

    private static void DebugWrapper(string log) { Debug.Log(log); }

	// Use this for initialization
	void Start ()
    {
        StartUp();
        LinkDebug(functionPointer);
	}
	
	// Update is called once per frame
	void Update () 
    {
        GL.IssuePluginEvent(1);
	}
}
