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
public class ParticleRenderData
{

}

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class ShapeModuleData
{
    public int type;
    public float radius;
    public float angle;
    public float length;
    public float boxX;
    public float boxY;
    public float boxZ;
    public bool randomDirection;
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
    public Curve initModuleLiftTime;
    public Curve initModuleSpeed;
    public Curve initModuleSize;
    public Curve initModuleRotation;
    public bool rotationModuleEnable;
    public Curve rotationModuleCurve = new Curve();
    public float emissionRate;
    public bool sizeModuleEnable;
    public Curve sizeModuleCurve = new Curve();
    public bool shapeModuleEnable;
    public ShapeModuleData shapeModuleData;
    public bool colorModuleEnable;
    public MinMaxGradient colorModuleGradient;
}

[StructLayoutAttribute(LayoutKind.Sequential)]
public class ParticleSystremUpdateData
{
    public Matrix4x4 worldMatrix;
    public int index;
}

public class NativeParticleSystem : MonoBehaviour
{
    public enum ERenderType : byte
    {
        none = 0,
        first = 1,
        normal = 1 << 2,
        last = 1 << 3 
    }

    public const string PluginName = "NativeParticleSystem";

    [SerializeField]
    Material m_Material;

    [SerializeField]
    Mesh m_Mesh;

    public bool EnableCull = false;

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private extern static int Internal_CreateParticleSystem(ParticleInitState initState);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private extern static void Internal_ParticleSystem_Update(ParticleSystremUpdateData updateData);

    [DllImport(NativePlugin.PluginName)]
    private static extern void SetTextureFromUnity(System.IntPtr texture);

    [DllImport(NativePlugin.PluginName)]
    private static extern void Native_Render(int index, byte renderType);

    [DllImport(NativePlugin.PluginName)]
    private static extern void Native_SetActive(int index, bool isActive);

    private Coroutine m_Coroutine = null;
    public ParticleInitState InitState = new ParticleInitState();

    public ParticleSystremUpdateData m_UpdateData = new ParticleSystremUpdateData();

    private Vector3 m_DefaultMeshPos = new Vector3(-1000f, -1000f, -1000f);

    public ParticleSystem particleSystem;

    // Use this for initialization
    void Awake()
    {
        m_UpdateData.index = Internal_CreateParticleSystem(InitState);
        Debug.Log("index:"+ m_UpdateData.index.ToString());
        m_Coroutine = StartCoroutine(NativeUpdate());
    }

    //void Start()
    //{
    //    //SetTextureFromUnity(m_Material.GetTexture().GetNativeTexturePtr());
    //}

    private void OnEnable()
    {
        Native_SetActive(m_UpdateData.index, true);
    }

    private void OnDisable()
    {
        Native_SetActive(m_UpdateData.index, false);
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
            //m_Material.SetPass(0);

            //Graphics.DrawMeshNow(m_Mesh, m_DefaultMeshPos, this.gameObject.transform.rotation);
            ////m_Material.GetMatrix();
            //m_UpdateData.worldMatrix = transform.localToWorldMatrix;
            //Internal_ParticleSystem_Update(m_UpdateData);
            //Render();
            //GL.IssuePluginEvent(0);
        }
    }

    public void Render(ERenderType renderType)
    {
        m_Material.SetPass(0);

        Graphics.DrawMeshNow(m_Mesh, m_DefaultMeshPos, this.gameObject.transform.rotation);
        m_UpdateData.worldMatrix = transform.localToWorldMatrix;
        Internal_ParticleSystem_Update(m_UpdateData);
        Native_Render(m_UpdateData.index, (byte)renderType);
    }
}
