#ifndef BRDF_HLSL
#define BRDF_HLSL

#include "Common.hlsli"
#include "ShaderTypes.h"

#define F90_Metal 1.0f

struct DirectLight
{
	float3 illuminance;
	float3 direction;
};

float Fd_Lambert()
{
    return INV_PI;
}

float3 Fd_Lambert(float3 albedo)
{
    return albedo * INV_PI;
}

float F0Dielectric(float reflectance)
{
    return 0.16 * reflectance * reflectance;
}

float F90Dielectric(float VdotH, float perceptualRoughess)
{
    return 0.5 + 2.0 * perceptualRoughess * VdotH * VdotH;
}

float F_Schlick(float f0, float f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(1.0f - VdotH, 5.0f);
}

float3 F_Schlick(float3 f0, float f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(1.0f - VdotH, 5.0f);
}

float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float perceptualRoughess)
{
    float energyBias = lerp(0.0f, 0.5f, perceptualRoughess);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, perceptualRoughess);
    float fd90 = energyBias + 2.0f * LdotH * LdotH * perceptualRoughess;
    float f0 = 1.0f;
    float lightScatter = F_Schlick(f0, fd90, NdotL);
    float viewScatter = F_Schlick(f0, fd90, NdotV);

    return lightScatter * viewScatter * energyFactor;
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float roughness)
{
    float alphaG2 = roughness * roughness;
    float Lambda_GGXV = NdotL * sqrt((NdotV - NdotV * alphaG2) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((NdotL - NdotL * alphaG2) * NdotL + alphaG2);
    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float D_GGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float denom = (NdotH * a2 - NdotH) * NdotH + 1.0f;
    return a2 / (denom * denom);
}

float3 GetDiffuseDominantDir(float3 N, float3 V, float NdotV, float roughness)
{
	float a = 1.02341f * roughness - 1.51174f;
	float b = -0.511705f * roughness + 0.755868f;
	float lerpFactor = saturate((NdotV * a + b) * roughness);
	return lerp(N, V, lerpFactor);
}

float3 GetSpecularDominantDir(const float3 n, const float3 r, float roughness)
{
	float smoothness = saturate(1.0f - roughness);
	float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
	return lerp(n, r, lerpFactor);
}

float3 EvaluateDiffuseDirectLight(float3 albedo, float metallic, float perceptualRoughness, float NdotV, float NdotL, float LdotH)
{
    float3 diffuseColor = albedo * (1.0 - metallic);
    float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, perceptualRoughness);
	return diffuseColor * (Fd * Fd_Lambert());
}

float3 EvaluateSpecularDirectLight(float3 albedo, float metallic, float perceptualRoughness, float NdotV, float NdotL, float LdotH, float NdotH)
{
    float roughness = perceptualRoughness * perceptualRoughness;

    float3 f0 = albedo * metallic + F0Dielectric(0.5) * (1.0 - metallic);
    float f90 = lerp(F90Dielectric(LdotH, perceptualRoughness), F90_Metal, metallic);

    float3 F = F_Schlick(f0, f90, LdotH);
    float D = D_GGX(NdotH, roughness);
    float V = V_SmithGGXCorrelated(NdotL, NdotV, roughness);

	return F * (D * V * Fd_Lambert());
}

float3 EvaluateDirectLight(Gleam::SurfaceOutput surface, DirectLight light, float3 viewDir, float3 worldNormal)
{
	float3 H = normalize(viewDir + light.direction);
	float NdotV = abs(dot(worldNormal, viewDir)) + 1e-5f;
	float NdotL = saturate(dot(worldNormal, light.direction));
	float NdotH = saturate(dot(worldNormal, H));
    float VdotH = saturate(dot(viewDir, H));
	float LdotH = saturate(dot(light.direction, H));

    float3 radiance = 0.0;
	radiance += EvaluateDiffuseDirectLight(surface.albedo.rgb, surface.metallic, surface.roughness, NdotV, NdotL, LdotH);
	radiance += EvaluateSpecularDirectLight(surface.albedo.rgb, surface.metallic, surface.roughness, NdotV, NdotL, LdotH, NdotH);
	return light.illuminance * radiance * NdotL;
}
#endif // BRDF_HLSL