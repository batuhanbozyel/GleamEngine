#ifndef VISIBILITY_BUFFER_COMMON_HLSLI
#define VISIBILITY_BUFFER_COMMON_HLSLI

#include "MeshletCommon.hlsli"
#include "SurfaceShading.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

// Visibility buffer encoding (R32G32_UInt), bit budget defined in ShaderInterop.h:
//   R = (batchIndex << 17) | (instanceID + 1) (0 = background, matches zero clear;
//       instanceID + 1 fits 17 bits since MaxMeshInstances = 65536, leaving 15 bits for batchIndex)
//   G = (meshletID << 7) | triangleID (meshletID is instance-relative, triangleID < MAX_MESHLET_TRIANGLES fits 7 bits)
#define PackedVisibilityID uint2

namespace Gleam {

struct VisibilityPrimOut
{
    nointerpolation PackedVisibilityID visID : ATTRIB4;
};

struct VisibilityID
{
    uint instanceID;
    uint meshletID;
    uint triangleID;
};
    
// Perspective-correct barycentrics with analytic screen-space derivatives,
// from "Visibility Buffer Rendering with Material Graphs" (John Hable).
struct BarycentricDeriv
{
    float3 lambda;
    float3 ddxLambda;
    float3 ddyLambda;
};

} // namespace Gleam

PackedVisibilityID PackVisibilityID(uint batchIndex, uint instanceID, uint meshletID, uint triangleID)
{
    return PackedVisibilityID((batchIndex << VISIBILITY_INSTANCE_BITS) | (instanceID + 1u), (meshletID << VISIBILITY_TRIANGLE_BITS) | (triangleID & VISIBILITY_TRIANGLE_MASK));
}

bool IsValidVisibilityID(PackedVisibilityID packedID)
{
    return (packedID.x & VISIBILITY_INSTANCE_MASK) != 0u;
}

uint UnpackVisibilityBatchIndex(PackedVisibilityID packedID)
{
    return packedID.x >> VISIBILITY_INSTANCE_BITS;
}

Gleam::VisibilityID UnpackVisibilityID(PackedVisibilityID packedID)
{
    Gleam::VisibilityID id;
    id.instanceID = (packedID.x & VISIBILITY_INSTANCE_MASK) - 1u;
    id.meshletID = packedID.y >> VISIBILITY_TRIANGLE_BITS;
    id.triangleID = packedID.y & VISIBILITY_TRIANGLE_MASK;
    return id;
}

uint3 LoadTriangleVertexIDs(Gleam::MeshInstanceData instance, uint meshletID, uint triangleID)
{
    ByteAddressBuffer meshletsBuffer = ResourceDescriptorHeap[instance.meshletsBuffer];
    Gleam::MeshletDescriptor meshlet = meshletsBuffer.Load<Gleam::MeshletDescriptor>((instance.baseMeshlet + meshletID) * sizeof(Gleam::MeshletDescriptor));

    ByteAddressBuffer meshletTriangleBuffer = ResourceDescriptorHeap[instance.meshletTriangleBuffer];
    uint packedTriangle = meshletTriangleBuffer.Load((meshlet.triangleOffset + triangleID) * sizeof(uint));
    uint3 tri = UnpackMeshletTriangles(packedTriangle);

    ByteAddressBuffer meshletVertexBuffer = ResourceDescriptorHeap[instance.meshletVertexBuffer];
    uint3 vertexIDs;
    vertexIDs.x = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + tri.x) * sizeof(uint)) + instance.baseVertex;
    vertexIDs.y = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + tri.y) * sizeof(uint)) + instance.baseVertex;
    vertexIDs.z = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + tri.z) * sizeof(uint)) + instance.baseVertex;
    return vertexIDs;
}

float2 PixelToNdc(uint2 pixel, float2 resolution)
{
    float2 uv = (float2(pixel) + 0.5f) / resolution;
    return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

Gleam::BarycentricDeriv CalculateScreenSpaceBarycentrics(float4 clip0, float4 clip1, float4 clip2, float2 pixelNdc, float2 resolution)
{
    Gleam::BarycentricDeriv ret = (Gleam::BarycentricDeriv)0;

    float3 invW = rcp(float3(clip0.w, clip1.w, clip2.w));
    float2 ndc0 = clip0.xy * invW.x;
    float2 ndc1 = clip1.xy * invW.y;
    float2 ndc2 = clip2.xy * invW.z;

    float invDet = rcp(determinant(float2x2(ndc2 - ndc1, ndc0 - ndc1)));
    ret.ddxLambda = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet * invW;
    ret.ddyLambda = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet * invW;
    float ddxSum = dot(ret.ddxLambda, float3(1.0f, 1.0f, 1.0f));
    float ddySum = dot(ret.ddyLambda, float3(1.0f, 1.0f, 1.0f));

    float2 deltaVec = pixelNdc - ndc0;
    float interpInvW = invW.x + deltaVec.x * ddxSum + deltaVec.y * ddySum;
    float interpW = rcp(interpInvW);

    ret.lambda.x = interpW * (invW.x + deltaVec.x * ret.ddxLambda.x + deltaVec.y * ret.ddyLambda.x);
    ret.lambda.y = interpW * (0.0f   + deltaVec.x * ret.ddxLambda.y + deltaVec.y * ret.ddyLambda.y);
    ret.lambda.z = interpW * (0.0f   + deltaVec.x * ret.ddxLambda.z + deltaVec.y * ret.ddyLambda.z);

    ret.ddxLambda *= (2.0f / resolution.x);
    ret.ddyLambda *= (2.0f / resolution.y);
    ddxSum        *= (2.0f / resolution.x);
    ddySum        *= (2.0f / resolution.y);

    // Screen-space y goes down while NDC y goes up
    ret.ddyLambda *= -1.0f;
    ddySum        *= -1.0f;

    float interpDdxW = rcp(interpInvW + ddxSum);
    float interpDdyW = rcp(interpInvW + ddySum);

    ret.ddxLambda = interpDdxW * (ret.lambda * interpInvW + ret.ddxLambda) - ret.lambda;
    ret.ddyLambda = interpDdyW * (ret.lambda * interpInvW + ret.ddyLambda) - ret.lambda;
    return ret;
}

float3 InterpolateWithDeriv(Gleam::BarycentricDeriv deriv, float v0, float v1, float v2)
{
    float3 mergedV = float3(v0, v1, v2);
    return float3(dot(mergedV, deriv.lambda), dot(mergedV, deriv.ddxLambda), dot(mergedV, deriv.ddyLambda));
}

float2 InterpolateBary(Gleam::BarycentricDeriv deriv, float2 v0, float2 v1, float2 v2)
{
    return v0 * deriv.lambda.x + v1 * deriv.lambda.y + v2 * deriv.lambda.z;
}

float3 InterpolateBary(Gleam::BarycentricDeriv deriv, float3 v0, float3 v1, float3 v2)
{
    return v0 * deriv.lambda.x + v1 * deriv.lambda.y + v2 * deriv.lambda.z;
}

float4 InterpolateBary(Gleam::BarycentricDeriv deriv, float4 v0, float4 v1, float4 v2)
{
    return v0 * deriv.lambda.x + v1 * deriv.lambda.y + v2 * deriv.lambda.z;
}

void InterpolateUV(Gleam::BarycentricDeriv deriv, float2 uv0, float2 uv1, float2 uv2, out float2 uv, out float2 ddxUV, out float2 ddyUV)
{
    float3 interpU = InterpolateWithDeriv(deriv, uv0.x, uv1.x, uv2.x);
    float3 interpV = InterpolateWithDeriv(deriv, uv0.y, uv1.y, uv2.y);
    uv = float2(interpU.x, interpV.x);
    ddxUV = float2(interpU.y, interpV.y);
    ddyUV = float2(interpU.z, interpV.z);
}

Gleam::MeshVertexOut InterpolateVertexAttributes(Gleam::MeshInstanceData instance, Gleam::VisibilityID visID, uint2 pixelCoords)
{
    uint3 vertexIDs = LoadTriangleVertexIDs(instance, visID.meshletID, visID.triangleID);

    ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instance.positionBuffer];
    float3 w0 = mul(instance.transform, float4(positionBuffer.Load<float3>(vertexIDs.x * sizeof(float3)), 1.0f)).xyz;
    float3 w1 = mul(instance.transform, float4(positionBuffer.Load<float3>(vertexIDs.y * sizeof(float3)), 1.0f)).xyz;
    float3 w2 = mul(instance.transform, float4(positionBuffer.Load<float3>(vertexIDs.z * sizeof(float3)), 1.0f)).xyz;

    float4 clip0 = mul(camera.viewProjectionMatrix, float4(w0, 1.0f));
    float4 clip1 = mul(camera.viewProjectionMatrix, float4(w1, 1.0f));
    float4 clip2 = mul(camera.viewProjectionMatrix, float4(w2, 1.0f));

    Gleam::BarycentricDeriv deriv = CalculateScreenSpaceBarycentrics(clip0, clip1, clip2, PixelToNdc(pixelCoords, camera.resolution), camera.resolution);

    ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instance.interleavedBuffer];
    Gleam::InterleavedMeshVertex v0 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexIDs.x * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v1 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexIDs.y * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v2 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexIDs.z * sizeof(Gleam::InterleavedMeshVertex));
    
    float3 objectNormal = InterpolateBary(deriv, v0.normal, v1.normal, v2.normal);
    float3 objectTangent = InterpolateBary(deriv, v0.tangent.xyz, v1.tangent.xyz, v2.tangent.xyz);
    float3 normal = normalize(mul(instance.transform, float4(objectNormal, 0.0f)).xyz);
    float3 tangent = normalize(mul(instance.transform, float4(objectTangent, 0.0f)).xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * v0.tangent.w;

    Gleam::MeshVertexOut OUT = (Gleam::MeshVertexOut)0;
    OUT.position = float4(float2(pixelCoords) + 0.5f, 0.0f, 1.0f);
    OUT.worldPosition = InterpolateBary(deriv, w0, w1, w2);
    OUT.normal = normal;
    OUT.tangent = tangent;
    OUT.bitangent = bitangent;
    OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    InterpolateUV(deriv, v0.texCoord, v1.texCoord, v2.texCoord, OUT.uv, OUT.ddxUV, OUT.ddyUV);
    return OUT;

}

#endif // VISIBILITY_BUFFER_COMMON_HLSLI
