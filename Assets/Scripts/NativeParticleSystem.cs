using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;

public class NativeParticleSystem : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    [SerializeField]
    Material m_Material;

    [SerializeField]
    Mesh m_Mesh;

    [DllImport(NativePlugin.PluginName)]
    private static extern void CreateParticleSystem();

    [DllImport(NativePlugin.PluginName)]
    private static extern void SetTextureFromUnity(System.IntPtr texture);

    [DllImport(NativePlugin.PluginName)]
    private static extern void Render();

    private Coroutine m_Coroutine = null;

    // Use this for initialization
    void Awake()
    {
        CreateParticleSystem();
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
            Render();
            GL.IssuePluginEvent(0);
        }
    }
}
