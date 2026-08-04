#define FFX_DNSR_REFLECTIONS_PREFILTER_PASS 1
#include "ffx_denoiser_reflections_callbacks.hlsli"
#include "ffx_denoiser_reflections_prefilter.hlsli"

[shader("compute")]
[numthreads(REFLECTION_DENOISER_TILE_SIZE, REFLECTION_DENOISER_TILE_SIZE, 1)]
void reflectionDenoiserPrefilter(uint groupIndex : SV_GroupIndex, uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    Prefilter(groupIndex, gid.x, (int2)gtid.xy);
}
