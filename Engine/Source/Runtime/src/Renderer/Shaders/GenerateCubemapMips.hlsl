#include "Common.hlsli"
#include "ShaderTypes.h"

#pragma compute generateCubemapMipsShader

PUSH_CONSTANT(Gleam::GenerateCubemapMipsConstants, constants);

[numthreads(16, 16, 1)]
void generateCubemapMipsShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (any(dispatchThreadId.xy >= constants.resolution))
    {
        return;
    }

    TextureCube<float4> srcTexture = ResourceDescriptorHeap[constants.srcTexture];
	RWTexture2DArray<float4> dstTexture = ResourceDescriptorHeap[constants.targetTexture];

    float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / constants.resolution;

    float3 direction = GetCubemapDirection(uv, dispatchThreadId.z);
    float4 color = srcTexture.SampleLevel(Sampler_Trilinear_Clamp, direction, constants.level - 1);
    dstTexture[uint3(dispatchThreadId.xy, dispatchThreadId.z + constants.level * 6)] = color;
}