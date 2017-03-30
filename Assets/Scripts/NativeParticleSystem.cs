using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using System.Runtime.CompilerServices;

//[StructLayoutAttribute(LayoutKind.Sequential)]
class ParticleInitState
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
}

public class NativeParticleSystem : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    [SerializeField]
    Material m_Material;

    [SerializeField]
    Mesh m_Mesh;

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private extern static void Internal_CreateParticleSystem(ParticleInitState initState);

    [DllImport(NativePlugin.PluginName)]
    private static extern void SetTextureFromUnity(System.IntPtr texture);

    [DllImport(NativePlugin.PluginName)]
    private static extern void Render();

    private Coroutine m_Coroutine = null;
    private ParticleInitState m_InitState = null;

    // Use this for initialization
    void Awake()
    {
        m_InitState = new ParticleInitState();
        m_InitState.looping = true;
        m_InitState.playOnAwake = true;
        m_InitState.useLocalSpace = true;
        m_InitState.speed = 0.2f;
        m_InitState.maxNumParticles = 5;
        Internal_CreateParticleSystem(m_InitState);
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
            Graphics.DrawMeshNow(m_Mesh, this.gameObject.transform.position, this.gameObject.transform.rotation);
            //m_Material.GetMatrix();
            Render();
            GL.IssuePluginEvent(0);
        }
    }
}
