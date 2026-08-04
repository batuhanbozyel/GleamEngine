#define FFX_DNSR_REFLECTIONS_REPROJECT_PASS 1
#include "ffx_denoiser_reflections_callbacks.hlsli"
#include "ffx_denoiser_reflections_reproject.hlsli"

[shader("compute")]
[numthreads(REFLECTION_DENOISER_TILE_SIZE, REFLECTION_DENOISER_TILE_SIZE, 1)]
void reflectionDenoiserReproject(uint groupIndex : SV_GroupIndex, uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    Reproject(groupIndex, gid.x, gtid.xy);
}
