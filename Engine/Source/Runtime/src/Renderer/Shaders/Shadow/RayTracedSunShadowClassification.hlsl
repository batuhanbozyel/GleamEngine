#define FFX_CLASSIFIER_OPTION_INVERTED_DEPTH 0
#define FFX_CLASSIFIER_OPTION_CLASSIFIER_MODE 0

#include "ffx_classifier_shadows_callbacks.hlsli"
#include "ffx_classifier_shadows.hlsli"

[shader("compute")]
[numthreads(SHADOW_TILE_WIDTH * SHADOW_TILE_HEIGHT, 1, 1)]
void rayTracedSunShadowClassification(uint groupIndex : SV_GroupIndex, uint3 groupId : SV_GroupID)
{
    FfxClassifyShadows(groupIndex, groupId);
}
