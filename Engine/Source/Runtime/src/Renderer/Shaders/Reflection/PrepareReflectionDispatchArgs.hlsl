#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::PrepareReflectionDispatchArgsConstants, constants);

[shader("compute")]
[numthreads(1, 1, 1)]
void prepareReflectionDispatchArgs()
{
    RWByteAddressBuffer rayCounterBuffer           = ResourceDescriptorHeap[constants.rayCounterBuffer];
    RWByteAddressBuffer rayDispatchArgsBuffer      = ResourceDescriptorHeap[constants.rayDispatchArgsBuffer];
    RWByteAddressBuffer denoiserDispatchArgsBuffer = ResourceDescriptorHeap[constants.denoiserDispatchArgsBuffer];

    const uint rayCount = rayCounterBuffer.Load(REFLECTION_RAY_COUNTER_HW * sizeof(uint));
    rayCounterBuffer.Store(REFLECTION_RAY_COUNTER_HW * sizeof(uint), 0u);
    rayCounterBuffer.Store(REFLECTION_RAY_COUNTER_HW_HISTORY * sizeof(uint), rayCount);
    rayDispatchArgsBuffer.Store3(0, uint3(rayCount, 1u, 1u));

    const uint tileCount = rayCounterBuffer.Load(REFLECTION_RAY_COUNTER_DENOISE * sizeof(uint));
    rayCounterBuffer.Store(REFLECTION_RAY_COUNTER_DENOISE * sizeof(uint), 0u);
    rayCounterBuffer.Store(REFLECTION_RAY_COUNTER_DENOISE_HISTORY * sizeof(uint), tileCount);
    denoiserDispatchArgsBuffer.Store3(0, uint3(tileCount, 1u, 1u));
}
