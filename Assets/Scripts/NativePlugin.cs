using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using System;

[StructLayoutAttribute(LayoutKind.Sequential)]
class NativeUpdateData 
{
    public float frameTime;
    public float deltaTime;
    public Matrix4x4 viewMatrix; 
}

public class NativePlugin : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    public Camera MainCamera;

    [DllImport(PluginName)]
    private static extern void StartUp([MarshalAs(UnmanagedType.FunctionPtr)]IntPtr debugCal);

    [DllImport(PluginName)]
    private static extern void ShutDown();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate void DebugLog(string log);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private static extern void Internal_Update(NativeUpdateData updateData);

    private static readonly DebugLog debugLog = DebugWrapper;
    private static readonly IntPtr functionPointer = Marshal.GetFunctionPointerForDelegate(debugLog);

    private static void DebugWrapper(string log) { Debug.Log(log); }

    private NativeUpdateData m_NativeUpdateData = new NativeUpdateData();

    // todo 
    public List<NativeParticleSystem> m_ParticleSystems = new List<NativeParticleSystem>();

	void Awake ()
    {
        StartUp(functionPointer);	
	}

    void Update()
    {
        float frameTime = Time.time;
        float deltaTime = Time.deltaTime;
        Debug.Log("Plugin Update, frameTime:" + Time.time.ToString() + ", deltaTime:" + deltaTime.ToString());
        m_NativeUpdateData.viewMatrix = MainCamera.worldToCameraMatrix;
        m_NativeUpdateData.frameTime = Time.time;
        m_NativeUpdateData.deltaTime = Time.deltaTime;
        Internal_Update(m_NativeUpdateData);
    }

    void OnPostRender()
    {
        for (int i = 0; i < m_ParticleSystems.Count; ++i)
            m_ParticleSystems[i].Render();
    }

    void OnDestroy()
    {
        ShutDown();
    }
}
