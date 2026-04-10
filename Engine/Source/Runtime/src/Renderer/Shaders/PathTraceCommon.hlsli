#ifndef PATH_TRACE_COMMON_HLSL
#define PATH_TRACE_COMMON_HLSL

#include "BRDF.hlsli"
#include "Colors.hlsli"
#include "Random.hlsli"
#include "SurfaceShading.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

PUSH_CONSTANT(Gleam::PathTracerConstants, pathTraceConstants);

static RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[pathTraceConstants.accelerationStructure];

enum class BRDFType
{
	Specular,
	Diffuse
};

//#define USE_PCG
#ifdef USE_PCG
    #define PathTraceSeed PCGSeed
    #define PathTraceInitSeed PCGInitSeed
    #define PathTraceRand PCGRand
    #define PathTraceRand2 PCGRand2
#else
    #define PathTraceSeed SobolSeed
    #define PathTraceInitSeed SobolInitSeed
    #define PathTraceRand SobolRand
    #define PathTraceRand2 SobolRand2
#endif

//https://www.realtimerendering.com/raytracinggems/unofficial_RayTracingGems_v1.2.pdf
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

float SpecularLobeProbability(Gleam::SurfaceOutput surface, float NdotV)
{
	float3 f0 = surface.albedo.rgb * surface.metallic + F0Dielectric(0.5) * (1.0 - surface.metallic);
    // Use NdotV as a stand-in for VdotH at the sampling stage (H unknown yet)
	float f90 = lerp(F90Dielectric(NdotV, surface.roughness), F90_Metal, surface.metallic);
	float3 F = F_Schlick(f0, f90, NdotV);

	float specLuma = Luminance(F);
	float diffLuma = Luminance(surface.albedo * (1.0 - surface.metallic));
	return clamp(specLuma / max(specLuma + diffLuma, 1e-4), 0.1, 0.9);
}

MeshVertexOut InterpolateVertexAttributes(Gleam::MeshInstanceData instance, uint primitiveIndex, float2 bary)
{
    ByteAddressBuffer indexBuffer       = ResourceDescriptorHeap[NonUniformResourceIndex(instance.indexBuffer)];
    ByteAddressBuffer positionBuffer    = ResourceDescriptorHeap[NonUniformResourceIndex(instance.positionBuffer)];
	ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.interleavedBuffer)];

    uint baseIdx = instance.firstIndex + primitiveIndex * 3;
    uint i0 = instance.baseVertex + indexBuffer.Load<uint>(baseIdx       * sizeof(uint));
    uint i1 = instance.baseVertex + indexBuffer.Load<uint>((baseIdx + 1) * sizeof(uint));
    uint i2 = instance.baseVertex + indexBuffer.Load<uint>((baseIdx + 2) * sizeof(uint));

    float3 p0 = positionBuffer.Load<float3>(i0 * sizeof(float3));
    float3 p1 = positionBuffer.Load<float3>(i1 * sizeof(float3));
    float3 p2 = positionBuffer.Load<float3>(i2 * sizeof(float3));

    Gleam::InterleavedMeshVertex v0 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(i0 * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v1 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(i1 * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v2 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(i2 * sizeof(Gleam::InterleavedMeshVertex));

    float3 b = float3(1.0 - bary.x - bary.y, bary.x, bary.y);

    float3 position   = b.x * p0             + b.y * p1             + b.z * p2;
    float3 normal     = b.x * v0.normal      + b.y * v1.normal      + b.z * v2.normal;
    float3 tangentXYZ = b.x * v0.tangent.xyz + b.y * v1.tangent.xyz + b.z * v2.tangent.xyz;
    float2 uv         = b.x * v0.texCoord    + b.y * v1.texCoord    + b.z * v2.texCoord;
    float  tangentW   = v0.tangent.w;
    
    
    
    MeshVertexOut OUT;
    OUT.worldPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	OUT.position      = mul(camera.viewProjectionMatrix, float4(OUT.worldPosition, 1.0));
    OUT.normal        = normalize(mul(instance.transform, float4(normal, 0.0f)).xyz);
    OUT.tangent       = normalize(mul(instance.transform, float4(tangentXYZ, 0.0f)).xyz);
    OUT.bitangent     = normalize(cross(OUT.normal, OUT.tangent)) * tangentW;
    OUT.color         = float4(1, 1, 1, 1);
    OUT.uv            = uv;
    return OUT;
}

#endif // PATH_TRACE_COMMON_HLSL
