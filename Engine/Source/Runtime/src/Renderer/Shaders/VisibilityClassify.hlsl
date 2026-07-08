#include "Common.hlsli"
#include "VisibilityBufferCommon.hlsli"

PUSH_CONSTANT(Gleam::VisibilityClassifyConstants, constants);

[shader("compute")]
[numthreads(8, 8, 1)]
void visibilityCountShader(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    if (any(pixelCoord >= (uint2) camera.resolution))
    {
        return;
    }

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoord, 0));
    if (IsValidVisibilityID(packedID) == false)
    {
        return;
    }
    Gleam::VisibilityID visibility = UnpackVisibilityID(packedID);
    
    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(visibility.instanceID * sizeof(Gleam::MeshInstanceData));

    RWByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];
    uint original;
    countsBuffer.InterlockedAdd(instance.batchIndex * sizeof(uint), 1u, original);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void visibilityScatterShader(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    if (any(pixelCoord >= (uint2) camera.resolution))
    {
        return;
    }

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoord, 0));
    if (IsValidVisibilityID(packedID) == false)
    {
        return;
    }

    Gleam::VisibilityID visibility = UnpackVisibilityID(packedID);
    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(visibility.instanceID * sizeof(Gleam::MeshInstanceData));

    RWByteAddressBuffer cursorsBuffer = ResourceDescriptorHeap[constants.cursorsBuffer];
    uint slot;
    cursorsBuffer.InterlockedAdd(instance.batchIndex * sizeof(uint), 1u, slot);

    ByteAddressBuffer offsetsBuffer = ResourceDescriptorHeap[constants.offsetsBuffer];
    uint offset = offsetsBuffer.Load(instance.batchIndex * sizeof(uint));

    RWByteAddressBuffer pixelListBuffer = ResourceDescriptorHeap[constants.pixelListBuffer];
    pixelListBuffer.Store((offset + slot) * sizeof(uint), (pixelCoord.y << 16u) | pixelCoord.x);
}
