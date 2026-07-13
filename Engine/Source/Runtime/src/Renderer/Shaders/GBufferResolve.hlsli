#ifndef GBUFFER_RESOLVE_HLSL
#define GBUFFER_RESOLVE_HLSL

#define MOTION_VECTOR_PASS

#include "Common.hlsli"
#include "VisibilityBufferCommon.hlsli"

PUSH_CONSTANT(Gleam::GBufferResolveConstants, constants);

[shader("compute")]
[numthreads(VISIBILITY_RESOLVE_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadID : SV_DispatchThreadID)
{
    Gleam::MeshVertexOut IN;
    Gleam::SurfaceOutput surface;
    uint2 pixelCoords;
    if (!UnpackVisibilityShading(constants.resolve, dispatchThreadID, IN, surface, pixelCoords))
    {
        return;
    }
    
    float3x3 TBN = transpose(float3x3(IN.tangent, IN.bitangent, IN.normal));
    float3 shadingNormal = mul(TBN, surface.normal);
    
    float4 prevClip = mul(camera.prevViewProjectionMatrix, float4(IN.prevWorldPosition, 1.0f));
    float2 prevNdc = prevClip.xy / prevClip.w;
    float2 prevViewport = (prevNdc * float2(0.5f, -0.5f) + 0.5f) * camera.resolution;
    float2 motionVector = prevViewport - IN.position.xy;

    RWTexture2D<float2> motionVectorTarget = ResourceDescriptorHeap[constants.motionVectorTarget];
    RWTexture2D<float2> geometryNormalTarget = ResourceDescriptorHeap[constants.geometryNormalTarget];
    RWTexture2D<float2> shadingNormalTarget = ResourceDescriptorHeap[constants.shadingNormalTarget];
    RWTexture2D<float> roughnessTarget = ResourceDescriptorHeap[constants.roughnessTarget];

    motionVectorTarget[pixelCoords] = motionVector;
    geometryNormalTarget[pixelCoords] = OctEncode(normalize(IN.normal));
    shadingNormalTarget[pixelCoords] = OctEncode(normalize(shadingNormal));
    roughnessTarget[pixelCoords] = surface.roughness;
}
#endif // GBUFFER_RESOLVE_HLSL
