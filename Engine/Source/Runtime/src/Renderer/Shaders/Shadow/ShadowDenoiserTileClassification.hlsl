#define FFX_DNSR_SHADOWS_TILECLASSIFICATION_PASS 1
#include "ffx_denoiser_shadows_callbacks.hlsli"
#include "ffx_denoiser_shadows_tileclassification.hlsli"

[shader("compute")]
[numthreads(64, 1, 1)]
void shadowDenoiserTileClassification(uint groupIndex : SV_GroupIndex, uint3 gid : SV_GroupID)
{
    FFX_DNSR_Shadows_TileClassification(groupIndex, gid.xy);
}
