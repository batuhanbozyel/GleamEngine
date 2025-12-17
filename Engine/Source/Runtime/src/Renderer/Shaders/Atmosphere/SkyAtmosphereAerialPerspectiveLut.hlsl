#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereAerialPerspectiveLUTShader

float4 IntegrateScatteredLuminance(in float3 WorldPos, in float3 WorldDir, in float3 SunDir, in float SampleCountIni, in float tMaxMax = 9000000.0f)
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
			return float4(0, 0, 0, 1);
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

	// Phase functions
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
		//t = tMax * (s + SampleSegmentT) / SampleCount;
		// Exact difference, important for accuracy of multiple scattering
		float NewT = tMax * (s + SampleSegmentT) / SampleCount;
		dt = NewT - t;
		t = NewT;
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
	return float4(L, 1.0f - dot(throughput, 1.0f / 3.0f));
}

[numthreads(SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_LUT_RES, SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_LUT_RES, 1)]
void skyAtmosphereAerialPerspectiveLUTShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / float(SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_LUT_RES);
	
	float3 ClipSpace = float3(uv * float2(2.0, -2.0) - float2(1.0, -1.0), 0.5);
	float4 HViewPos = mul(camera.invProjectionMatrix, float4(ClipSpace, 1.0));
	float3 WorldDir = normalize(mul((float3x3) camera.invViewMatrix, HViewPos.xyz / HViewPos.w));

	float earthR = atmosphereParams.bottomRadius;
	float3 camPos = camera.position + float3(0, earthR, 0);
	float3 SunDir = atmosphereUniforms.sunDirection;
	float3 SunLuminance = 0.0;

	float Slice = ((float(dispatchThreadId.z) + 0.5f) / AP_SLICE_COUNT);
	Slice *= Slice; // squared distribution
	Slice *= AP_SLICE_COUNT;
	
	// Compute position from froxel information
	float3 WorldPos = camPos;
	float tMax = AerialPerspectiveSliceToDepth(Slice);
	float3 newWorldPos = WorldPos + tMax * WorldDir;
	
	// If the voxel is under the ground, make sure to offset it out on the ground.
	float viewHeight = length(newWorldPos);
	if (viewHeight <= (atmosphereParams.bottomRadius + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET))
	{
		// Apply a position offset to make sure no artifact are visible close to the earth boundaries for large voxel.
		newWorldPos = normalize(newWorldPos) * (atmosphereParams.bottomRadius + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET + 0.001f);
		WorldDir = normalize(newWorldPos - camPos);
		tMax = length(newWorldPos - camPos);
	}
	float tMaxMax = tMax;

	RWTexture3D<float4> texture = ResourceDescriptorHeap[UAVIndex(atmosphereUniforms.aerialPerspectiveLutTexture)];
	if (viewHeight >= atmosphereParams.topRadius)
	{
		float3 prevWorlPos = WorldPos;
		if (!MoveToTopAtmosphere(WorldDir, atmosphereParams.topRadius, WorldPos))
		{
			// Ray is not intersecting the atmosphere
			texture[dispatchThreadId] = float4(0.0, 0.0, 0.0, 1.0);
		}
		float LengthToAtmosphere = length(prevWorlPos - WorldPos);
		if (tMaxMax < LengthToAtmosphere)
		{
			// tMaxMax for this voxel is not within earth atmosphere
			texture[dispatchThreadId] = float4(0.0, 0.0, 0.0, 1.0);
		}
		// Now world position has been moved to the atmosphere boundary: we need to reduce tMaxMax accordingly. 
		tMaxMax = max(0.0, tMaxMax - LengthToAtmosphere);
	}
	
	const float SampleCountIni = max(1.0, float(dispatchThreadId.z + 1.0) * 2.0f);
	float4 luminance = IntegrateScatteredLuminance(WorldPos, WorldDir, SunDir, SampleCountIni, tMaxMax);
	texture[dispatchThreadId] = luminance;
}