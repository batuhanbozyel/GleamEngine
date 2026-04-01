#ifndef PATH_TRACING_HLSL
#define PATH_TRACING_HLSL

#include "SurfaceShading.hlsli"
#include "PathTraceCommon.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

Gleam::MeshInstanceData LoadInstanceData(uint instanceID)
{
	ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[resources.instanceBuffer];
	Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.materialBuffer)];
	LoadMaterialInstance(materialBuffer, instance.materialID);
	return instance;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attribs)
{
    Gleam::MeshInstanceData instance = LoadInstanceData(InstanceID());
    MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = surf(vertex);

    float3 viewDir = -WorldRayDirection();
    float3x3 TBN   = transpose(float3x3(vertex.tangent, vertex.bitangent, vertex.normal));
    float3 worldNormal = normalize(mul(TBN, surface.normal));

    DirectLight light;
    light.direction   = atmosphereUniforms.sunDirection;
    light.illuminance = GetSunLuminance(GetSkyWorldPosition(vertex.worldPosition), atmosphereUniforms.sunDirection);

    float3 color = 0.0;
    color += EvaluateDirectLight(surface, light, viewDir, worldNormal);
    color += EvaluateIndirectLight(surface, resources.brdfTexture,
                                   resources.diffuseReflectionTexture, resources.specularReflectionTexture,
                                   viewDir, worldNormal);
    color += surface.emission.rgb;

    payload.radiance = color;
    payload.hitT     = RayTCurrent();
}

[shader("anyhit")]
void AnyHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attribs)
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
