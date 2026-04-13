#ifndef PATH_TRACING_HLSL
#define PATH_TRACING_HLSL

#include "SurfaceShading.hlsli"
#include "PathTraceCommon.hlsli"

Gleam::MeshInstanceData LoadInstanceData(uint instanceID)
{
	ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[pathTraceConstants.instanceBuffer];
	Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.materialBuffer)];
	LoadMaterialInstance(materialBuffer, instance.materialID);
	return instance;
}

[shader("closesthit")]
void ClosestHit(inout Gleam::RayPayload payload : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    Gleam::MeshInstanceData instance = LoadInstanceData(InstanceID());
    MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    
    Gleam::SurfaceOutput surface = surf(vertex);
    surface.roughness = max(surface.roughness, 0.04);

    float3 viewDir = -WorldRayDirection();
    float3x3 TBN   = transpose(float3x3(vertex.tangent, vertex.bitangent, vertex.normal));
    float3 worldNormal = normalize(mul(TBN, surface.normal));
    
    DirectLight light;
    light.direction   = atmosphereUniforms.sunDirection;
    light.illuminance = GetSunLuminance(GetSkyWorldPosition(vertex.worldPosition), atmosphereUniforms.sunDirection);
    
    payload.radiance += payload.throughput * EvaluateDirectLight(surface, light, viewDir, worldNormal);
	payload.radiance += payload.throughput * surface.emission.rgb;
    
    float NdotV = dot(worldNormal, viewDir);
    if (NdotV <= 0.0)
    {
        return;
    }
    
    BRDFType brdfType;
    if (surface.metallic == 1.0 && surface.roughness <= PERFECT_MIRROR_ROUGHNESS)
    {
        brdfType = BRDFType::Specular;
    }
    else
    {
        float pSpec = SpecularLobeProbability(surface, NdotV);
        if (rand(payload.seed) < pSpec)
        {
            brdfType = BRDFType::Specular;
            payload.throughput /= pSpec;
        }
        else
        {
            brdfType = BRDFType::Diffuse;
            payload.throughput /= (1.0 - pSpec);
        }
    }

    float3 nextDir;
    float2 xi = rand2(payload.seed);
    if (brdfType == BRDFType::Specular)
    {
        float partialPdf;
        float3 H = ImportanceSampleGGX(xi, worldNormal, surface.roughness, partialPdf);
        nextDir = reflect(-viewDir, H);
        
        float NdotL = dot(worldNormal, nextDir);
		if (NdotL <= 0.0)
		{
			return;
		}
        
        float NdotH = dot(worldNormal, H);
        float VdotH = saturate(dot(viewDir, H));
        float LdotH = VdotH; // symmetric: LdotH == VdotH for reflect()

        float roughness = surface.roughness * surface.roughness;
        float3 f0 = surface.albedo.rgb * surface.metallic + F0Dielectric(0.5) * (1.0 - surface.metallic);
        float f90 = lerp(F90Dielectric(LdotH, surface.roughness), F90_Metal, surface.metallic);
        float3 F = F_Schlick(f0, f90, LdotH);
        float G = G_SmithGGXCorrelated(NdotL, NdotV, roughness);
        
        // Full BRDF: F * D * G / (4 * NdotL * NdotV)
		// Monte Carlo weight: brdf * NdotL / pdf
		// pdf: D * NdotH / (4 * VdotH)
#if EXPLICIT_SPECULAR_BRDF_FORMULA
		float pdf = partialPdf * NdotH / (4.0 * VdotH);
		float3 brdf = F * partialPdf * G / max(4.0 * NdotL * NdotV, 1e-4);
		payload.throughput *= brdf * NdotL / max(pdf, 1e-4);
#else
		payload.throughput *= F * G * VdotH / max(NdotV * NdotH, 1e-4);
#endif
	}
    else
    {
		float pdf; // The pdf is not used because it's canceled with other terms (The 1/PI from diffuse BRDF and the NdotL from Lambert's law).
        nextDir = CosineSampleHemisphere(xi, worldNormal, pdf);

        float NdotL = dot(worldNormal, nextDir);
        if (NdotL <= 0.0)
        {
            return;
        }

        float3 H = normalize(viewDir + nextDir);
        float LdotH = dot(nextDir, H);
        float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, surface.roughness);
        
        // weight = Fr_DisneyDiffuse * Fd_Lambert() * NdotL / pdf
#if EXPLICIT_DIFFUSE_BRDF_FORMULA
		float3 brdf = surface.albedo.rgb * (1.0 - surface.metallic) * Fd * Fd_Lambert();
		payload.throughput *= brdf * NdotL / max(pdf, 1e-4);
#else
        // CosineSampleHemisphere pdf = NdotL * INV_PI, so NdotL and INV_PI both cancel
		payload.throughput *= surface.albedo.rgb * (1.0 - surface.metallic) * Fd;
#endif
	}
    
    if (all(payload.throughput == 0.0))
    {
        return;
    }
    
	if (payload.depth >= 5)
	{
		float p = max(payload.throughput.r, max(payload.throughput.g, payload.throughput.b));
		if (rand(payload.seed) > p)
		{
			return;
		}
		payload.throughput /= p;
	}
    
	if (payload.depth < pathTraceConstants.maxRayRecursionDepth)
	{
		RayDesc ray;
		ray.Origin = OffsetRayAlongNormal(vertex.worldPosition, vertex.normal);
		ray.Direction = nextDir;
		ray.TMin = 1e-3;
		ray.TMax = 1e6;
		payload.depth += 1;
    
		Gleam::RayPayload reflection = payload;
		reflection.depth += 1;
        
        RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[pathTraceConstants.accelerationStructure];
		TraceRay(
            accelerationStructure,
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xFF,
            0,
            0,
            0,
            ray,
            reflection
        );
        
		payload.radiance += reflection.radiance;
	}
}

[shader("anyhit")]
void AnyHit(inout Gleam::RayPayload payload : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    Gleam::MeshInstanceData instance = LoadInstanceData(InstanceID());
    MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = surf(vertex);
    if (surface.albedo.a < 0.5)
    {
        IgnoreHit();
    }
}

#endif // PATH_TRACING_HLSL
