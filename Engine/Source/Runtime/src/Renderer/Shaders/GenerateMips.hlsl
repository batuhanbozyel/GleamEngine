#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::GenerateMipsConstants, constants);

[shader("compute")]
[numthreads(16, 16, 1)]
void generateMipsShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (any(dispatchThreadId.xy >= constants.resolution))
    {
        return;
    }

	RWTexture2D<float4> srcTexture = ResourceDescriptorHeap[constants.sourceTexture];
	RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[constants.targetTexture];

	uint srcResolution = constants.resolution << 1;
	uint2 srcCoord = dispatchThreadId.xy << 1;

	float4 c0 = srcTexture[min(srcCoord + uint2(0, 0), srcResolution - 1)];
	float4 c1 = srcTexture[min(srcCoord + uint2(1, 0), srcResolution - 1)];
	float4 c2 = srcTexture[min(srcCoord + uint2(0, 1), srcResolution - 1)];
	float4 c3 = srcTexture[min(srcCoord + uint2(1, 1), srcResolution - 1)];

	dstTexture[dispatchThreadId.xy] = (c0 + c1 + c2 + c3) * 0.25;
}