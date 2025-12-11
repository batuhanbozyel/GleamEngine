#define SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS
#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereTransmittanceLUTShader

[numthreads(16, 16, 1)]
void skyAtmosphereTransmittanceLUTShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float2 position = dispatchThreadId.xy + 0.5;
	float2 uv = position / float2(SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT);
	
	float viewHeight, viewZenithCosAngle;
	UvToLutTransmittanceParams(uv, viewHeight, viewZenithCosAngle);

	float3 rayOrigin = float3(0.0, 0.0, viewHeight);
	float3 rayDirection = float3(0.0, sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle), viewZenithCosAngle);

	const bool ground = false;
	const float sampleCountIni = 40.0f;	// Can go a low as 10 sample but energy lost starts to be visible.
	const float depthBufferValue = -1.0;
	const bool variableSampleCount = false;
	const bool mieRayPhase = false;
	float3 transmittance = exp(-IntegrateScatteredLuminance(position, rayOrigin, rayDirection, ground, sampleCountIni, depthBufferValue, variableSampleCount, mieRayPhase).OpticalDepth);

	// Optical depth to transmittance
	RWTexture2D<float4> texture = ResourceDescriptorHeap[UAVIndex(atmosphereUniforms.transmittanceLutTexture)];
	texture[dispatchThreadId.xy] = float4(transmittance, 1.0f);
}