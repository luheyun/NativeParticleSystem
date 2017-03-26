#pragma once

#include "Math/Vector4.h"
#include "Utilities/Utility.h"

class ShaderConstantCacheD3D9
{
public:
    enum { kCacheSize = 256 };

public:
    ShaderConstantCacheD3D9() { memset(m_Flags, 0, sizeof(m_Flags));  Invalidate(); }

    void SetValues(int index, const float* values, int count)
    {
        float* dest = m_Values[index].GetPtr();
        UInt8 andedFlags = m_Flags[index];
        for (int i = 1; i < count; i++)
            andedFlags &= m_Flags[index + i];
        // Not worth filtering values bigger than one register
        if (andedFlags == kValid && count == 1)
        {
            // We have a valid value which is not dirty
            UInt32* destInt32 = reinterpret_cast<UInt32*>(dest);
            int sizeInt32 = count * sizeof(Vector4f) / sizeof(UInt32);
            if (CompareArrays(destInt32, reinterpret_cast<const UInt32*>(values), sizeInt32))
                return;
        }
        memcpy(dest, values, count * sizeof(Vector4f));
        // If all values are marked as dirty we are done
        if (andedFlags & kDirty)
            return;
        // Update flags
        for (int i = 0; i < count; i++)
            m_Flags[index + i] = kValid | kDirty;
        // Add dirty range or append to last range
        if (!m_DirtyRanges.empty() && m_DirtyRanges.back().second == index)
            m_DirtyRanges.back().second += count;
        else
            m_DirtyRanges.push_back(Range(index, index + count));
    }

    void Invalidate()
    {
        memset(m_Flags, 0, sizeof(m_Flags));
    }

private:
    enum
    {
        kValid = 1 << 0,
        kDirty = 1 << 1
    };

    UInt8 m_Flags[kCacheSize];
    Vector4f m_Values[kCacheSize];
    typedef std::pair<int, int> Range;
    std::vector<Range> m_DirtyRanges;
};
