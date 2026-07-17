#ifndef VISIBILITY_SHADING_HLSL
#define VISIBILITY_SHADING_HLSL

#include "VisibilityBufferCommon.hlsli"
#include "MeshLighting.hlsli"

PUSH_CONSTANT(Gleam::VisibilityShadingConstants, constants);

[shader("compute")]
[numthreads(VISIBILITY_RESOLVE_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadID : SV_DispatchThreadID)
{
    Gleam::VisibilitySample visSample;
    
    [branch]
    if (UnpackVisibilityShading(constants.resolve, dispatchThreadID, constants.barycentricCoords, constants.barycentricDerivatives, visSample))
    {
        float3 viewDir = normalize(camera.position - visSample.vertex.worldPosition);
        float3x3 TBN = transpose(float3x3(visSample.vertex.tangent, visSample.vertex.bitangent, visSample.vertex.normal));
        float3 worldNormal = normalize(mul(TBN, visSample.surface.normal));

        float3 color = EvaluateMeshLighting(visSample.surface,
                                            visSample.vertex.worldPosition,
                                            worldNormal,
                                            viewDir,
                                            visSample.pixelCoords,
                                            constants.brdfTexture,
                                            constants.ggxEssTexture,
                                            constants.ggxEAvgTexture,
                                            constants.diffuseReflectionTexture,
                                            constants.specularReflectionTexture,
                                            constants.shadowTexture);

        RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[constants.colorTarget];
        colorTarget[visSample.pixelCoords] = float4(color, visSample.surface.albedo.a);
    }
}
#endif // VISIBILITY_SHADING_HLSL
