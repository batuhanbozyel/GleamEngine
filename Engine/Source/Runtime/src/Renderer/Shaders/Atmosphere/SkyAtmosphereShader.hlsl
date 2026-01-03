#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereRenderShader

PUSH_CONSTANT(Gleam::SkyAtmosphereRenderConstants, constants);

static RWTexture2D<float4> TargetTexture = ResourceDescriptorHeap[UAVIndex(constants.targetTexture)];
static Texture2D<float> DepthTexture = ResourceDescriptorHeap[SRVIndex(constants.depthTexture)];

[numthreads(16, 16, 1)]
void skyAtmosphereRenderShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
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
	
	float3 Luminance = 0.0;
	ClipSpace.z = DepthTexture[dispatchThreadId.xy];
	if (ClipSpace.z >= (1.0f - FLT_EPSILON))
	{
		Luminance += GetSkyLuminance(ClipSpace, WorldPos, WorldDir);
		Luminance += GetSunLuminance(WorldPos, WorldDir);
		
		float4 sceneColor = TargetTexture[dispatchThreadId.xy];
		TargetTexture[dispatchThreadId.xy] = float4(sceneColor.rgb * sceneColor.a + Luminance.rgb * (1.0 - sceneColor.a), 1.0);
	}
}