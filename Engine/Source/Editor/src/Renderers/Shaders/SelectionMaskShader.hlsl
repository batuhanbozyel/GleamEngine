#include "Common.hlsli"
#include "VisibilityBufferCommon.hlsli"
#include "../ShaderTypes.h"

PUSH_CONSTANT(GEditor::SelectionMaskConstants, constants);

[shader("compute")]
[numthreads(SELECTION_MASK_GROUP_SIZE_X, SELECTION_MASK_GROUP_SIZE_Y, 1)]
void selectionMaskShader(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    if (any(pixelCoord >= uint2(constants.targetWidth, constants.targetHeight)))
    {
        return;
    }

    float selected = 0.0;

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    PackedVisibilityID packedID = visibilityBuffer.Load(int3(pixelCoord, 0));
    if (IsValidVisibilityID(packedID))
    {
        Gleam::VisibilityID visibility = UnpackVisibilityID(packedID);
        ByteAddressBuffer instanceMaskBuffer = ResourceDescriptorHeap[constants.instanceMaskBuffer];
        uint word = instanceMaskBuffer.Load((visibility.instanceID >> 5u) * sizeof(uint));
        selected = ((word >> (visibility.instanceID & 31u)) & 1u) ? 1.0 : 0.0;
    }

    RWTexture2D<unorm float> selectionMask = ResourceDescriptorHeap[constants.selectionMask];
    selectionMask[pixelCoord] = selected;
}
