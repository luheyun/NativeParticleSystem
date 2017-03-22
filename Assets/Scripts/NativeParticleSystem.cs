using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;

public class NativeParticleSystem : MonoBehaviour
{
    public const string PluginName = "NativeParticleSystem";

    [SerializeField]
    Material m_Material;

    [DllImport(NativePlugin.PluginName)]
    private static extern void CreateParticleSystem();

    private Coroutine m_Coroutine = null;

    // Use this for initialization
    void Awake()
    {
        CreateParticleSystem();
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
