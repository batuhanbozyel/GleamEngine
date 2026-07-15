#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::PrepareShadowRayDispatchArgsConstants, constants);

[shader("compute")]
[numthreads(1, 1, 1)]
void prepareShadowRayDispatchArgs()
{
    ByteAddressBuffer   tileCountBuffer    = ResourceDescriptorHeap[constants.tileCountBuffer];
    RWByteAddressBuffer dispatchArgsBuffer = ResourceDescriptorHeap[constants.dispatchArgsBuffer];

    const uint tileCount = tileCountBuffer.Load(0);
    dispatchArgsBuffer.Store3(0, uint3(tileCount, SHADOW_TILE_WIDTH * SHADOW_TILE_HEIGHT, 1u));
}
