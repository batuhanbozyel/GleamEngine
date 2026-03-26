#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::GenerateCubemapMipsConstants, constants);

[shader("compute")]
[numthreads(16, 16, 1)]
void generateCubemapMipsShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (any(dispatchThreadId.xy >= constants.resolution))
    {
        return;
    }

	RWTexture2DArray<float4> srcTexture = ResourceDescriptorHeap[constants.sourceTexture];
	RWTexture2DArray<float4> dstTexture = ResourceDescriptorHeap[constants.targetTexture];

	uint srcResolution = constants.resolution << 1;
	uint3 srcCoord = uint3(dispatchThreadId.xy << 1, constants.face);

	float4 c0 = srcTexture[min(srcCoord + uint3(0, 0, 0), srcResolution - 1)];
	float4 c1 = srcTexture[min(srcCoord + uint3(1, 0, 0), srcResolution - 1)];
	float4 c2 = srcTexture[min(srcCoord + uint3(0, 1, 0), srcResolution - 1)];
	float4 c3 = srcTexture[min(srcCoord + uint3(1, 1, 0), srcResolution - 1)];

	dstTexture[uint3(dispatchThreadId.xy, constants.face)] = (c0 + c1 + c2 + c3) * 0.25;
}