#include "Common.hlsli"
#include "../ShaderTypes.h"

PUSH_CONSTANT(GEditor::SelectionOutlineUniforms, uniforms);

[shader("pixel")]
float4 selectionOutlineFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    Texture2D<float> selectionMask = ResourceDescriptorHeap[uniforms.selectionMask];
    int2 pixelCoord = int2(IN.position.xy);

    if (selectionMask.Load(int3(pixelCoord, 0)) > 0.5)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    int radius = (int) ceil(uniforms.outlineWidth + 0.5);
    float closest = uniforms.outlineWidth + 1.0;

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            float dist = length(float2(x, y));
            if (dist < closest && selectionMask.Load(int3(pixelCoord + int2(x, y), 0)) > 0.5)
            {
                closest = dist;
            }
        }
    }

    float alpha = saturate(uniforms.outlineWidth + 0.5 - closest) * uniforms.outlineColor.a;
    return float4(uniforms.outlineColor.rgb, alpha);
}
