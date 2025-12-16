#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereSkyViewLUTShader

float3 IntegrateScatteredLuminance(in float3 WorldPos, in float3 WorldDir, in float3 SunDir, in float SampleCountIni, in float tMaxMax = 9000000.0f)
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
			return 0.0f;
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
	float SampleCountFloor = SampleCountIni;
	float tMaxFloor = tMax;
	{
		SampleCount = lerp(atmosphereUniforms.rayMarchMinMaxSPP.x, atmosphereUniforms.rayMarchMinMaxSPP.y, saturate(tMax * 0.01));
		SampleCountFloor = floor(SampleCount);
		tMaxFloor = tMax * SampleCountFloor / SampleCount; // rescale tMax to map to the last entire step segment.
	}
	float dt = tMax / SampleCount;

	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);
	const float3 wi = SunDir;
	const float3 wo = WorldDir;
	float cosTheta = dot(wi, wo);
	float MiePhaseValue = hgPhase(atmosphereParams.miePhaseG, -cosTheta); // mnegate cosTheta because due to WorldDir being a "in" direction. 
	float RayleighPhaseValue = RayleighPhase(cosTheta);
	float3 globalL = atmosphereUniforms.sunIlluminance;

	// Ray march the atmosphere to integrate optical depth
	float3 L = 0.0f;
	float3 throughput = 1.0;
	float t = 0.0f;
	const float SampleSegmentT = 0.3f;
	for (float s = 0.0f; s < SampleCount; s += 1.0f)
	{
		// More expenssive but artifact free
		float t0 = (s) / SampleCountFloor;
		float t1 = (s + 1.0f) / SampleCountFloor;
		// Non linear distribution of sample within the range.
		t0 = t0 * t0;
		t1 = t1 * t1;
		// Make t0 and t1 world space distances.
		t0 = tMaxFloor * t0;
		if (t1 > 1.0)
		{
			t1 = tMax;
			//	t1 = tMaxFloor;	// this reveal depth slices
		}
		else
		{
			t1 = tMaxFloor * t1;
		}
		//t = t0 + (t1 - t0) * (whangHashNoise(pixPos.x, pixPos.y, gFrameId * 1920 * 1080)); // With dithering required to hide some sampling artifact relying on TAA later? This may even allow volumetric shadow?
		t = t0 + (t1 - t0) * SampleSegmentT;
		dt = t1 - t0;
		float3 P = WorldPos + t * WorldDir;

		MediumSampleRGB medium = SampleMediumRGB(P);
		const float3 SampleOpticalDepth = medium.extinction * dt;
		const float3 SampleTransmittance = exp(-SampleOpticalDepth);
		
		float pHeight = length(P);
		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		float2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;

		float3 PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;

		// Earth shadow 
		float tEarth = RaySphereIntersectNearest(P, SunDir, earthO + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET * UpVector, atmosphereParams.bottomRadius);
		float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

		// Dual scattering for multi scattering 
		float3 multiScatteredLuminance = GetMultipleScattering(medium.scattering, medium.extinction, P, SunZenithCosAngle);
		float3 S = globalL * (earthShadow * TransmittanceToSun * PhaseTimesScattering + multiScatteredLuminance * medium.scattering);

		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		float3 Sint = (S - S * SampleTransmittance) / medium.extinction; // integrate along the current step segment 
		L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= SampleTransmittance;
	}
	
	if (tMax == tBottom && tBottom > 0.0)
	{
		// Account for bounced light off the earth
		float3 P = WorldPos + tBottom * WorldDir;
		float pHeight = length(P);

		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		float2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;

		const float NdotL = saturate(dot(normalize(UpVector), normalize(SunDir)));
		L += globalL * TransmittanceToSun * throughput * NdotL * atmosphereParams.groundAlbedo / PI;
	}
	return L;
}

[numthreads(16, 16, 1)]
void skyAtmosphereSkyViewLUTShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / float2(SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_WIDTH, SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_HEIGHT);
	
	float3 ClipSpace = float3(uv * float2(2.0, -2.0) - float2(1.0, -1.0), 1.0);
	float4 HViewPos = mul(camera.invProjectionMatrix, float4(ClipSpace, 1.0));
	float3 WorldDir = normalize(mul((float3x3) camera.invViewMatrix, HViewPos.xyz / HViewPos.w));
	float3 WorldPos = camera.position + float3(0, atmosphereParams.bottomRadius, 0);
	
	float viewHeight = length(WorldPos);

	float viewZenithCosAngle;
	float lightViewCosAngle;
	UvToSkyViewLutParams(uv, viewHeight, viewZenithCosAngle, lightViewCosAngle);
	
	float3 SunDir;
	{
		float3 UpVector = WorldPos / viewHeight;
		float sunZenithCosAngle = dot(UpVector, atmosphereUniforms.sunDirection);
		SunDir = normalize(float3(sqrt(1.0 - sunZenithCosAngle * sunZenithCosAngle), sunZenithCosAngle, 0.0));
	}
	WorldPos = float3(0.0f, viewHeight, 0.0f);

	float viewZenithSinAngle = sqrt(1 - viewZenithCosAngle * viewZenithCosAngle);
	WorldDir = float3(
		viewZenithSinAngle * lightViewCosAngle,
		viewZenithCosAngle,
		viewZenithSinAngle * sqrt(1.0 - lightViewCosAngle * lightViewCosAngle));
	
	RWTexture2D<float3> texture = ResourceDescriptorHeap[UAVIndex(atmosphereUniforms.skyViewLutTexture)];
	if (!MoveToTopAtmosphere(WorldDir, atmosphereParams.topRadius, WorldPos))
	{
		// Ray is not intersecting the atmosphere
		texture[dispatchThreadId.xy] = 0.0;
	}
	
	const float SampleCountIni = 30;
	texture[dispatchThreadId.xy] = IntegrateScatteredLuminance(WorldPos, WorldDir, SunDir, SampleCountIni);
}