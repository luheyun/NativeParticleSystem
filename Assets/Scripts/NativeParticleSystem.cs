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
    private Coroutine m_Coroutine = null;

    // Use this for initialization
    void Awake()
    {
        StartUp(functionPointer);
        m_Coroutine = StartCoroutine(NativeUpdate());
    }

    void OnDestroy()
    {
        if (m_Coroutine != null)
            StopCoroutine(m_Coroutine);

        m_Coroutine = null;
    }

    // Update is called once per frame
    IEnumerator NativeUpdate()
    {
        while (true)
        {
            yield return new WaitForEndOfFrame();
            GL.IssuePluginEvent(1);
        }
    }
}
