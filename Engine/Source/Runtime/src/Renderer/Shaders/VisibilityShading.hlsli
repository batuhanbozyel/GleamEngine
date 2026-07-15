#ifndef VISIBILITY_SHADING_HLSL
#define VISIBILITY_SHADING_HLSL

#include "VisibilityBufferCommon.hlsli"
#include "MeshLighting.hlsli"

PUSH_CONSTANT(Gleam::VisibilityShadingConstants, constants);

[shader("compute")]
[numthreads(VISIBILITY_RESOLVE_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoords;
    Gleam::MeshVertexOut IN;
    Gleam::SurfaceOutput surface;
    
    [branch]
    if (UnpackVisibilityShading(constants.resolve, dispatchThreadID, IN, surface, pixelCoords))
    {
        float3 viewDir = normalize(camera.position - IN.worldPosition);
        float3x3 TBN = transpose(float3x3(IN.tangent, IN.bitangent, IN.normal));
        float3 worldNormal = normalize(mul(TBN, surface.normal));

        float3 color = EvaluateMeshLighting(surface,
                                        IN.worldPosition,
                                        worldNormal,
                                        viewDir,
                                        pixelCoords,
                                        constants.brdfTexture,
                                        constants.ggxEssTexture,
                                        constants.ggxEAvgTexture,
                                        constants.diffuseReflectionTexture,
                                        constants.specularReflectionTexture,
                                        constants.shadowTexture);

        RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[constants.colorTarget];
        colorTarget[pixelCoords] = float4(color, surface.albedo.a);
    }
}
#endif // VISIBILITY_SHADING_HLSL
