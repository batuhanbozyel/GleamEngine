#include <metal_stdlib>
using namespace metal;

// Buffer indices are kept above the IR-converter reserved
kernel void transformDispatchRaysIndirectArgs(
    device const uint* threadDims   [[buffer(13)]],
    device uint*       threadgroups [[buffer(14)]],
    constant uint2&    groupSize    [[buffer(15)]])
{
    threadgroups[0] = (threadDims[0] + groupSize.x - 1) / groupSize.x;
    threadgroups[1] = (threadDims[1] + groupSize.y - 1) / groupSize.y;
    threadgroups[2] =  threadDims[2];
}
