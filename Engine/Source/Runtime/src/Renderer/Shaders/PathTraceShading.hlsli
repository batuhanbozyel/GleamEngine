#ifndef PATH_TRACING_HLSL
#define PATH_TRACING_HLSL

#include "SurfaceShading.hlsli"
#include "PathTraceCommon.hlsli"

static Texture2D<float> ggxEssTexture = ResourceDescriptorHeap[pathTraceConstants.ggxEssTexture];
static Texture2D<float> ggxEAvgTexture = ResourceDescriptorHeap[pathTraceConstants.ggxEAvgTexture];

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
    RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[pathTraceConstants.accelerationStructure];
    Gleam::MeshInstanceData instance = LoadInstanceData(InstanceID());
    Gleam::MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    
    Gleam::SurfaceOutput surface = SurfMain(vertex);
    surface.roughness = max(surface.roughness, 0.04);

    float3 viewDir = -WorldRayDirection();
    float3x3 TBN = transpose(float3x3(vertex.tangent, vertex.bitangent, vertex.normal));
    float3 worldNormal = normalize(mul(TBN, surface.normal));
    float3 newOrigin = OffsetRayAlongNormal(vertex.worldPosition, vertex.normal);
    
    float NdotV = dot(worldNormal, viewDir);
    if (NdotV <= 0.0)
    {
        return;
    }
    
    DirectLight light;
    light.direction = atmosphereUniforms.sunDirection;
    light.illuminance = GetSunLuminance(GetSkyWorldPosition(vertex.worldPosition), atmosphereUniforms.sunDirection);
    
    Gleam::ShadowPayload shadowPayload;
    shadowPayload.visibility = 0.0;

    float shadowConePdf;
    const float sunHalfAngle = 0.5 * atmosphereUniforms.sunAngularDiameter * (PI / 180.0);
    const float cosSunHalfAngle = cos(sunHalfAngle);
    float3 shadowDir = UniformSampleCone(PathTraceRand2(payload.seed), light.direction, cosSunHalfAngle, shadowConePdf);

    RayDesc shadowRay;
    shadowRay.Origin = newOrigin;
    shadowRay.Direction = shadowDir;
    shadowRay.TMin = camera.nearPlane;
    shadowRay.TMax = camera.farPlane;

    TraceRay(
        accelerationStructure,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        0xFF,
        (uint)Gleam::RayType::ShadowRay,
        0,
        (uint)Gleam::RayType::ShadowRay,
        shadowRay,
        shadowPayload
    );
    
    payload.radiance += payload.throughput * EvaluateDirectLight(surface,
                                                                 pathTraceConstants.ggxEssTexture,
                                                                 pathTraceConstants.ggxEAvgTexture,
                                                                 light,
                                                                 viewDir,
                                                                 worldNormal) * shadowPayload.visibility;
    payload.radiance += payload.throughput * surface.emission.rgb;
    
    BRDFType brdfType;
    if (surface.metallic == 1.0 && surface.roughness <= PERFECT_MIRROR_ROUGHNESS)
    {
        brdfType = BRDFType::Specular;
    }
    else
    {
        float pSpec = SpecularLobeProbability(surface, NdotV);
        if (PathTraceRand(payload.seed) < pSpec)
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
    float2 xi = PathTraceRand2(payload.seed);
    if (brdfType == BRDFType::Specular)
    {
        float pdf;
        float3 H = ImportanceSampleGGX_VNDF(xi, viewDir, worldNormal, surface.roughness, pdf);
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
        float G2 = G_SmithGGXCorrelated(NdotL, NdotV, roughness);
        float G1V = G1_SmithGGX(NdotV, roughness);

        // Full BRDF: F * D * G2 / (4 * NdotL * NdotV)
        // VNDF pdf:  G1(V) * D(H) / (4 * NdotV)
        // Monte Carlo weight: brdf * NdotL / pdf = F * G2 / G1
#if EXPLICIT_SPECULAR_BRDF_FORMULA
        float D     = D_GGX(NdotH, roughness);
        float3 brdf = F * D * G2 / max(4.0 * NdotL * NdotV, 1e-4);
        float3 brdfWeight = brdf * NdotL / max(pdf, 1e-4);
#else
        float3 brdfWeight = F * G2 / max(G1V, 1e-4);
#endif
        payload.throughput *= brdfWeight * MultiscatteringGGX(ggxEssTexture, ggxEAvgTexture, f0, surface.roughness, NdotV);
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
    
    if (all(payload.throughput < FLT_EPSILON))
    {
        return;
    }
    
    if (payload.depth >= 5)
    {
        float p = max(payload.throughput.r, max(payload.throughput.g, payload.throughput.b));
        if (PathTraceRand(payload.seed) > p)
        {
            return;
        }
        payload.throughput /= p;
    }
    
    if (payload.depth < pathTraceConstants.maxRayRecursionDepth)
    {
        RayDesc ray;
        ray.Origin = newOrigin;
        ray.Direction = nextDir;
        ray.TMin = camera.nearPlane;
        ray.TMax = camera.farPlane;
        
        Gleam::RayPayload reflection;
        reflection.radiance = 0.0;
        reflection.throughput = payload.throughput;
        reflection.seed = payload.seed;
        reflection.depth = payload.depth + 1;
        
        TraceRay(
            accelerationStructure,
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xFF,
            (uint)Gleam::RayType::PrimaryRay,
            0,
            (uint)Gleam::RayType::PrimaryRay,
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
    Gleam::MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = SurfMain(vertex);
    if (surface.albedo.a < surface.alphaCutoff)
    {
        IgnoreHit();
    }
}

[shader("anyhit")]
void ShadowAnyHit(inout Gleam::ShadowPayload payload : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    Gleam::MeshInstanceData instance = LoadInstanceData(InstanceID());
    Gleam::MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = SurfMain(vertex);
    if (surface.albedo.a < surface.alphaCutoff)
    {
        IgnoreHit();
    }
}

#endif // PATH_TRACING_HLSL
