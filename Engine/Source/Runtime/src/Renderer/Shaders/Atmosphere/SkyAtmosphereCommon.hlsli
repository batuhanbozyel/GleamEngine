#ifndef SKY_ATMOSPHERE_COMMON_HLSL
#define SKY_ATMOSPHERE_COMMON_HLSL

#include "SkyAtmosphereDefinitions.h"
#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereParameters, atmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereUniforms, atmosphereUniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

static Texture2D<float4> TransmittanceLutTexture = ResourceDescriptorHeap[atmosphereUniforms.transmittanceLutTexture];
static Texture2D<float3> MultiScatterTexture = ResourceDescriptorHeap[atmosphereUniforms.multiScatterLutTexture];

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

float3 GetSkyWorldCameraOrigin(float3 cameraPosition)
{
	const float3 cameraPositionInKM = cameraPosition * M_TO_KM;
	const float3 planetCenterWorld = float3(0.0f, -atmosphereParams.bottomRadius, 0.0f);
	const float bottomRadiusWorldOffset = atmosphereParams.bottomRadius + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET;
	
	const float3 planetCenterToCameraWorld = cameraPositionInKM - planetCenterWorld;
	const float distanceToPlanetCenterWorld = length(planetCenterToCameraWorld);
	const float3 planetCenterToCameraWorldNormalized = planetCenterToCameraWorld / distanceToPlanetCenterWorld;
	
	// If the camera is below the planet surface, we snap it back onto the surface.
	// This is to make sure the sky is always visible even if the camera is inside the virtual planet.
	return distanceToPlanetCenterWorld < bottomRadiusWorldOffset ?
	planetCenterWorld + bottomRadiusWorldOffset * planetCenterToCameraWorldNormalized : cameraPositionInKM;
}

float3 GetCameraPlanetPos(float3 cameraPosition)
{
	const float3 planetCenterWorld = float3(0.0f, -atmosphereParams.bottomRadius, 0.0f);
	const float3 skyWorldCameraOrigin = GetSkyWorldCameraOrigin(cameraPosition);
	return (skyWorldCameraOrigin - planetCenterWorld);
}

float3 GetTransmittance(float pHeight, float sunZenithCosAngle)
{
	float2 uv;
	LutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uv);
	return TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;
}

float3 GetAtmosphereTransmittance(float3 worldPosition, float3 worldDirection)
{
	// If the worldDirection is occluded from this virtual planet, then return.
	// We do this due to the low resolution LUT, where the stored zenith to horizon never reaches black, to prevent linear interpolation artefacts.
	// At the most shadowed point of the LUT, pure black with earth shadow is never reached.
	float2 sol = RaySphereIntersect(worldPosition, worldDirection, 0, atmosphereParams.bottomRadius);
	if (sol.x > 0 || sol.y > 0)
	{
		return 0;
	}
	
	float pHeight = length(worldPosition);
	const float3 UpVector = worldPosition / pHeight;
	float SunZenithCosAngle = dot(worldDirection, UpVector);
	return GetTransmittance(pHeight, SunZenithCosAngle);
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

float ComputeHorizonCos(float viewHeight)
{
	float h = max(viewHeight, atmosphereParams.bottomRadius + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET);
	float r = atmosphereParams.bottomRadius;
	return sqrt(saturate(1.0f - (r * r) / (h * h)));
}

float3 ClampToHorizon(float3 dir, float3 up, float horizonCos)
{
    float3 lateral = normalize(dir - dot(dir, up) * up);
    float horizonSin = sqrt(saturate(1.0 - horizonCos * horizonCos));
    return normalize(lateral * horizonSin + up * horizonCos);
}

float3 GetSkyLuminance(in float3 WorldPos, in float3 WorldDir, in float tMaxMax = 9000000.0f)
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
			return 0.0f; // No intersection with earth nor atmosphere: stop right away 
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
	float SampleCount = 0.0f;
	float SampleCountFloor = 0.0f;
	float tMaxFloor = tMax;
	{
		SampleCount = lerp(SKY_ATMOSPHERE_RAY_MARCH_MIN_SPP, SKY_ATMOSPHERE_RAY_MARCH_MAX_SPP, saturate(tMax * 0.01));
		SampleCountFloor = floor(SampleCount);
		tMaxFloor = tMax * SampleCountFloor / SampleCount; // rescale tMax to map to the last entire step segment.
	}
	float dt = tMax / SampleCount;
	float3 sunDirection = normalize(atmosphereUniforms.sunDirection);

	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);
	const float3 wi = sunDirection;
	const float3 wo = WorldDir;
	float cosTheta = dot(wi, wo);
	float MiePhaseValue = hgPhase(atmosphereParams.miePhaseG, -cosTheta); // negate cosTheta because due to WorldDir being a "in" direction. 
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
		float SunZenithCosAngle = dot(sunDirection, UpVector);
		float3 TransmittanceToSun = GetTransmittance(pHeight, SunZenithCosAngle);

		float3 PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;

		// Earth shadow 
		float tEarth = RaySphereIntersectNearest(P, sunDirection, earthO + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET * UpVector, atmosphereParams.bottomRadius);
		float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

		// Dual scattering for multi scattering 
		float3 multiScatteredLuminance = GetMultipleScattering(medium.scattering, medium.extinction, P, SunZenithCosAngle);

		float shadow = 1.0f;
#if SKY_ATMOSPHERE_SHADOWMAP_ENABLED
		// First evaluate opaque shadow
		shadow = GetShadow(P);
#endif

		float3 S = globalL * (earthShadow * shadow * TransmittanceToSun * PhaseTimesScattering + multiScatteredLuminance * medium.scattering);

		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		float3 Sint = (S - S * SampleTransmittance) / medium.extinction; // integrate along the current step segment 
		L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= SampleTransmittance;
	}
	return L;
}

float3 GetSunLuminance(float3 WorldPos, float3 WorldDir)
{
	const float sunHalfApexAngleRadian = 0.5 * atmosphereUniforms.sunAngularDiameter * PI / 180.0;
	const float sunCosHalfApexAngle = cos(sunHalfApexAngleRadian);
	
	float t = RaySphereIntersectNearest(WorldPos, WorldDir, float3(0.0f, 0.0f, 0.0f), atmosphereParams.bottomRadius);
	if (t < 0.0f) // no intersection
	{
		float3 sunLuminance = atmosphereUniforms.sunIlluminance;
		
		float cosZenithAngle = max(atmosphereUniforms.sunDirection.y, 0.0);
		float airMass = 1.0 / (cosZenithAngle + 0.025); // Kasten-Young formula approximation
		airMass = min(airMass, 38.0);
		
		float VdotL = dot(WorldDir, atmosphereUniforms.sunDirection);
		if (VdotL < sunCosHalfApexAngle) // outside sun disk
		{
			float offset = sunCosHalfApexAngle - VdotL;
			float atmosphericScale = lerp(1.0, 3.0, (airMass - 1.0) / 37.0);
			float angularScale = atmosphereUniforms.sunAngularDiameter / 0.5357f; // scale bloom based on defualt angular diameter
			
			float gaussianBloom = exp(-offset * 50000.0 / (angularScale + atmosphericScale)) * 0.5 * atmosphericScale;
			float invBloom = 1.0 / (0.02 + offset * 300.0 / (angularScale + atmosphericScale)) * 0.015 * atmosphericScale;
			float bloomFactor = gaussianBloom + invBloom;
			sunLuminance *= smoothstep(0.002, 1.0, bloomFactor);
		}
		return sunLuminance * GetAtmosphereTransmittance(WorldPos, WorldDir);
	}
	return 0;
}

#endif // SKY_ATMOSPHERE_COMMON_HLSL