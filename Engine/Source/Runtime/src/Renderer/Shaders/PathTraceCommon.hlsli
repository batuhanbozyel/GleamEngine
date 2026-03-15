#ifndef PATH_TRACE_COMMON_HLSL
#define PATH_TRACE_COMMON_HLSL

#include "BRDF.hlsli"
#include "Colors.hlsli"
#include "Random.hlsli"

PUSH_CONSTANT(Gleam::PathTracerConstants, constants);

struct Ray
{
	float3 origin;
	float3 direction;
	float tMin;
	float tMax;
	uint depth;
};

enum BRDFType
{
	SPECULAR_BRDF,
	DIFFUSE_BRDF
};

uint initSeed(uint2 pixel, uint frameIndex)
{
	return pcgHash(pixel.x ^ pcgHash(pixel.y ^ pcgHash(frameIndex)));
}

#if 0 // Enable after ray-tracing support is added
// https://www.realtimerendering.com/raytracinggems/unofficial_RayTracingGems_v1.2.pdf
//  6.2.2.4 ADAPTIVE OFFSETTING ALONG THE GEOMETRIC NORMAL
float3 OffsetRayAlongNormal(const float3 p, const float3 n)
{
	const float origin = 1.0 / 32.0;
	const float floatScale = 1.0 / 65536.0;
	const float intScale = 256.0;
	
	int3 of_i = int3(intScale * n.x, intScale * n.y, intScale * n.z);
	float3 p_i = float3(asfloat(asint(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
						asfloat(asint(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
						asfloat(asint(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return float3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,
				  abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,
				  abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}
#endif

float SpecularLobeProbability(Gleam::SurfaceOutput surface, float NdotV)
{
	float3 f0 = surface.albedo * surface.metallic + F0Dielectric(0.5) * (1.0 - surface.metallic);
    // Use NdotV as a stand-in for VdotH at the sampling stage (H unknown yet)
	float f90 = lerp(F90Dielectric(NdotV, surface.roughness), F90_Metal, surface.metallic);
	float3 F = F_Schlick(f0, f90, NdotV);

	float specLuma = Luminance(F);
	float diffLuma = Luminance(surface.albedo * (1.0 - surface.metallic));
	return clamp(specLuma / max(specLuma + diffLuma, 1e-4), 0.1, 0.9);
}
#endif // PATH_TRACE_COMMON_HLSL