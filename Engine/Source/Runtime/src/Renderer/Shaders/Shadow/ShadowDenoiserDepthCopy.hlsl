#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::ShadowDenoiserDepthCopyConstants, constants);

[shader("compute")]
[numthreads(8, 8, 1)]
void shadowDenoiserDepthCopy(uint3 did : SV_DispatchThreadID)
{
    Texture2D<float> src = ResourceDescriptorHeap[constants.sourceDepth];
    RWTexture2D<float> dst = ResourceDescriptorHeap[constants.destDepth];
    dst[did.xy] = src.Load(int3(did.xy, 0));
}
