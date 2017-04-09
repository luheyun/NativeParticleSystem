using System;
using System.Runtime.InteropServices;

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class ColorKey
{
    public int color;
    public int time;
}

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class AlphaKey
{
    public int alpha;
    public int time;
}

[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class Gradient
{
    public int colorKeyCount;
    public ColorKey[] colorKeys;
    public int alphaKeyCount;
    public AlphaKey[] alphaKeys;
}


[System.Serializable]
[StructLayoutAttribute(LayoutKind.Sequential)]
public class MinMaxGradient
{
    public Gradient maxGradient;
    public Gradient minGradient;
    public int minColor; // rgba
    public int maxColor;
    public int minMaxState;
}
