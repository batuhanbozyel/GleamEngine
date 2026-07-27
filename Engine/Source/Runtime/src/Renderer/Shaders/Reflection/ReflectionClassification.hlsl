#define FFX_CLASSIFIER_OPTION_INVERTED_DEPTH 0

#include "ffx_classifier_reflections_callbacks.hlsli"
#include "ffx_classifier_reflections.hlsli"

[shader("compute")]
[numthreads(REFLECTION_DENOISER_TILE_SIZE, REFLECTION_DENOISER_TILE_SIZE, 1)]
void reflectionClassification(uint groupIndex : SV_GroupIndex, uint3 groupId : SV_GroupID)
{
    uint2 groupThreadId = ffxRemapForWaveReduction(groupIndex);
    uint2 dispatchThreadId = groupId.xy * REFLECTION_DENOISER_TILE_SIZE + groupThreadId;

    int2 screenSize = int2(camera.resolution);
    float3 worldSpaceNormal = LoadWorldSpaceNormal(int2(dispatchThreadId));
    float depth = GetInputDepth(dispatchThreadId);
    float3 viewSpaceSurfaceNormal = normalize(mul(ViewMatrix(), float4(worldSpaceNormal, 0.0)).xyz);
    float roughness = LoadRoughnessFromMaterialParametersInput(uint3(dispatchThreadId, 0));

    ClassifyTiles(dispatchThreadId,
                  groupThreadId,
                  roughness,
                  viewSpaceSurfaceNormal,
                  depth,
                  screenSize,
                  SamplesPerQuad(),
                  TemporalVarianceGuidedTracingEnabled(),
                  /* enable_hitcounter */ false,
                  /* enable_screen_space_tracing */ false,
                  /* enable_hw_ray_tracing */ true);

    StoreExtractedRoughness(dispatchThreadId, roughness);
}
