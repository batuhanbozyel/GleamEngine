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
    uint batchIndex = UnpackVisibilityBatchIndex(packedID);

    RWByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];
    
    bool pending = true;
    while (pending)
    {
        uint uniformBatch = WaveReadLaneFirst(batchIndex);
        if (batchIndex == uniformBatch)
        {
            uint waveTotal = WaveActiveCountBits(true);
            if (WaveIsFirstLane())
            {
                uint original;
                countsBuffer.InterlockedAdd(uniformBatch * sizeof(uint), waveTotal, original);
            }
            pending = false;
        }
    }
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
    uint batchIndex = UnpackVisibilityBatchIndex(packedID);

    RWByteAddressBuffer cursorsBuffer = ResourceDescriptorHeap[constants.cursorsBuffer];
    RWByteAddressBuffer pixelListBuffer = ResourceDescriptorHeap[constants.pixelListBuffer];
    
    bool pending = true;
    while (pending)
    {
        uint uniformBatch = WaveReadLaneFirst(batchIndex);
        if (batchIndex == uniformBatch)
        {
            uint laneOffset = WavePrefixCountBits(true);
            uint waveTotal = WaveActiveCountBits(true);
            uint waveBase = 0u;
            if (laneOffset == 0u)
            {
                cursorsBuffer.InterlockedAdd(uniformBatch * sizeof(uint), waveTotal, waveBase);
            }
            waveBase = WaveReadLaneFirst(waveBase);

            pixelListBuffer.Store((waveBase + laneOffset) * sizeof(uint), (pixelCoord.y << 16u) | pixelCoord.x);
            pending = false;
        }
    }
}
