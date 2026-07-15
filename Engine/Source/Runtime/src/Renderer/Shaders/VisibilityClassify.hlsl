#include "Common.hlsli"
#include "Random.hlsli"
#include "VisibilityBufferCommon.hlsli"

PUSH_CONSTANT(Gleam::VisibilityClassifyConstants, constants);

#define CLASSIFY_TABLE_MASK (VISIBILITY_CLASSIFY_GROUP_SIZE - 1u)
#define CLASSIFY_TABLE_BITS (firstbithigh(VISIBILITY_CLASSIFY_GROUP_SIZE))
#define CLASSIFY_INVALID_BATCH 0xFFFFFFFFu

groupshared uint gs_BatchIDs[VISIBILITY_CLASSIFY_GROUP_SIZE];
groupshared uint gs_BatchCounts[VISIBILITY_CLASSIFY_GROUP_SIZE];
groupshared uint gs_BatchBases[VISIBILITY_CLASSIFY_GROUP_SIZE];

uint FindOrInsertBatch(uint batchIndex)
{
    uint slot = KnuthHash(batchIndex, CLASSIFY_TABLE_BITS);
    for (;;)
    {
        uint prevBatch;
        InterlockedCompareExchange(gs_BatchIDs[slot], CLASSIFY_INVALID_BATCH, batchIndex, prevBatch);
        if (prevBatch == CLASSIFY_INVALID_BATCH || prevBatch == batchIndex)
        {
            return slot;
        }
        slot = (slot + 1u) & CLASSIFY_TABLE_MASK;
    }
}

[shader("compute")]
[numthreads(VISIBILITY_CLASSIFY_GROUP_SIZE_X, VISIBILITY_CLASSIFY_GROUP_SIZE_Y, 1)]
void visibilityCountShader(uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    gs_BatchIDs[groupIndex] = CLASSIFY_INVALID_BATCH;
    gs_BatchCounts[groupIndex] = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint2 pixelCoord = dispatchThreadID.xy;
    if (all(pixelCoord < (uint2) camera.resolution))
    {
        Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
        PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoord, 0));
        if (IsValidVisibilityID(packedID))
        {
            uint batchIndex = UnpackVisibilityBatchIndex(packedID);
            uint entry = FindOrInsertBatch(batchIndex);
            InterlockedAdd(gs_BatchCounts[entry], 1u);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    uint batchIndex = gs_BatchIDs[groupIndex];
    if (batchIndex != CLASSIFY_INVALID_BATCH)
    {
        RWByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];
        uint original;
        countsBuffer.InterlockedAdd(batchIndex * sizeof(uint), gs_BatchCounts[groupIndex], original);
    }
}

[shader("compute")]
[numthreads(VISIBILITY_CLASSIFY_GROUP_SIZE_X, VISIBILITY_CLASSIFY_GROUP_SIZE_Y, 1)]
void visibilityScatterShader(uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    gs_BatchIDs[groupIndex] = CLASSIFY_INVALID_BATCH;
    gs_BatchCounts[groupIndex] = 0u;
    GroupMemoryBarrierWithGroupSync();

    uint2 pixelCoord = dispatchThreadID.xy;
    bool valid = false;
    uint entry = 0u;
    uint rank = 0u;
    if (all(pixelCoord < (uint2) camera.resolution))
    {
        Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
        PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoord, 0));
        if (IsValidVisibilityID(packedID))
        {
            uint batchIndex = UnpackVisibilityBatchIndex(packedID);
            entry = FindOrInsertBatch(batchIndex);
            InterlockedAdd(gs_BatchCounts[entry], 1u, rank);
            valid = true;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    uint batchIndex = gs_BatchIDs[groupIndex];
    if (batchIndex != CLASSIFY_INVALID_BATCH)
    {
        RWByteAddressBuffer cursorsBuffer = ResourceDescriptorHeap[constants.cursorsBuffer];
        uint base;
        cursorsBuffer.InterlockedAdd(batchIndex * sizeof(uint), gs_BatchCounts[groupIndex], base);
        gs_BatchBases[groupIndex] = base;
    }
    GroupMemoryBarrierWithGroupSync();

    if (valid)
    {
        RWByteAddressBuffer pixelListBuffer = ResourceDescriptorHeap[constants.pixelListBuffer];
        uint slot = gs_BatchBases[entry] + rank;
        pixelListBuffer.Store(slot * sizeof(uint), (pixelCoord.y << 16u) | pixelCoord.x);
    }
}
