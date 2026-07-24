#include "../Common.hlsli"
#include "XeGTAO.hlsli"

PUSH_CONSTANT(Gleam::GTAODepthPrefilterConstants, constants);

[shader("compute")]
[numthreads(8, 8, 1)]
void gtaoDepthPrefilter(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
    Texture2D<float>     sourceDepth = ResourceDescriptorHeap[constants.sourceDepth];
    RWTexture2D<lpfloat> outDepth0   = ResourceDescriptorHeap[constants.outDepthMip0];
    RWTexture2D<lpfloat> outDepth1   = ResourceDescriptorHeap[constants.outDepthMip1];
    RWTexture2D<lpfloat> outDepth2   = ResourceDescriptorHeap[constants.outDepthMip2];
    RWTexture2D<lpfloat> outDepth3   = ResourceDescriptorHeap[constants.outDepthMip3];
    RWTexture2D<lpfloat> outDepth4   = ResourceDescriptorHeap[constants.outDepthMip4];

    XeGTAO_PrefilterDepths16x16(dispatchThreadID, groupThreadID, constants.gtao, sourceDepth,
                                Sampler_Point_Clamp, outDepth0, outDepth1, outDepth2, outDepth3, outDepth4);
}
