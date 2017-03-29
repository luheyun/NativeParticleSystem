using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using System.Runtime.CompilerServices;

class ParticleInitState
{
    public bool playOnAwake;
    public bool looping;
    public float startDelay;
}

public class NativeParticleSystem : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    [SerializeField]
    Material m_Material;

    [SerializeField]
    Mesh m_Mesh;

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    private extern void Internal_CreateParticleSystem(ParticleInitState initState);

    [DllImport(NativePlugin.PluginName)]
    private static extern void SetTextureFromUnity(System.IntPtr texture);

    [DllImport(NativePlugin.PluginName)]
    private static extern void Render();

    private Coroutine m_Coroutine = null;

    // Use this for initialization
    void Awake()
    {
        ParticleInitState initState = new ParticleInitState();
        Internal_CreateParticleSystem(initState);
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
