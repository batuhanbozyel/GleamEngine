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
    uint2 pixelCoords = uint2(packedPixel & 0xFFFFu, packedPixel >> 16u);

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoords, 0));
    Gleam::VisibilityID visID = UnpackVisibilityID(packedID);

    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(visID.instanceID * sizeof(Gleam::MeshInstanceData));

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[instance.materialBuffer];
    LoadMaterialInstance(materialBuffer, instance.materialID);

    Gleam::MeshVertexOut IN = InterpolateVertexAttributes(instance, visID, pixelCoords);
    Gleam::SurfaceOutput surface = SurfMain(IN);
    surface.roughness = max(surface.roughness, 0.04);
    
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
    shadingNormalTarget[pixelCoords] = OctEncode(normalize(surface.normal));
    roughnessTarget[pixelCoords] = surface.roughness;
}
#endif // GBUFFER_RESOLVE_HLSL
