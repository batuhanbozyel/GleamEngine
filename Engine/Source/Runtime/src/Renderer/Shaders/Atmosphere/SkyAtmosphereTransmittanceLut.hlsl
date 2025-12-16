#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereTransmittanceLUTShader

float3 IntegrateTransmittance(in float3 WorldPos, in float3 WorldDir, in float SampleCountIni, in float tMaxMax = 9000000.0f)
{
	// Compute next intersection with atmosphere or ground 
	float3 earthO = float3(0.0f, 0.0f, 0.0f);
	float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, earthO, atmosphereParams.bottomRadius);
	float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, earthO, atmosphereParams.topRadius);
	float tMax = 0.0f;
	if (tBottom < 0.0f)
	{
		if (tTop < 0.0f)
		{
			tMax = 0.0f; // No intersection with earth nor atmosphere: stop right away  
			return 1.0f; // exp(-0.0f);
		}
		else
		{
			tMax = tTop;
		}
	}
	else
	{
		if (tTop > 0.0f)
		{
			tMax = min(tTop, tBottom);
		}
	}
	tMax = min(tMax, tMaxMax);

	// Sample count 
	float SampleCount = SampleCountIni;
	float dt = tMax / SampleCount;

	// Ray march the atmosphere to integrate optical depth
	float3 OpticalDepth = 0.0;
	float t = 0.0f;
	float tPrev = 0.0;
	const float SampleSegmentT = 0.3f;
	for (float s = 0.0f; s < SampleCount; s += 1.0f)
	{
		//t = tMax * (s + SampleSegmentT) / SampleCount;
		// Exact difference, important for accuracy of multiple scattering
		float NewT = tMax * (s + SampleSegmentT) / SampleCount;
		dt = NewT - t;
		t = NewT;
		float3 P = WorldPos + t * WorldDir;

		MediumSampleRGB medium = SampleMediumRGB(P);
		OpticalDepth += medium.extinction * dt;

		tPrev = t;
	}
	return exp(-OpticalDepth);
}

[numthreads(16, 16, 1)]
void skyAtmosphereTransmittanceLUTShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / float2(SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT);
	
	float viewHeight, viewZenithCosAngle;
	UvToLutTransmittanceParams(uv, viewHeight, viewZenithCosAngle);

	float3 rayOrigin = float3(0.0, viewHeight, 0.0);
	float3 rayDirection = float3(0.0, viewZenithCosAngle, sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle));
	
	const float sampleCountIni = 40.0f;	// Can go a low as 10 sample but energy lost starts to be visible.
	float3 transmittance = IntegrateTransmittance(rayOrigin, rayDirection, sampleCountIni);

	// Optical depth to transmittance
	RWTexture2D<float4> texture = ResourceDescriptorHeap[UAVIndex(atmosphereUniforms.transmittanceLutTexture)];
	texture[dispatchThreadId.xy] = float4(transmittance, 1.0f);
}