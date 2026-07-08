#ifndef VISIBILITY_SHADING_HLSL
#define VISIBILITY_SHADING_HLSL

#include "VisibilityBufferCommon.hlsli"
#include "MeshLighting.hlsli"

PUSH_CONSTANT(Gleam::VisibilityShadingConstants, constants);

[shader("compute")]
[numthreads(VISIBILITY_RESOLVE_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadID : SV_DispatchThreadID)
{
    ByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];
    uint pixelCount = countsBuffer.Load(constants.batchIndex * sizeof(uint));
    if (dispatchThreadID >= pixelCount)
    {
        return;
    }

    ByteAddressBuffer offsetsBuffer = ResourceDescriptorHeap[constants.offsetsBuffer];
    uint offset = offsetsBuffer.Load(constants.batchIndex * sizeof(uint));

    ByteAddressBuffer pixelListBuffer = ResourceDescriptorHeap[constants.pixelListBuffer];
    uint packedPixel = pixelListBuffer.Load((offset + dispatchThreadID) * sizeof(uint));
    uint2 pixel = uint2(packedPixel & 0xFFFFu, packedPixel >> 16u);

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixel, 0));
    Gleam::VisibilityID visID = UnpackVisibilityID(packedID);

    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(visID.instanceID * sizeof(Gleam::MeshInstanceData));

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[instance.materialBuffer];
    LoadMaterialInstance(materialBuffer, instance.materialID);

    Gleam::MeshVertexOut IN = InterpolateVertexAttributes(instance, visID, pixel);
    Gleam::SurfaceOutput surface = SurfMain(IN);
    surface.roughness = max(surface.roughness, 0.04);

    float3 viewDir = normalize(camera.position - IN.worldPosition);
    float3x3 TBN = transpose(float3x3(IN.tangent, IN.bitangent, IN.normal));
    float3 worldNormal = normalize(mul(TBN, surface.normal));

    float3 color = EvaluateMeshLighting(surface,
                                        IN.worldPosition,
                                        worldNormal,
                                        viewDir,
                                        pixel,
                                        constants.brdfTexture,
                                        constants.ggxEssTexture,
                                        constants.ggxEAvgTexture,
                                        constants.diffuseReflectionTexture,
                                        constants.specularReflectionTexture,
                                        constants.shadowTexture);

    RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[constants.colorTarget];
    colorTarget[pixel] = float4(color, surface.albedo.a);
}
#endif // VISIBILITY_SHADING_HLSL
