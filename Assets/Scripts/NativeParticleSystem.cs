using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using System.Runtime.CompilerServices;
using System.Collections.Generic;

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class KeyFrame
{
    public float time;
    public float value;
    public float inSlope;
    public float outSlope;
    //public int tangentMode;
}

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class Curve
{
    public int minMaxState;
    public float scalar;
    public AnimationCurve minCurve = new AnimationCurve();
    public AnimationCurve maxCurve = new AnimationCurve();
}

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class AnimationCurve
{
    public int keyFrameCount;
    public KeyFrame[] keyFrameContainer;
    public int preInfinity;
    public int postInfinity;
}

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class ParticleInitState
{
    public bool looping;
    public bool prewarm;
    public int randomSeed;
    public bool playOnAwake;
    public float startDelay;
    public float speed;
    public float lengthInSec;
    public bool useLocalSpace;
    public int maxNumParticles;
    public bool rotationModuleEnable;
    public Curve rotationModuleCurve = new Curve();
    public float rotationMin;
    public float rotationMax;
    public float emissionRate;
    public bool sizeModuleEnable;
    public Curve sizeModuleCurve = new Curve();
}

[StructLayoutAttribute(LayoutKind.Sequential)]
public class ParticleSystremUpdateData
{
    public Matrix4x4 worldMatrix;
    public int index;
}

public class NativeParticleSystem : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    [SerializeField]
    Material m_Material;

    [SerializeField]
    Mesh m_Mesh;

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private extern static int Internal_CreateParticleSystem(ParticleInitState initState);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private extern static void Internal_ParticleSystem_Update(ParticleSystremUpdateData updateData);

    [DllImport(NativePlugin.PluginName)]
    private static extern void SetTextureFromUnity(System.IntPtr texture);

    [DllImport(NativePlugin.PluginName)]
    private static extern void Render();

    private Coroutine m_Coroutine = null;
    public ParticleInitState InitState = new ParticleInitState();

    public ParticleSystremUpdateData m_UpdateData = new ParticleSystremUpdateData();

    private Vector3 m_DefaultMeshPos = new Vector3(-1000f, -1000f, -1000f);

    public ParticleSystem particleSystem;

    // Use this for initialization
    void Awake()
    {
        InitState.useLocalSpace = true;
        InitState.speed = 0.2f;
        m_UpdateData.index = Internal_CreateParticleSystem(InitState);
        Debug.Log("index:"+ m_UpdateData.index.ToString());
        m_Coroutine = StartCoroutine(NativeUpdate());
    }

    void Start()
    {
        //SetTextureFromUnity(m_Material.GetTexture().GetNativeTexturePtr());
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
            //GL.IssuePluginEvent(1);
            m_Material.SetPass(0);

            Graphics.DrawMeshNow(m_Mesh, m_DefaultMeshPos, this.gameObject.transform.rotation);
            //m_Material.GetMatrix();
            m_UpdateData.worldMatrix = transform.localToWorldMatrix;
            Internal_ParticleSystem_Update(m_UpdateData);
            Render();
            GL.IssuePluginEvent(0);
        }
    }
}
