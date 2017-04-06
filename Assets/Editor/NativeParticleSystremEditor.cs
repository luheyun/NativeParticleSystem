using UnityEngine;
using UnityEditor;
using System.Collections;

[CustomEditor(typeof(NativeParticleSystem))]
public class NativeParticleSystremEditor : Editor
{
    private int count = 0;
    public override void OnInspectorGUI()
    {
        base.OnInspectorGUI();

        NativeParticleSystem ps = target as NativeParticleSystem;
        SerializedObject so = new SerializedObject(ps.particleSystem);

        if (count == 0)
        {
            //SerializedProperty it = so.GetIterator();
            //while (it.Next(true))
            //    Debug.Log(it.propertyPath + " " + it.propertyType.ToString());

            ++count;
        }

        if (GUILayout.Button("Save Particle System Property"))
        {
            ps.InitState.looping = so.FindProperty("looping").boolValue;
            ps.InitState.playOnAwake = so.FindProperty("playOnAwake").boolValue;
            ps.InitState.prewarm = so.FindProperty("prewarm").boolValue;
            ps.InitState.speed = so.FindProperty("speed").floatValue;
            ps.InitState.maxNumParticles = so.FindProperty("InitialModule.maxNumParticles").intValue;
            ps.InitState.emissionRate = so.FindProperty("EmissionModule.rate.scalar").floatValue;
            ps.InitState.randomSeed = so.FindProperty("randomSeed").intValue;
            ps.InitState.rotationModuleEnable = so.FindProperty("RotationModule.enabled").boolValue;

            ps.InitState.rotationMax = so.FindProperty("RotationModule.curve.maxCurve.m_Curve.Array.data[0].value").floatValue;
            ps.InitState.rotationMin = so.FindProperty("RotationModule.curve.minCurve.m_Curve.Array.data[0].value").floatValue;

            ps.InitState.sizeModuleEnable = so.FindProperty("SizeModule.enabled").boolValue; 
            ps.InitState.sizeModuleCurve.minMaxState = so.FindProperty("SizeModule.curve.minMaxState").intValue;
            ps.InitState.sizeModuleCurve.maxCurve.keyFrameCount = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.size").intValue;
            //ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer = new KeyFrame[ps.InitState.sizeModuleCurve.maxCurve.keyFrameCount];

            //for (int i = 0; i < ps.InitState.sizeModuleCurve.maxCurve.keyFrameCount; ++i)
            //{
            //    ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i] = new KeyFrame();
            //    ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i].time = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.data[" + i.ToString() + "].time").floatValue;
            //    ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i].value = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.data[" + i.ToString() + "].value").floatValue;
            //    ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i].inSlope = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.data[" + i.ToString() + "].inSlope").floatValue;
            //    ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i].outSlope = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.data[" + i.ToString() + "].outSlope").floatValue;
            //    //ps.InitState.sizeModuleCurve.maxCurve.keyFrameContainer[i].tangentMode = so.FindProperty("SizeModule.curve.maxCurve.m_Curve.Array.data[" + i.ToString() + "].tangentMode").intValue;
            //}

            ps.InitState.sizeModuleCurve.maxCurve.preInfinity = so.FindProperty("SizeBySpeedModule.curve.maxCurve.m_PreInfinity").intValue;
            ps.InitState.sizeModuleCurve.maxCurve.postInfinity = so.FindProperty("SizeBySpeedModule.curve.maxCurve.m_PostInfinity").intValue;

            //for (int i = 0; i < ps.InitState.sizeModuleCurve.minCurve.keyFrameCount; ++i)
            //{
            //    ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i] = new KeyFrame();
            //    ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i].time = so.FindProperty("SizeModule.curve.minCurve.m_Curve.Array.data[" + i.ToString() + "].time").floatValue;
            //    ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i].value = so.FindProperty("SizeModule.curve.minCurve.m_Curve.Array.data[" + i.ToString() + "].value").floatValue;
            //    ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i].inSlope = so.FindProperty("SizeModule.curve.minCurve.m_Curve.Array.data[" + i.ToString() + "].inSlope").floatValue;
            //    ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i].outSlope = so.FindProperty("SizeModule.curve.minCurve.m_Curve.Array.data[" + i.ToString() + "].outSlope").floatValue;
            //    //ps.InitState.sizeModuleCurve.minCurve.keyFrameContainer[i].tangentMode = so.FindProperty("SizeModule.curve.minCurve.m_Curve.Array.data[" + i.ToString() + "].tangentMode").intValue;
            //}

            ps.InitState.sizeModuleCurve.minCurve.preInfinity = so.FindProperty("SizeBySpeedModule.curve.minCurve.m_PreInfinity").intValue;
            ps.InitState.sizeModuleCurve.minCurve.postInfinity = so.FindProperty("SizeBySpeedModule.curve.minCurve.m_PostInfinity").intValue;
        }
    }
}
