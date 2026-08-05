#include "Common.hlsli"
#include "VisibilityBufferCommon.hlsli"

PUSH_CONSTANT(Gleam::PickingConstants, constants);

[shader("compute")]
[numthreads(PICKING_GROUP_SIZE_X, PICKING_GROUP_SIZE_Y, 1)]
void visibilityPickingShader(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (any(dispatchThreadID.xy >= uint2(constants.rectWidth, constants.rectHeight)))
    {
        return;
    }

    uint2 pixelCoord = uint2(constants.rectOffsetX, constants.rectOffsetY) + dispatchThreadID.xy;

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoord, 0));
    if (IsValidVisibilityID(packedID) == false)
    {
        return;
    }

    Gleam::VisibilityID visibility = UnpackVisibilityID(packedID);

    RWByteAddressBuffer maskBuffer = ResourceDescriptorHeap[constants.maskBuffer];
    uint original;
    maskBuffer.InterlockedOr((visibility.instanceID >> 5u) * sizeof(uint), 1u << (visibility.instanceID & 31u), original);
}
