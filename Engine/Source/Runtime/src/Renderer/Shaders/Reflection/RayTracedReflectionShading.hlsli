#ifndef RAY_TRACED_REFLECTION_SHADING_HLSL
#define RAY_TRACED_REFLECTION_SHADING_HLSL

#include "PathTraceCommon.hlsli"

PUSH_CONSTANT(Gleam::RayTracedReflectionConstants, constants);

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
    Gleam::MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);

    Gleam::SurfaceOutput surface = SurfMain(vertex);
    surface.roughness = max(surface.roughness, 0.04);

    float3 viewDir = -WorldRayDirection();
    float3x3 TBN = transpose(float3x3(vertex.tangent, vertex.bitangent, vertex.normal));
    float3 worldNormal = normalize(mul(TBN, surface.normal));
    if (dot(worldNormal, viewDir) <= 0.0)
    {
        return;
    }

    DirectLight light;
    light.direction = atmosphereUniforms.sunDirection;
    if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex && atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
    {
        light.illuminance = GetSunLuminance(GetSkyWorldPosition(vertex.worldPosition), atmosphereUniforms.sunDirection);
    }
    else
    {
        light.illuminance = atmosphereUniforms.sunIlluminance;
    }

    float shadowConePdf;
    const float sunHalfAngle = 0.5 * atmosphereUniforms.sunAngularDiameter * (PI / 180.0);
    const float cosSunHalfAngle = cos(sunHalfAngle);
    float3 shadowDir = UniformSampleCone(PCGRand2(payload.seed), light.direction, cosSunHalfAngle, shadowConePdf);

    RayDesc shadowRay;
    shadowRay.Origin    = OffsetRayAlongNormal(vertex.worldPosition, vertex.normal);
    shadowRay.Direction = shadowDir;
    shadowRay.TMin      = camera.nearPlane;
    shadowRay.TMax      = camera.farPlane;

    Gleam::ShadowPayload shadowPayload;
    shadowPayload.visibility = 0.0;

    RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[pathTraceConstants.accelerationStructure];
    TraceRay(accelerationStructure,
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             0xFF,
             (uint)Gleam::RayType::ShadowRay,
             0,
             (uint)Gleam::RayType::ShadowRay,
             shadowRay,
             shadowPayload);

    payload.radiance = surface.emission.rgb;
    payload.radiance += EvaluateDirectLight(surface,
                                            pathTraceConstants.ggxEssTexture,
                                            pathTraceConstants.ggxEAvgTexture,
                                            light,
                                            viewDir,
                                            worldNormal) * shadowPayload.visibility;
    payload.radiance += EvaluateIndirectLight(surface,
                                              constants.brdfTexture,
                                              pathTraceConstants.ggxEssTexture,
                                              pathTraceConstants.ggxEAvgTexture,
                                              constants.diffuseReflectionTexture,
                                              constants.specularReflectionTexture,
                                              InvalidResourceIndex,
                                              uint2(0, 0),
                                              viewDir,
                                              worldNormal) * surface.occlusion;
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
#endif // RAY_TRACED_REFLECTION_SHADING_HLSL
