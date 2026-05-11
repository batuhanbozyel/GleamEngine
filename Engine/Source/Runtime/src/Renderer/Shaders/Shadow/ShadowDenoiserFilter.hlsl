#define FFX_DNSR_SHADOWS_FILTER_PASS 1
#include "ffx_denoiser_shadows_callbacks.hlsli"
#include "ffx_denoiser_shadows_filter.hlsli"

[shader("compute")]
[numthreads(8, 8, 1)]
void shadowDenoiserFilter(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID, uint3 did : SV_DispatchThreadID)
{
    switch (dnsr.passIndex)
    {
        case 0:  DenoiserShadowsFilterPass0(gid.xy, gtid.xy, did.xy); break;
        case 1:  DenoiserShadowsFilterPass1(gid.xy, gtid.xy, did.xy); break;
        default: DenoiserShadowsFilterPass2(gid.xy, gtid.xy, did.xy); break;
    }
}
