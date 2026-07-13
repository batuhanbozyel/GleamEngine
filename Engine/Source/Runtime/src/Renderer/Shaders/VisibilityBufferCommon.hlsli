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
    
struct VertexAttributes
{
    float3 positions[3];
    float3 normals[3];
    float4 tangents[3];
    float2 texCoords[3];
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
    ByteAddressBuffer meshletsBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.meshletsBuffer)];
    Gleam::MeshletDescriptor meshlet = meshletsBuffer.Load<Gleam::MeshletDescriptor>((instance.baseMeshlet + meshletID) * sizeof(Gleam::MeshletDescriptor));

    ByteAddressBuffer meshletTriangleBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.meshletTriangleBuffer)];
    uint packedTriangle = meshletTriangleBuffer.Load((meshlet.triangleOffset + triangleID) * sizeof(uint));
    uint3 tri = UnpackMeshletTriangles(packedTriangle);

    ByteAddressBuffer meshletVertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.meshletVertexBuffer)];
    uint3 vertexIDs;
    vertexIDs.x = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + tri.x) * sizeof(uint)) + instance.baseVertex;
    vertexIDs.y = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + tri.y) * sizeof(uint)) + instance.baseVertex;
    vertexIDs.z = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + tri.z) * sizeof(uint)) + instance.baseVertex;
    return vertexIDs;
}

Gleam::VertexAttributes LoadVertexAttributes(Gleam::MeshInstanceData instance, uint meshletID, uint triangleID)
{
    uint3 vertexIDs = LoadTriangleVertexIDs(instance, meshletID, triangleID);
    ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.positionBuffer)];
    
    Gleam::VertexAttributes attribs;
    attribs.positions[0] = positionBuffer.Load<float3>(vertexIDs.x * sizeof(float3));
    attribs.positions[1] = positionBuffer.Load<float3>(vertexIDs.y * sizeof(float3));
    attribs.positions[2] = positionBuffer.Load<float3>(vertexIDs.z * sizeof(float3));
    
    ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.interleavedBuffer)];
    Gleam::InterleavedMeshVertex v0 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexIDs.x * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v1 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexIDs.y * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v2 = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexIDs.z * sizeof(Gleam::InterleavedMeshVertex));
    
    attribs.normals[0] = v0.normal;
    attribs.normals[1] = v1.normal;
    attribs.normals[2] = v2.normal;
    
    attribs.tangents[0] = v0.tangent;
    attribs.tangents[1] = v1.tangent;
    attribs.tangents[2] = v2.tangent;
    
    attribs.texCoords[0] = v0.texCoord;
    attribs.texCoords[1] = v1.texCoord;
    attribs.texCoords[2] = v2.texCoord;
    
    return attribs;
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
    Gleam::VertexAttributes attribs = LoadVertexAttributes(instance, visID.meshletID, visID.triangleID);
    float3 worldPos0 = mul(instance.transform, attribs.positions[0]).xyz;
    float3 worldPos1 = mul(instance.transform, attribs.positions[1]).xyz;
    float3 worldPos2 = mul(instance.transform, attribs.positions[2]).xyz;

    float4 clipPos0 = mul(camera.viewProjectionMatrix, float4(worldPos0, 1.0f));
    float4 clipPos1 = mul(camera.viewProjectionMatrix, float4(worldPos1, 1.0f));
    float4 clipPos2 = mul(camera.viewProjectionMatrix, float4(worldPos2, 1.0f));

    Gleam::BarycentricDeriv deriv = CalculateScreenSpaceBarycentrics(clipPos0, clipPos1, clipPos2, PixelSpaceToNDC(pixelCoords, camera.resolution), camera.resolution);
    
    float3 objectNormal = InterpolateBary(deriv, attribs.normals[0], attribs.normals[1], attribs.normals[2]);
    float3 objectTangent = InterpolateBary(deriv, attribs.tangents[0].xyz, attribs.tangents[1].xyz, attribs.tangents[2].xyz);
    float3 normal = normalize(mul(instance.transform, float4(objectNormal, 0.0f)).xyz);
    float3 tangent = normalize(mul(instance.transform, float4(objectTangent, 0.0f)).xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * attribs.tangents[0].w;

    Gleam::MeshVertexOut OUT = (Gleam::MeshVertexOut)0;
    OUT.position = float4(float2(pixelCoords) + 0.5f, 0.0f, 1.0f);
    OUT.worldPosition = InterpolateBary(deriv, worldPos0, worldPos1, worldPos2);
    OUT.normal = normal;
    OUT.tangent = tangent;
    OUT.bitangent = bitangent;
    OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    InterpolateUV(deriv, attribs.texCoords[0], attribs.texCoords[1], attribs.texCoords[2], OUT.uv, OUT.ddxUV, OUT.ddyUV);
    
#ifdef MOTION_VECTOR_PASS
    float3 prevWorldPos0  = mul(instance.previousTransform, float4(attribs.positions[0], 1.0f)).xyz;
    float3 prevWorldPos1  = mul(instance.previousTransform, float4(attribs.positions[1], 1.0f)).xyz;
    float3 prevWorldPos2  = mul(instance.previousTransform, float4(attribs.positions[2], 1.0f)).xyz;
    OUT.prevWorldPosition = InterpolateBary(deriv, prevWorldPos0, prevWorldPos1, prevWorldPos2);
#endif // MOTION_VECTOR_PASS
    
    return OUT;
}

bool UnpackVisibilityShading(Gleam::VisibilityResolveConstants constants, uint threadID,
    out Gleam::MeshVertexOut vertex, out Gleam::SurfaceOutput surface, out uint2 pixelCoords)
{
    vertex = (Gleam::MeshVertexOut)0;
    surface = (Gleam::SurfaceOutput)0;
    pixelCoords = uint2(0u, 0u);

    ByteAddressBuffer countsBuffer = ResourceDescriptorHeap[constants.countsBuffer];
    uint pixelCount = countsBuffer.Load(constants.batchIndex * sizeof(uint));
    if (threadID >= pixelCount)
    {
        return false;
    }

    ByteAddressBuffer offsetsBuffer = ResourceDescriptorHeap[constants.offsetsBuffer];
    uint offset = offsetsBuffer.Load(constants.batchIndex * sizeof(uint));

    ByteAddressBuffer pixelListBuffer = ResourceDescriptorHeap[constants.pixelListBuffer];
    uint packedPixel = pixelListBuffer.Load((offset + threadID) * sizeof(uint));
    pixelCoords = uint2(packedPixel & 0xFFFFu, packedPixel >> 16u);

    Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[constants.visibilityBuffer];
    Gleam::VisibilityID visID = UnpackVisibilityID(visibilityBuffer.Load(int3(pixelCoords, 0)));

    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(visID.instanceID * sizeof(Gleam::MeshInstanceData));

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[instance.materialBuffer];
    LoadMaterialInstance(materialBuffer, instance.materialID);

    vertex = InterpolateVertexAttributes(instance, visID, pixelCoords);
    surface = SurfMain(vertex);
    surface.roughness = max(surface.roughness, 0.04);
    return true;
}

#endif // VISIBILITY_BUFFER_COMMON_HLSLI
