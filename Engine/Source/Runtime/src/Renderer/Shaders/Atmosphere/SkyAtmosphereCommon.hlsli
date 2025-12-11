#pragma once
#include "SkyAtmosphereDefinitions.h"
#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::CameraUniforms, cameraUniforms, SKY_ATMOSPHERE_CAMERA_UNIFORMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereParameters, atmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereCommonUniforms, atmosphereUniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

static Texture2D<float4> TransmittanceLutTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.transmittanceLutTexture)];
static Texture2D<float4> MultiScatterTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.multiScatterTexture)];
static Texture2D<float4> SkyViewLutTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.skyViewLutTexture)];
static Texture2D<float> DepthTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.depthTexture)];

float FromUnitToSubUvs(float u, float resolution) { return (u + 0.5f / resolution) * (resolution / (resolution + 1.0f)); }
float FromSubUvsToUnit(float u, float resolution) { return (u - 0.5f / resolution) * (resolution / (resolution - 1.0f)); }

float GetAlbedo(float scattering, float extinction) { return scattering / max(0.001, extinction); }
float3 GetAlbedo(float3 scattering, float3 extinction) { return scattering / max(0.001, extinction);}

struct MediumSampleRGB
{
	float3 scattering;
	float3 absorption;
	float3 extinction;

	float3 scatteringMie;
	float3 absorptionMie;
	float3 extinctionMie;

	float3 scatteringRay;
	float3 absorptionRay;
	float3 extinctionRay;

	float3 scatteringOzo;
	float3 absorptionOzo;
	float3 extinctionOzo;

	float3 albedo;
};

MediumSampleRGB SampleMediumRGB(in float3 WorldPos)
{
	const float viewHeight = length(WorldPos) - atmosphereParams.bottomRadius;

	const float densityMie = exp(atmosphereParams.mieDensityExpScale * viewHeight);
	const float densityRay = exp(atmosphereParams.rayleighDensityExpScale * viewHeight);
	const float densityOzo = saturate(viewHeight < atmosphereParams.absorptionDensity0LayerWidth ?
		atmosphereParams.absorptionDensity0LinearTerm * viewHeight + atmosphereParams.absorptionDensity0ConstantTerm :
		atmosphereParams.absorptionDensity1LinearTerm * viewHeight + atmosphereParams.absorptionDensity1ConstantTerm);

	MediumSampleRGB s;

	s.scatteringMie = densityMie * atmosphereParams.mieScattering;
	s.absorptionMie = densityMie * atmosphereParams.mieAbsorption;
	s.extinctionMie = densityMie * atmosphereParams.mieExtinction;

	s.scatteringRay = densityRay * atmosphereParams.rayleighScattering;
	s.absorptionRay = 0.0f;
	s.extinctionRay = s.scatteringRay + s.absorptionRay;

	s.scatteringOzo = 0.0;
	s.absorptionOzo = densityOzo * atmosphereParams.absorptionExtinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;

	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = GetAlbedo(s.scattering, s.extinction);

	return s;
}

float RayleighPhase(float cosTheta)
{
	float factor = 3.0f / (16.0f * PI);
	return factor * (1.0f + cosTheta * cosTheta);
}

#define USE_CornetteShanks
float CornetteShanksMiePhaseFunction(float g, float cosTheta)
{
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + cosTheta * cosTheta) / pow(1.0 + g * g - 2.0 * g * -cosTheta, 1.5);
}

float hgPhase(float g, float cosTheta)
{
#ifdef USE_CornetteShanks
	return CornetteShanksMiePhaseFunction(g, cosTheta);
#else
	// Reference implementation (i.e. not schlick approximation). 
	// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
	float numer = 1.0f - g * g;
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	return numer / (4.0f * PI * denom * sqrt(denom));
#endif
}

void UvToLutTransmittanceParams(in float2 uv, out float viewHeight, out float viewZenithCosAngle)
{
	float x_mu = uv.x;
	float x_r = uv.y;

	float H = sqrt(atmosphereParams.topRadius * atmosphereParams.topRadius - atmosphereParams.bottomRadius * atmosphereParams.bottomRadius);
	float rho = H * x_r;
	viewHeight = sqrt(rho * rho + atmosphereParams.bottomRadius * atmosphereParams.bottomRadius);

	float d_min = atmosphereParams.topRadius - viewHeight;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	viewZenithCosAngle = d == 0.0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0 * viewHeight * d);
	viewZenithCosAngle = clamp(viewZenithCosAngle, -1.0, 1.0);
}

void LutTransmittanceParamsToUv(in float viewHeight, in float viewZenithCosAngle, out float2 uv)
{
	float H = sqrt(max(0.0f, atmosphereParams.topRadius * atmosphereParams.topRadius - atmosphereParams.bottomRadius * atmosphereParams.bottomRadius));
	float rho = sqrt(max(0.0f, viewHeight * viewHeight - atmosphereParams.bottomRadius * atmosphereParams.bottomRadius));

	float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0) + atmosphereParams.topRadius * atmosphereParams.topRadius;
	float d = max(0.0, (-viewHeight * viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary

	float d_min = atmosphereParams.topRadius - viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	uv = float2(x_mu, x_r);
	//uv = float2(FromUnitToSubUvs(uv.x, TRANSMITTANCE_TEXTURE_WIDTH), fromUnitToSubUvs(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT)); // No real impact so off
}

float3 GetSunLuminance(float3 WorldPos, float3 WorldDir, float PlanetRadius)
{
	if (dot(WorldDir, atmosphereUniforms.sunDirection) > cos(0.5*0.505*3.14159 / 180.0))
	{
		float t = RaySphereIntersectNearest(WorldPos, WorldDir, float3(0.0f, 0.0f, 0.0f), PlanetRadius);
		if (t < 0.0f) // no intersection
		{
			const float3 SunLuminance = 1000000.0; // arbitrary. But fine, not use when comparing the models
			return SunLuminance;
		}
	}
	return 0;
}

float3 GetMultipleScattering(float3 scattering, float3 extinction, float3 worlPos, float viewZenithCosAngle)
{
	float2 uv = saturate(float2(viewZenithCosAngle * 0.5f + 0.5f, (length(worlPos) - atmosphereParams.bottomRadius) / (atmosphereParams.topRadius - atmosphereParams.bottomRadius)));
	uv = float2(FromUnitToSubUvs(uv.x, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES), FromUnitToSubUvs(uv.y, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES));

	float3 multiScatteredLuminance = MultiScatterTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;
	return multiScatteredLuminance;
}

float GetShadow(float3 P)
{
#if SKY_ATMOSPHERE_SHADOWMAP_ENABLED
	// First evaluate opaque shadow
	float4 shadowUv = mul(gShadowmapViewProjMat, float4(P + float3(0.0, 0.0, -atmosphereParams.bottomRadius), 1.0));
	//shadowUv /= shadowUv.w;	// not be needed as it is an ortho projection
	shadowUv.x = shadowUv.x*0.5 + 0.5;
	shadowUv.y = -shadowUv.y*0.5 + 0.5;
	if (all(shadowUv.xyz >= 0.0) && all(shadowUv.xyz < 1.0))
	{
		return ShadowmapTexture.SampleCmpLevelZero(samplerShadow, shadowUv.xy, shadowUv.z);
	}
#endif
	return 1.0f;
}

struct SingleScatteringResult
{
	float3 L;						// Scattered light (luminance)
	float3 OpticalDepth;			// Optical depth (1/m)
	float3 Transmittance;			// Transmittance in [0,1] (unitless)
	float3 MultiScatAs1;

	float3 NewMultiScatStep0Out;
	float3 NewMultiScatStep1Out;
};

SingleScatteringResult IntegrateScatteredLuminance(
	in float2 pixPos, in float3 WorldPos, in float3 WorldDir,
	in bool ground, in float SampleCountIni, in float DepthBufferValue, in bool VariableSampleCount,
	in bool MieRayPhase, in float tMaxMax = 9000000.0f)
{
	SingleScatteringResult result = (SingleScatteringResult)0;

	float3 ClipSpace = float3((pixPos / cameraUniforms.resolution) * float2(2.0, -2.0) - float2(1.0, -1.0), 1.0);

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
			return result;
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

	if (DepthBufferValue >= 0.0f)
	{
		ClipSpace.z = DepthBufferValue;
		if (ClipSpace.z < 1.0f)
		{
			float4 DepthBufferWorldPos = mul(cameraUniforms.invViewProjectionMatrix, float4(ClipSpace, 1.0));
			DepthBufferWorldPos /= DepthBufferWorldPos.w;

			float tDepth = length(DepthBufferWorldPos.xyz - (WorldPos + float3(0.0, 0.0, -atmosphereParams.bottomRadius))); // apply earth offset to go back to origin as top of earth mode. 
			if (tDepth < tMax)
			{
				tMax = tDepth;
			}
		}
	}
	tMax = min(tMax, tMaxMax);

	// Sample count 
	float SampleCount = SampleCountIni;
	float SampleCountFloor = SampleCountIni;
	float tMaxFloor = tMax;
	if (VariableSampleCount)
	{
		SampleCount = lerp(atmosphereUniforms.rayMarchMinMaxSPP.x, atmosphereUniforms.rayMarchMinMaxSPP.y, saturate(tMax * 0.01));
		SampleCountFloor = floor(SampleCount);
		tMaxFloor = tMax * SampleCountFloor / SampleCount;	// rescale tMax to map to the last entire step segment.
	}
	float dt = tMax / SampleCount;

	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);
	const float3 wi = atmosphereUniforms.sunDirection;
	const float3 wo = WorldDir;
	float cosTheta = dot(wi, wo);
	float MiePhaseValue = hgPhase(atmosphereParams.miePhaseG, -cosTheta);	// mnegate cosTheta because due to WorldDir being a "in" direction. 
	float RayleighPhaseValue = RayleighPhase(cosTheta);

#ifdef SKY_ATMOSPHERE_ILLUMINANCE_IS_ONE
	// When building the scattering factor, we assume light illuminance is 1 to compute a transfert function relative to identity illuminance of 1.
	// This make the scattering factor independent of the light. It is now only linked to the atmosphere properties.
	float3 globalL = 1.0f;
#else
	float3 globalL = atmosphereUniforms.sunIlluminance;
#endif

	// Ray march the atmosphere to integrate optical depth
	float3 L = 0.0f;
	float3 throughput = 1.0;
	float3 OpticalDepth = 0.0;
	float t = 0.0f;
	float tPrev = 0.0;
	const float SampleSegmentT = 0.3f;
	for (float s = 0.0f; s < SampleCount; s += 1.0f)
	{
		if (VariableSampleCount)
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
			t = t0 + (t1 - t0)*SampleSegmentT;
			dt = t1 - t0;
		}
		else
		{
			//t = tMax * (s + SampleSegmentT) / SampleCount;
			// Exact difference, important for accuracy of multiple scattering
			float NewT = tMax * (s + SampleSegmentT) / SampleCount;
			dt = NewT - t;
			t = NewT;
		}
		float3 P = WorldPos + t * WorldDir;

		MediumSampleRGB medium = SampleMediumRGB(P);
		const float3 SampleOpticalDepth = medium.extinction * dt;
		const float3 SampleTransmittance = exp(-SampleOpticalDepth);
		OpticalDepth += SampleOpticalDepth;

#ifndef SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS
		float pHeight = length(P);
		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(atmosphereUniforms.sunDirection, UpVector);
		float2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;

		float3 PhaseTimesScattering;
		if (MieRayPhase)
		{
			PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;
		}
		else
		{
			PhaseTimesScattering = medium.scattering * uniformPhase;
		}

		// Earth shadow 
		float tEarth = RaySphereIntersectNearest(P, atmosphereUniforms.sunDirection, earthO + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET * UpVector, atmosphereParams.bottomRadius);
		float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

		// Dual scattering for multi scattering 
		float3 multiScatteredLuminance = 0.0f;
#if SKY_ATMOSPHERE_MULTISCATAPPROX_ENABLED
		multiScatteredLuminance = GetMultipleScattering(medium.scattering, medium.extinction, P, SunZenithCosAngle);
#endif

		float shadow = 1.0f;
#if SKY_ATMOSPHERE_SHADOWMAP_ENABLED
		// First evaluate opaque shadow
		shadow = GetShadow(P);
#endif

		float3 S = globalL * (earthShadow * shadow * TransmittanceToSun * PhaseTimesScattering + multiScatteredLuminance * medium.scattering);

		// When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge.
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
		// The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.
#define MULTI_SCATTERING_POWER_SERIE 1
#if MULTI_SCATTERING_POWER_SERIE==0
		// 1 is the integration of luminance over the 4pi of a sphere, and assuming an isotropic phase function of 1.0/(4*PI)
		result.MultiScatAs1 += throughput * medium.scattering * 1 * dt;
#else
		float3 MS = medium.scattering * 1;
		float3 MSint = (MS - MS * SampleTransmittance) / medium.extinction;
		result.MultiScatAs1 += throughput * MSint;
#endif

		// Evaluate input to multi scattering 
		{
			float3 newMS;

			newMS = earthShadow * TransmittanceToSun * medium.scattering * uniformPhase * 1;
			result.NewMultiScatStep0Out += throughput * (newMS - newMS * SampleTransmittance) / medium.extinction;

			newMS = medium.scattering * uniformPhase * multiScatteredLuminance;
			result.NewMultiScatStep1Out += throughput * (newMS - newMS * SampleTransmittance) / medium.extinction;
		}

		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		float3 Sint = (S - S * SampleTransmittance) / medium.extinction;	// integrate along the current step segment 
		L += throughput * Sint;														// accumulate and also take into account the transmittance from previous steps
		throughput *= SampleTransmittance;
#endif

		tPrev = t;
	}

#ifndef SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS
	if (ground && tMax == tBottom && tBottom > 0.0)
	{
		// Account for bounced light off the earth
		float3 P = WorldPos + tBottom * WorldDir;
		float pHeight = length(P);

		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(atmosphereUniforms.sunDirection, UpVector);
		float2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;

		const float NdotL = saturate(dot(normalize(UpVector), normalize(atmosphereUniforms.sunDirection)));
		L += globalL * TransmittanceToSun * throughput * NdotL * atmosphereParams.groundAlbedo / PI;
	}
#endif

	result.L = L;
	result.OpticalDepth = OpticalDepth;
	result.Transmittance = throughput;
	return result;
}