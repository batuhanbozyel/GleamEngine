#include "Common.hlsli"
#include "ShaderTypes.h"
#include "ffx_denoiser_shadows_util.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
PUSH_CONSTANT(Gleam::RayTracedSunShadowResolveConstants, constants);

[shader("compute")]
[numthreads(8, 8, 1)]
void rayTracedSunShadowResolve(uint3 did : SV_DispatchThreadID)
{
    const uint2 resolution = uint2(camera.resolution);
    if (any(did.xy >= resolution))
    {
        return;
    }

    ByteAddressBuffer hitMaskBuffer = ResourceDescriptorHeap[constants.hitMaskResults];
    const uint2 tileCoord  = FFX_DNSR_Shadows_GetTileIndexFromPixelPosition(did.xy);
    const uint  tileIndex  = FFX_DNSR_Shadows_LinearTileIndex(tileCoord, resolution.x);
    const uint  bitMask    = FFX_DNSR_Shadows_GetBitMaskFromPixelPosition(did.xy);
    const uint  shadowTile = hitMaskBuffer.Load<uint>(tileIndex * sizeof(uint));

    RWTexture2D<unorm float> shadowMask = ResourceDescriptorHeap[constants.shadowMaskOutput];
    shadowMask[did.xy] = (shadowTile & bitMask) != 0u ? 1.0f : 0.0f;
}
