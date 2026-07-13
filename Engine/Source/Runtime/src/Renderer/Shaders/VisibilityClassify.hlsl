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
    RWByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];

    uint original;
    uint batchIndex = UnpackVisibilityBatchIndex(packedID);
    countsBuffer.InterlockedAdd(batchIndex * sizeof(uint), 1u, original);
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

    RWByteAddressBuffer cursorsBuffer = ResourceDescriptorHeap[constants.cursorsBuffer];
    RWByteAddressBuffer pixelListBuffer = ResourceDescriptorHeap[constants.pixelListBuffer];

    uint slot;
    uint batchIndex = UnpackVisibilityBatchIndex(packedID);
    cursorsBuffer.InterlockedAdd(batchIndex * sizeof(uint), 1u, slot);
    pixelListBuffer.Store(slot * sizeof(uint), (pixelCoord.y << 16u) | pixelCoord.x);
}
