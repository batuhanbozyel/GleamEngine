#define FFX_DNSR_REFLECTIONS_RESOLVE_TEMPORAL_PASS 1
#include "ffx_denoiser_reflections_callbacks.hlsli"
#include "ffx_denoiser_reflections_resolve_temporal.hlsli"

[shader("compute")]
[numthreads(REFLECTION_DENOISER_TILE_SIZE, REFLECTION_DENOISER_TILE_SIZE, 1)]
void reflectionDenoiserResolveTemporal(uint groupIndex : SV_GroupIndex, uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    ResolveTemporal(groupIndex, gid.x, gtid.xy);
}
