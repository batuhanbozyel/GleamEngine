#include "../Common.hlsli"
#include "XeGTAO.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
PUSH_CONSTANT(Gleam::GTAOMainPassConstants, constants);

static Texture2D<lpfloat> workingDepth = ResourceDescriptorHeap[constants.workingDepth];
static RWTexture2D<uint> outWorkingAOTerm = ResourceDescriptorHeap[constants.outWorkingAOTerm];
static RWTexture2D<unorm float> outWorkingEdges = ResourceDescriptorHeap[constants.outWorkingEdges];

lpfloat3 LoadNormal(int2 pos)
{
    Texture2D<float2> normalTexture = ResourceDescriptorHeap[constants.normalTexture];
    float3 worldNormal = OctDecode(normalTexture.Load(int3(pos, 0)));
    return (lpfloat3)normalize(mul((float3x3)camera.viewMatrix, worldNormal));
}

// Hilbert-curve index (Intel XeGTAO / https://www.shadertoy.com/view/3tB3z3), used to drive the
// spatiotemporal blue noise below without a precomputed LUT texture.
#define XE_HILBERT_LEVEL 6U
#define XE_HILBERT_WIDTH (1U << XE_HILBERT_LEVEL)
uint HilbertIndex(uint posX, uint posY)
{
    uint index = 0U;
    for (uint curLevel = XE_HILBERT_WIDTH / 2U; curLevel > 0U; curLevel /= 2U)
    {
        uint regionX = (posX & curLevel) > 0U;
        uint regionY = (posY & curLevel) > 0U;
        index += curLevel * curLevel * ((3U * regionX) ^ regionY);
        if (regionY == 0U)
        {
            if (regionX == 1U)
            {
                posX = (XE_HILBERT_WIDTH - 1U) - posX;
                posY = (XE_HILBERT_WIDTH - 1U) - posY;
            }
            uint temp = posX;
            posX = posY;
            posY = temp;
        }
    }
    return index;
}

// R2 low-discrepancy sequence over the Hilbert index. temporalIndex is 0 when not using TAA.
lpfloat2 SpatioTemporalNoise(uint2 pixCoord, uint temporalIndex)
{
    uint index = HilbertIndex(pixCoord.x & (XE_HILBERT_WIDTH - 1U), pixCoord.y & (XE_HILBERT_WIDTH - 1U));
    index += 288U * (temporalIndex & (XE_HILBERT_WIDTH - 1U));
    return lpfloat2(frac(0.5 + index * float2(0.75487766624669276005f, 0.5698402909980532659114f)));
}

[shader("compute")]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void gtaoLowMainPass(uint2 pixCoord : SV_DispatchThreadID)
{
    XeGTAO_MainPass(pixCoord, 1, 2, SpatioTemporalNoise(pixCoord, (uint)constants.gtao.NoiseIndex), LoadNormal(pixCoord),
                    constants.gtao, workingDepth, Sampler_Point_Clamp, outWorkingAOTerm, outWorkingEdges);
}

[shader("compute")]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void gtaoMediumMainPass(uint2 pixCoord : SV_DispatchThreadID)
{
    XeGTAO_MainPass(pixCoord, 2, 2, SpatioTemporalNoise(pixCoord, (uint)constants.gtao.NoiseIndex), LoadNormal(pixCoord),
                    constants.gtao, workingDepth, Sampler_Point_Clamp, outWorkingAOTerm, outWorkingEdges);
}

[shader("compute")]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void gtaoHighMainPass(uint2 pixCoord : SV_DispatchThreadID)
{
    XeGTAO_MainPass(pixCoord, 3, 3, SpatioTemporalNoise(pixCoord, (uint)constants.gtao.NoiseIndex), LoadNormal(pixCoord),
                    constants.gtao, workingDepth, Sampler_Point_Clamp, outWorkingAOTerm, outWorkingEdges);
}

[shader("compute")]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void gtaoUltraMainPass(uint2 pixCoord : SV_DispatchThreadID)
{
    XeGTAO_MainPass(pixCoord, 9, 3, SpatioTemporalNoise(pixCoord, (uint)constants.gtao.NoiseIndex), LoadNormal(pixCoord),
                    constants.gtao, workingDepth, Sampler_Point_Clamp, outWorkingAOTerm, outWorkingEdges);
}
