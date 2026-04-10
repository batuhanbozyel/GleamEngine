#ifndef BRDF_HLSL
#define BRDF_HLSL

#include "Common.hlsli"
#include "ShaderTypes.h"

#define F90_Metal 1.0f
#define PERFECT_MIRROR_ROUGHNESS 0.0016

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

// V = G / (4 * NdotL * NdotV)
float V_SmithGGXCorrelated(float NdotL, float NdotV, float roughness)
{
    float alphaG2 = roughness * roughness;
    float Lambda_GGXV = NdotL * sqrt((NdotV - NdotV * alphaG2) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((NdotL - NdotL * alphaG2) * NdotL + alphaG2);
    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

// G = V * (4 * NdotL * NdotV)
float G_SmithGGXCorrelated(float NdotL, float NdotV, float roughness)
{
	float alphaG2 = roughness * roughness;
	float Lambda_GGXV = NdotL * sqrt((NdotV - NdotV * alphaG2) * NdotV + alphaG2);
	float Lambda_GGXL = NdotV * sqrt((NdotL - NdotL * alphaG2) * NdotL + alphaG2);
	return (2.0f * NdotL * NdotV) / (Lambda_GGXV + Lambda_GGXL);
}

float D_GGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float denom = (NdotH * a2 - NdotH) * NdotH + 1.0f;
    return a2 / (denom * denom);
}

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 CosineSampleHemisphere(float2 u, float3 N, out float pdf)
{
	float theta = TWO_PI * u.x;
	float cosPhi2 = 1.0 - u.y;
	float cosPhi = sqrt(cosPhi2);
	float sinPhi = sqrt(1.0f - cosPhi2);
	float3 L = float3(sinPhi * cos(theta), cosPhi, sinPhi * sin(theta));

	pdf = cosPhi * INV_PI;

	float3 tangent;
	float3 bitangent;
	GetOrthonormalBasis(N, tangent, bitangent);
	
	float3 sampleVec = tangent * L.x + N * L.y + bitangent * L.z;
	return normalize(sampleVec);
}

float3 ImportanceSampleGGX(float2 u, float3 N, float perceptualRoughness, out float pdf)
{
    float a = perceptualRoughness * perceptualRoughness;
    float a2 = a * a;
    
    float theta = TWO_PI * u.x;
	float cosPhi2 = (1.0 - u.y) / (1.0 + (a2 - 1.0) * u.y);
    float cosPhi = sqrt(cosPhi2);
    float sinPhi = sqrt(max(0.0, 1.0 - cosPhi2));
	float3 H = float3(cos(theta) * sinPhi, cosPhi, sin(theta) * sinPhi);
	
	float d = (cosPhi * a2 - cosPhi) * cosPhi + 1.0;
	float D = a2 / (PI * d * d);
	pdf = D; // partial PDF (full PDF = D * NdotH / (4 * VdotH))
	
	float3 tangent;
	float3 bitangent;
	GetOrthonormalBasis(N, tangent, bitangent);
	
	float3 sampleVec = tangent * H.x + N * H.y + bitangent * H.z;
	return normalize(sampleVec);
}

float3 UniformSampleCone(float2 u, float3 N, float cosHalfAngle, out float pdf)
{
    float cosTheta = 1.0 - u.y * (1.0 - cosHalfAngle);
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = TWO_PI * u.x;

    float3 tangent;
    float3 bitangent;
    GetOrthonormalBasis(N, tangent, bitangent);

    pdf = 1.0 / (TWO_PI * (1.0 - cosHalfAngle));
    return normalize(tangent * (sinTheta * cos(phi)) + N * cosTheta + bitangent * (sinTheta * sin(phi)));
}

float PerceptualRoughnessToMipLevel(float perceptualRoughness, int maxMip)
{
	return maxMip * perceptualRoughness * (2.0 - perceptualRoughness);
}

float MipLevelToPerceptualRoughness(float mipLevel, int maxMip)
{
	return 1.0 - sqrt(1.0 - mipLevel / float(maxMip));
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
	float NdotV = abs(dot(worldNormal, viewDir)) + FLT_EPSILON;
	float NdotL = saturate(dot(worldNormal, light.direction));
	float NdotH = saturate(dot(worldNormal, H));
    float VdotH = saturate(dot(viewDir, H));
	float LdotH = saturate(dot(light.direction, H));
    
	if (NdotL <= 0.0f || NdotH <= 0.0f)
	{
		return 0.0;
	}

    float3 radiance = 0.0;
	radiance += EvaluateDiffuseDirectLight(surface.albedo.rgb, surface.metallic, surface.roughness, NdotV, NdotL, LdotH);
	radiance += EvaluateSpecularDirectLight(surface.albedo.rgb, surface.metallic, surface.roughness, NdotV, NdotL, LdotH, NdotH);
	return light.illuminance * radiance * NdotL;
}

float3 GetDiffuseDominantDir(float3 N, float3 V, float NdotV, float perceptualRoughness)
{
	float a = 1.02341f * perceptualRoughness - 1.51174f;
	float b = -0.511705f * perceptualRoughness + 0.755868f;
	float lerpFactor = saturate((NdotV * a + b) * perceptualRoughness);
	return lerp(N, V, lerpFactor);
}

float3 GetSpecularDominantDir(const float3 N, const float3 R, float perceptualRoughness)
{
	float smoothness = saturate(1.0f - perceptualRoughness);
	float lerpFactor = smoothness * (sqrt(smoothness) + perceptualRoughness);
	return lerp(N, R, lerpFactor);
}

float3 EvaluateDiffuseIndirectLight(Texture2D<float4> brdfTexture, TextureCube<float4> diffuseReflection, float3 albedo, float metallic, float perceptualRoughness, float3 viewDir, float3 worldNormal, float NdotV)
{
	float3 diffuseColor = albedo * (1.0 - metallic);
	float3 diffuseDirection = GetDiffuseDominantDir(worldNormal, viewDir, NdotV, perceptualRoughness);
	float3 diffuseLighting = diffuseReflection.Sample(Sampler_Bilinear_Repeat, diffuseDirection).rgb;
	
	float diffF = brdfTexture.SampleLevel(Sampler_Bilinear_Clamp, float2(NdotV, perceptualRoughness), 0).w;
	return diffuseLighting * diffuseColor * diffF;
}

float3 EvaluateSpecularIndirectLight(Texture2D<float4> brdfTexture, TextureCube<float4> specularReflection, float3 albedo, float metallic, float perceptualRoughness, float3 viewDir, float3 worldNormal, float NdotV)
{
	float3 reflectionDir = normalize(reflect(-viewDir, worldNormal));
	float3 specularDirection = GetSpecularDominantDir(worldNormal, reflectionDir, perceptualRoughness);
	float3 f0 = albedo * metallic + F0Dielectric(0.5) * (1.0 - metallic);
	
	float mipLevel = PerceptualRoughnessToMipLevel(perceptualRoughness, SPECULAR_RADIANCE_MAX_MIP_COUNT - 1);
	float3 specularLighting = specularReflection.SampleLevel(Sampler_Trilinear_Repeat, specularDirection, mipLevel).rgb;
	
	float3 DFG = brdfTexture.SampleLevel(Sampler_Bilinear_Clamp, float2(NdotV, perceptualRoughness), 0).xyz;
	return specularLighting * (f0 * DFG.x + lerp(DFG.y /* F90Dielectric(LdotH, perceptualRoughness) */, DFG.z /* F90_Metal */, metallic));
}

float3 EvaluateIndirectLight(Gleam::SurfaceOutput surface,
							 Gleam::ShaderResourceIndex brdfTextureIndex,
							 Gleam::ShaderResourceIndex diffuseReflectionTextureIndex,
							 Gleam::ShaderResourceIndex specularReflectionTextureIndex,
							 float3 viewDir,
							 float3 worldNormal)
{
	Texture2D<float4> brdfTexture = ResourceDescriptorHeap[brdfTextureIndex];
	TextureCube<float4> diffuseReflectionTexture = ResourceDescriptorHeap[diffuseReflectionTextureIndex];
	TextureCube<float4> specularReflectionTexture = ResourceDescriptorHeap[specularReflectionTextureIndex];
	float NdotV = abs(dot(worldNormal, viewDir)) + FLT_EPSILON;
	
	float3 irradiance = 0.0;
	irradiance += EvaluateDiffuseIndirectLight(brdfTexture, diffuseReflectionTexture, surface.albedo.rgb, surface.metallic, surface.roughness, viewDir, worldNormal, NdotV);
	irradiance += EvaluateSpecularIndirectLight(brdfTexture, specularReflectionTexture, surface.albedo.rgb, surface.metallic, surface.roughness, viewDir, worldNormal, NdotV);
	return irradiance;
}
#endif // BRDF_HLSL