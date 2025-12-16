#pragma once
#include "SkyAtmosphereDefinitions.h"
#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, SKY_ATMOSPHERE_CAMERA_UNIFORMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereParameters, atmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereCommonUniforms, atmosphereUniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

static Texture2D<float4> TransmittanceLutTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.transmittanceLutTexture)];
static Texture2D<float3> MultiScatterTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.multiScatterLutTexture)];
static Texture2D<float4> SkyViewLutTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.skyViewLutTexture)];
static Texture2D<float> DepthTexture = ResourceDescriptorHeap[SRVIndex(atmosphereUniforms.depthTexture)];

float FromUnitToSubUvs(float u, float resolution) { return (u + 0.5f / resolution) * (resolution / (resolution + 1.0f)); }
float FromSubUvsToUnit(float u, float resolution) { return (u - 0.5f / resolution) * (resolution / (resolution - 1.0f)); }

float GetAlbedo(float scattering, float extinction) { return scattering / max(0.001, extinction); }
float3 GetAlbedo(float3 scattering, float3 extinction) { return scattering / max(0.001, extinction); }

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
	//uv = float2(FromUnitToSubUvs(uv.x, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH), FromUnitToSubUvs(uv.y, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT));
}

#define NONLINEARSKYVIEWLUT 1
void UvToSkyViewLutParams(in float2 uv, in float viewHeight, out float viewZenithCosAngle, out float lightViewCosAngle)
{
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	uv = float2(FromSubUvsToUnit(uv.x, float(SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_WIDTH)), FromSubUvsToUnit(uv.y, float(SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_HEIGHT)));

	float Vhorizon = sqrt(viewHeight * viewHeight - atmosphereParams.bottomRadius * atmosphereParams.bottomRadius);
	float CosBeta = Vhorizon / viewHeight;				// GroundToHorizonCos
	float Beta = acos(CosBeta);
	float ZenithHorizonAngle = PI - Beta;

	if (uv.y < 0.5f)
	{
		float coord = 2.0*uv.y;
		coord = 1.0 - coord;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		coord = 1.0 - coord;
		viewZenithCosAngle = cos(ZenithHorizonAngle * coord);
	}
	else
	{
		float coord = uv.y*2.0 - 1.0;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		viewZenithCosAngle = cos(ZenithHorizonAngle + Beta * coord);
	}

	float coord = uv.x;
	coord *= coord;
	lightViewCosAngle = -(coord*2.0 - 1.0);
}

bool MoveToTopAtmosphere(in float3 WorldDir, in float AtmosphereTopRadius, inout float3 WorldPos)
{
	float viewHeight = length(WorldPos);
	if (viewHeight > AtmosphereTopRadius)
	{
		float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, float3(0.0f, 0.0f, 0.0f), AtmosphereTopRadius);
		if (tTop >= 0.0f)
		{
			float3 UpVector = WorldPos / viewHeight;
			float3 UpOffset = UpVector * -SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET;
			WorldPos = WorldPos + WorldDir * tTop + UpOffset;
		}
		else
		{
			// Ray is not intersecting the atmosphere
			return false;
		}
	}
	return true; // ok to start tracing
}

float3 GetSunLuminance(float3 WorldPos, float3 WorldDir)
{
	if (dot(WorldDir, atmosphereUniforms.sunDirection) > cos(0.5*0.505*3.14159 / 180.0))
	{
		float t = RaySphereIntersectNearest(WorldPos, WorldDir, float3(0.0f, 0.0f, 0.0f), atmosphereParams.bottomRadius);
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
	return MultiScatterTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0);
}

float GetShadow(float3 P)
{
#if SKY_ATMOSPHERE_SHADOWMAP_ENABLED
	// First evaluate opaque shadow
	float4 shadowUv = mul(gShadowmapViewProjMat, float4(P + float3(0.0, -atmosphereParams.bottomRadiusm, 0.0), 1.0));
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