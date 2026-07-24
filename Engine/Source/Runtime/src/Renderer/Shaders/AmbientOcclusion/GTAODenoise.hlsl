#include "../Common.hlsli"
#include "XeGTAO.hlsli"

PUSH_CONSTANT(Gleam::GTAODenoiseConstants, constants);

[shader("compute")]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void gtaoDenoise(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<uint> sourceAOTerm = ResourceDescriptorHeap[constants.sourceAOTerm];
    Texture2D<lpfloat> sourceEdges = ResourceDescriptorHeap[constants.sourceEdges];
    RWTexture2D<uint> outFinalAOTerm = ResourceDescriptorHeap[constants.outFinalAOTerm];

    const uint2 pixCoordBase = dispatchThreadID * uint2(2, 1);
    XeGTAO_Denoise(pixCoordBase, constants.gtao, sourceAOTerm, sourceEdges, Sampler_Point_Clamp,
                   outFinalAOTerm, constants.finalApply != 0);
}
