#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
PUSH_CONSTANT(Gleam::ReflectionDenoiserStoreHistoryConstants, constants);

[shader("compute")]
[numthreads(REFLECTION_DENOISER_TILE_SIZE, REFLECTION_DENOISER_TILE_SIZE, 1)]
void reflectionDenoiserStoreHistory(uint3 did : SV_DispatchThreadID)
{
    if (any(did.xy >= (uint2)camera.resolution))
    {
        return;
    }

    Texture2D<float2> normalTexture = ResourceDescriptorHeap[constants.normalTexture];
    Texture2D<float> roughnessTexture = ResourceDescriptorHeap[constants.roughnessTexture];
    RWTexture2D<float2> normalHistoryTexture = ResourceDescriptorHeap[constants.normalHistoryTexture];
    RWTexture2D<float> roughnessHistoryTexture = ResourceDescriptorHeap[constants.roughnessHistoryTexture];

    normalHistoryTexture[did.xy] = normalTexture.Load(int3(did.xy, 0));
    roughnessHistoryTexture[did.xy] = roughnessTexture.Load(int3(did.xy, 0));
}
