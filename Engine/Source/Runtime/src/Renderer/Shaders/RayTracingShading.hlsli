#ifndef RAY_TRACING_SHADING_HLSL
#define RAY_TRACING_SHADING_HLSL

#include "SurfaceShading.hlsli"
#include "PathTraceCommon.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, MESH_PASS_RESOURCES_BINDING_SLOT);

struct RayPayload
{
    float3 radiance;
    float  hitT;
    uint   depth;
};

struct ShadowPayload
{
    float visibility;
};

MeshVertexOut InterpolateVertexAttributes(Gleam::MeshInstanceData instance, uint primitiveIndex, float2 bary)
{
    ByteAddressBuffer indexBuffer       = ResourceDescriptorHeap[instance.indexBuffer];
    ByteAddressBuffer positionBuffer    = ResourceDescriptorHeap[instance.positionBuffer];
    ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instance.interleavedBuffer];

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

    float3 localPos    = b.x * p0            + b.y * p1            + b.z * p2;
    float3 normal      = b.x * v0.normal     + b.y * v1.normal     + b.z * v2.normal;
    float3 tangentXYZ  = b.x * v0.tangent.xyz + b.y * v1.tangent.xyz + b.z * v2.tangent.xyz;
    float2 uv          = b.x * v0.texCoord   + b.y * v1.texCoord   + b.z * v2.texCoord;
    float  tangentW    = v0.tangent.w; // handedness is constant across a triangle

    float3 worldPos   = mul(instance.transform, float4(localPos, 1.0)).xyz;
    float3 worldNorm  = normalize(mul((float3x3)instance.transform, normalize(normal)));
    float3 worldTan   = normalize(mul((float3x3)instance.transform, normalize(tangentXYZ)));
    float3 worldBitan = cross(worldNorm, worldTan) * tangentW;

    MeshVertexOut OUT;
    OUT.position      = float4(worldPos, 1.0);
    OUT.worldPosition = worldPos;
    OUT.normal        = worldNorm;
    OUT.tangent       = worldTan;
    OUT.bitangent     = worldBitan;
    OUT.color         = float4(1, 1, 1, 1);
    OUT.uv            = uv;
    return OUT;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attribs)
{
    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[resources.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(InstanceID() * sizeof(Gleam::MeshInstanceData));
    LoadMaterialInstance(instance.materialID);

    MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = surf(vertex);

    float3 viewDir = -WorldRayDirection();
    float3x3 TBN   = transpose(float3x3(vertex.tangent, vertex.bitangent, vertex.normal));
    float3 worldNormal = normalize(mul(TBN, surface.normal));

    ShadowPayload shadow;
    shadow.visibility = 1.0;

    RayDesc shadowRay;
    shadowRay.Origin    = vertex.worldPosition + worldNormal * 1e-3;
    shadowRay.Direction = atmosphereUniforms.sunDirection;
    shadowRay.TMin      = 1e-3;
    shadowRay.TMax      = 1e6;

    RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[constants.tlasIndex];
    TraceRay(
        tlas,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        0xFF,
        (uint)Gleam::DispatchRayType::Shadow,
        (uint)Gleam::DispatchRayType::COUNT,
        (uint)Gleam::DispatchRayType::Shadow,
        shadowRay,
        shadow
    );

    DirectLight light;
    light.direction   = atmosphereUniforms.sunDirection;
    light.illuminance = GetSunLuminance(GetSkyWorldPosition(vertex.worldPosition), atmosphereUniforms.sunDirection)
                        * shadow.visibility;

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
    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[resources.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(InstanceID() * sizeof(Gleam::MeshInstanceData));
    LoadMaterialInstance(instance.materialID);

    MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = surf(vertex);
    if (surface.albedo.a < 0.5)
	{
		IgnoreHit();
	}
}

[shader("anyhit")]
void ShadowAnyHit(inout ShadowPayload payload, BuiltInTriangleIntersectionAttributes attribs)
{
    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[resources.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(InstanceID() * sizeof(Gleam::MeshInstanceData));
    LoadMaterialInstance(instance.materialID);

    MeshVertexOut vertex = InterpolateVertexAttributes(instance, PrimitiveIndex(), attribs.barycentrics);
    Gleam::SurfaceOutput surface = surf(vertex);
    if (surface.albedo.a < 0.5)
	{
		IgnoreHit();
	}
}

#endif // RAY_TRACING_SHADING_HLSL
