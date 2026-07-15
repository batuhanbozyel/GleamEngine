#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::VisibilityAllocateConstants, constants);

[shader("compute")]
[numthreads(1, 1, 1)]
void visibilityAllocateShader()
{
    ByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];
    RWByteAddressBuffer offsetsBuffer = ResourceDescriptorHeap[constants.offsetsBuffer];
    RWByteAddressBuffer cursorsBuffer = ResourceDescriptorHeap[constants.cursorsBuffer];
    RWByteAddressBuffer dispatchArgsBuffer = ResourceDescriptorHeap[constants.dispatchArgsBuffer];

    uint offset = 0u;
    for (uint batchIndex = 0u; batchIndex < constants.numBatches; ++batchIndex)
    {
        uint count = countsBuffer.Load(batchIndex * sizeof(uint));
        offsetsBuffer.Store(batchIndex * sizeof(uint), offset);
        cursorsBuffer.Store(batchIndex * sizeof(uint), offset); // seed scatter cursors
        offset += count;

        uint groupCount = (count + VISIBILITY_RESOLVE_GROUP_SIZE - 1u) / VISIBILITY_RESOLVE_GROUP_SIZE;
        dispatchArgsBuffer.Store3(batchIndex * sizeof(Gleam::DispatchIndirectArguments), uint3(groupCount, 1u, 1u));
    }
}
