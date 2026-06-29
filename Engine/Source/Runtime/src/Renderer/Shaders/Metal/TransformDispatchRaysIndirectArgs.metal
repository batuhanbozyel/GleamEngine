#include <metal_stdlib>
using namespace metal;

// Buffer indices are kept above the IR-converter reserved
// bind points so the transform never clobbers the ray-gen dispatch's argument table bindings.
kernel void transformDispatchRaysIndirectArgs(
    device const uint* threadDims   [[buffer(16)]],
    device uint*       threadgroups [[buffer(17)]],
    constant uint2&    groupSize    [[buffer(18)]])
{
    threadgroups[0] = (threadDims[0] + groupSize.x - 1) / groupSize.x;
    threadgroups[1] = (threadDims[1] + groupSize.y - 1) / groupSize.y;
    threadgroups[2] =  threadDims[2];
}
