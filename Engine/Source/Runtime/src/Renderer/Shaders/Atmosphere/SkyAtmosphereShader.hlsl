#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereRenderShader

PUSH_CONSTANT(Gleam::SkyAtmosphereRenderConstants, constants);

[numthreads(16, 16, 1)]
void skyAtmosphereRenderShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (constants.depthTexture != InvalidResourceIndex)
	{
		Texture2D<float> DepthTexture = ResourceDescriptorHeap[constants.depthTexture];
		float depth = DepthTexture[dispatchThreadId.xy];
		if (depth < (1.0f - FLT_EPSILON))
		{
			return;
		}
	}
	
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / camera.resolution;
	
	float3 ClipSpace = float3(uv * float2(2.0, -2.0) - float2(1.0, -1.0), 1.0);
	float4 HViewPos = mul(camera.invProjectionMatrix, float4(ClipSpace, 1.0));
	float3 WorldDir = normalize(mul((float3x3)camera.invViewMatrix, HViewPos.xyz / HViewPos.w));
	float3 WorldPos = GetCameraPlanetPos(camera.position);
	
	float viewHeight = length(WorldPos);
	if (!MoveToTopAtmosphere(WorldDir, atmosphereParams.topRadius, WorldPos))
	{
		// Ray is not intersecting the atmosphere
		return;
	}
	
	float3 Luminance = GetSkyLuminance(WorldPos, WorldDir);
	if (constants.renderSun)
	{
		Luminance += GetSunLuminance(WorldPos, WorldDir);
	}
	
	RWTexture2D<float4> TargetTexture = ResourceDescriptorHeap[constants.targetTexture];
	TargetTexture[dispatchThreadId.xy] = float4(Luminance, 1.0);
}