using UnityEngine;
using UnityEditor;
using System.Collections;

[CustomEditor(typeof(NativeParticleSystem))]
public class NativeParticleSystremEditor : Editor
{
    public override void OnInspectorGUI()
    {
        base.OnInspectorGUI();

        NativeParticleSystem ps = target as NativeParticleSystem;
        SerializedObject so = new SerializedObject(ps.particleSystem);

        if (GUILayout.Button("Log Particle System Property"))
        {
            SerializedProperty it = so.GetIterator();
            while (it.Next(true))
                Debug.Log(it.propertyPath + " " + it.propertyType.ToString());
        }

        if (GUILayout.Button("Save Particle System Property"))
        {
            ps.InitState.looping = so.FindProperty("looping").boolValue;
            ps.InitState.playOnAwake = so.FindProperty("playOnAwake").boolValue;
            ps.InitState.prewarm = so.FindProperty("prewarm").boolValue;
            ps.InitState.speed = so.FindProperty("speed").floatValue;
            ps.InitState.maxNumParticles = so.FindProperty("InitialModule.maxNumParticles").intValue;
            ps.InitState.emissionRate = so.FindProperty("EmissionModule.rate.scalar").floatValue;
            ps.InitState.lengthInSec = so.FindProperty("lengthInSec").floatValue;
            ps.InitState.randomSeed = so.FindProperty("randomSeed").intValue;
            ParseCurve(so, ref ps.InitState.initModuleLiftTime, "InitialModule.startLifetime");
            ParseCurve(so, ref ps.InitState.initModuleSpeed, "InitialModule.startSpeed");
            ParseCurve(so, ref ps.InitState.initModuleSize, "InitialModule.startSize");
            ParseCurve(so, ref ps.InitState.initModuleRotation, "InitialModule.startRotation");
            ps.InitState.rotationModuleEnable = so.FindProperty("RotationModule.enabled").boolValue;
            ParseCurve(so, ref ps.InitState.rotationModuleCurve, "RotationModule.curve");
            ps.InitState.rotationMax = so.FindProperty("RotationModule.curve.maxCurve.m_Curve.Array.data[0].value").floatValue;
            ps.InitState.rotationMin = so.FindProperty("RotationModule.curve.minCurve.m_Curve.Array.data[0].value").floatValue;

            ps.InitState.sizeModuleEnable = so.FindProperty("SizeModule.enabled").boolValue;
            ParseCurve(so, ref ps.InitState.sizeModuleCurve, "SizeModule.curve");

            EditorUtility.SetDirty(ps);
            AssetDatabase.SaveAssets();
        }
    }

    void ParseCurve(SerializedObject so, ref Curve curve, string curveName)
    {
        curve.scalar = so.FindProperty(curveName + ".scalar").floatValue;
        curve.minMaxState = so.FindProperty(curveName + ".minMaxState").intValue;
        int keyFrameCount = so.FindProperty(curveName + ".maxCurve.m_Curve.Array.size").intValue;
        curve.maxCurve.keyFrameCount = keyFrameCount;
        curve.maxCurve.keyFrameContainer = keyFrameCount > 0 ? new KeyFrame[keyFrameCount] : null;

        for (int i = 0; i < keyFrameCount; ++i)
        {
            curve.maxCurve.keyFrameContainer[i] = new KeyFrame();
            curve.maxCurve.keyFrameContainer[i].time = so.FindProperty(curveName + ".maxCurve.m_Curve.Array.data[" + i.ToString() + "].time").floatValue;
            curve.maxCurve.keyFrameContainer[i].value = so.FindProperty(curveName + ".maxCurve.m_Curve.Array.data[" + i.ToString() + "].value").floatValue;
            curve.maxCurve.keyFrameContainer[i].inSlope = so.FindProperty(curveName + ".maxCurve.m_Curve.Array.data[" + i.ToString() + "].inSlope").floatValue;
            curve.maxCurve.keyFrameContainer[i].outSlope = so.FindProperty(curveName + ".maxCurve.m_Curve.Array.data[" + i.ToString() + "].outSlope").floatValue;
            //ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i].tangentMode = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.data[" + i.ToString() + "].tangentMode").intValue;
        }

        curve.maxCurve.preInfinity = so.FindProperty(curveName + ".maxCurve.m_PreInfinity").intValue;
        curve.maxCurve.postInfinity = so.FindProperty(curveName + ".maxCurve.m_PostInfinity").intValue;

        keyFrameCount = so.FindProperty(curveName + ".minCurve.m_Curve.Array.size").intValue;
        curve.minCurve.keyFrameCount = keyFrameCount;
        curve.minCurve.keyFrameContainer = keyFrameCount > 0 ? new KeyFrame[keyFrameCount] : null;

        for (int i = 0; i < keyFrameCount; ++i)
        {
            curve.minCurve.keyFrameContainer[i] = new KeyFrame();
            curve.minCurve.keyFrameContainer[i].time = so.FindProperty(curveName + ".minCurve.m_Curve.Array.data[" + i.ToString() + "].time").floatValue;
            curve.minCurve.keyFrameContainer[i].value = so.FindProperty(curveName + ".minCurve.m_Curve.Array.data[" + i.ToString() + "].value").floatValue;
            curve.minCurve.keyFrameContainer[i].inSlope = so.FindProperty(curveName + ".minCurve.m_Curve.Array.data[" + i.ToString() + "].inSlope").floatValue;
            curve.minCurve.keyFrameContainer[i].outSlope = so.FindProperty(curveName + ".minCurve.m_Curve.Array.data[" + i.ToString() + "].outSlope").floatValue;
            //ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i].tangentMode = so.FindProperty("SizeModule.curve.minCurve.m_Curve.Array.data[" + i.ToString() + "].tangentMode").intValue;
        }

        curve.minCurve.preInfinity = so.FindProperty(curveName + ".minCurve.m_PreInfinity").intValue;
        curve.minCurve.postInfinity = so.FindProperty(curveName + ".minCurve.m_PostInfinity").intValue;
    }
}
