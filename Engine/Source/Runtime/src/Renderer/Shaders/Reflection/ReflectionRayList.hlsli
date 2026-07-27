#ifndef REFLECTION_RAY_LIST_HLSL
#define REFLECTION_RAY_LIST_HLSL

// Ray list packing shared by the classifier and the ray generation shader.
// Layout matches FFX_DNSR / classifier: x in bits 0-14, y in bits 15-28,
// quad copy flags in bits 29-31.
uint PackReflectionRayCoords(uint2 rayCoord, bool copyHorizontal, bool copyVertical, bool copyDiagonal)
{
    uint rayX15Bit = rayCoord.x & 0b111111111111111;
    uint rayY14Bit = rayCoord.y & 0b11111111111111;
    uint copyHorizontal1Bit = copyHorizontal ? 1u : 0u;
    uint copyVertical1Bit = copyVertical ? 1u : 0u;
    uint copyDiagonal1Bit = copyDiagonal ? 1u : 0u;

    return (rayX15Bit << 0) | (rayY14Bit << 15) | (copyHorizontal1Bit << 29) | (copyVertical1Bit << 30) | (copyDiagonal1Bit << 31);
}

void UnpackReflectionRayCoords(uint packed, out uint2 rayCoord, out bool copyHorizontal, out bool copyVertical, out bool copyDiagonal)
{
    rayCoord.x = (packed >> 0) & 0b111111111111111;
    rayCoord.y = (packed >> 15) & 0b11111111111111;
    copyHorizontal = bool((packed >> 29) & 1u);
    copyVertical = bool((packed >> 30) & 1u);
    copyDiagonal = bool((packed >> 31) & 1u);
}

#endif // REFLECTION_RAY_LIST_HLSL
