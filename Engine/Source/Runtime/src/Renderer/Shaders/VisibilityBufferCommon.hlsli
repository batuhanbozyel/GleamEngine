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
    nointerpolation PackedVisibilityID visID : ATTRIB2;
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
    float4 colors[3];
};
    
struct VisibilitySample
{
    MeshVertexOut vertex;
    SurfaceOutput surface;
    uint2 pixelCoords;
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
    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.meshBuffer)];
    Gleam::MeshletDescriptor meshlet = meshBuffer.Load<Gleam::MeshletDescriptor>(instance.meshletsOffset + (instance.baseMeshlet + meshletID) * sizeof(Gleam::MeshletDescriptor));

    uint packedTriangle = meshBuffer.Load(instance.meshletTriangleOffset + (meshlet.triangleOffset + triangleID) * sizeof(uint));
    uint3 tri = UnpackMeshletTriangles(packedTriangle);

    uint3 vertexIDs;
    vertexIDs.x = meshBuffer.Load<uint>(instance.meshletVertexOffset + (meshlet.vertexOffset + tri.x) * sizeof(uint)) + instance.baseVertex;
    vertexIDs.y = meshBuffer.Load<uint>(instance.meshletVertexOffset + (meshlet.vertexOffset + tri.y) * sizeof(uint)) + instance.baseVertex;
    vertexIDs.z = meshBuffer.Load<uint>(instance.meshletVertexOffset + (meshlet.vertexOffset + tri.z) * sizeof(uint)) + instance.baseVertex;
    return vertexIDs;
}

Gleam::VertexAttributes LoadVertexAttributes(Gleam::MeshInstanceData instance, uint meshletID, uint triangleID)
{
    uint3 vertexIDs = LoadTriangleVertexIDs(instance, meshletID, triangleID);
    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instance.meshBuffer)];

    Gleam::VertexAttributes attribs;
    attribs.positions[0] = meshBuffer.Load<float3>(instance.positionsOffset + vertexIDs.x * sizeof(float3));
    attribs.positions[1] = meshBuffer.Load<float3>(instance.positionsOffset + vertexIDs.y * sizeof(float3));
    attribs.positions[2] = meshBuffer.Load<float3>(instance.positionsOffset + vertexIDs.z * sizeof(float3));

    Gleam::InterleavedMeshVertex v0 = meshBuffer.Load<Gleam::InterleavedMeshVertex>(instance.interleavedOffset + vertexIDs.x * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v1 = meshBuffer.Load<Gleam::InterleavedMeshVertex>(instance.interleavedOffset + vertexIDs.y * sizeof(Gleam::InterleavedMeshVertex));
    Gleam::InterleavedMeshVertex v2 = meshBuffer.Load<Gleam::InterleavedMeshVertex>(instance.interleavedOffset + vertexIDs.z * sizeof(Gleam::InterleavedMeshVertex));
    
    attribs.normals[0] = v0.normal;
    attribs.normals[1] = v1.normal;
    attribs.normals[2] = v2.normal;
    
    attribs.tangents[0] = v0.tangent;
    attribs.tangents[1] = v1.tangent;
    attribs.tangents[2] = v2.tangent;
    
    attribs.texCoords[0] = v0.texCoord;
    attribs.texCoords[1] = v1.texCoord;
    attribs.texCoords[2] = v2.texCoord;

    attribs.colors[0] = v0.color;
    attribs.colors[1] = v1.color;
    attribs.colors[2] = v2.color;

    return attribs;
}

uint4 PackBarycentricDerivatives(Gleam::BarycentricDeriv deriv)
{
    return uint4(f32tof16(deriv.ddxLambda.x), f32tof16(deriv.ddxLambda.y),
                 f32tof16(deriv.ddyLambda.x), f32tof16(deriv.ddyLambda.y));
}

Gleam::BarycentricDeriv UnpackBarycentricDerivatives(float2 lambda, uint4 packedDerivs)
{
    Gleam::BarycentricDeriv deriv = (Gleam::BarycentricDeriv)0;
    deriv.lambda = float3(lambda, 1.0f - lambda.x - lambda.y);
    deriv.ddxLambda.x = f16tof32(packedDerivs.x);
    deriv.ddxLambda.y = f16tof32(packedDerivs.y);
    deriv.ddxLambda.z = -(deriv.ddxLambda.x + deriv.ddxLambda.y);
    deriv.ddyLambda.x = f16tof32(packedDerivs.z);
    deriv.ddyLambda.y = f16tof32(packedDerivs.w);
    deriv.ddyLambda.z = -(deriv.ddyLambda.x + deriv.ddyLambda.y);
    return deriv;
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

Gleam::MeshVertexOut InterpolateVertexAttributes(Gleam::MeshInstanceData instance, Gleam::VertexAttributes attribs, Gleam::BarycentricDeriv deriv, uint2 pixelCoords)
{
    float3 worldPos0 = mul(instance.transform, float4(attribs.positions[0], 1.0)).xyz;
    float3 worldPos1 = mul(instance.transform, float4(attribs.positions[1], 1.0)).xyz;
    float3 worldPos2 = mul(instance.transform, float4(attribs.positions[2], 1.0)).xyz;

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
    OUT.color = InterpolateBary(deriv, attribs.colors[0], attribs.colors[1], attribs.colors[2]);
    InterpolateUV(deriv, attribs.texCoords[0], attribs.texCoords[1], attribs.texCoords[2], OUT.uv, OUT.ddxUV, OUT.ddyUV);
    
#ifdef MOTION_VECTOR_PASS
    float3 prevWorldPos0  = mul(instance.previousTransform, float4(attribs.positions[0], 1.0f)).xyz;
    float3 prevWorldPos1  = mul(instance.previousTransform, float4(attribs.positions[1], 1.0f)).xyz;
    float3 prevWorldPos2  = mul(instance.previousTransform, float4(attribs.positions[2], 1.0f)).xyz;
    OUT.prevWorldPosition = InterpolateBary(deriv, prevWorldPos0, prevWorldPos1, prevWorldPos2);
#endif // MOTION_VECTOR_PASS

    return OUT;
}

bool UnpackVisibilityPixel(Gleam::VisibilityResolveConstants constants, uint threadID,
    out Gleam::MeshInstanceData instance, out Gleam::VisibilityID visID, out uint2 pixelCoords)
{
    instance = (Gleam::MeshInstanceData)0;
    visID = (Gleam::VisibilityID)0;
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
    visID = UnpackVisibilityID(visibilityBuffer.Load(int3(pixelCoords, 0)));

    ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    instance = instanceBuffer.Load<Gleam::MeshInstanceData>(visID.instanceID * sizeof(Gleam::MeshInstanceData));

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[instance.materialBuffer];
    LoadMaterialInstance(materialBuffer, instance.materialID);
    return true;
}

bool UnpackVisibilityShading(Gleam::VisibilityResolveConstants constants, uint threadID,
    Gleam::ShaderResourceIndex barycentricCoords, Gleam::ShaderResourceIndex barycentricDerivatives,
    out Gleam::VisibilitySample visSample)
{
    visSample = (Gleam::VisibilitySample)0;

    Gleam::MeshInstanceData instance;
    Gleam::VisibilityID visID;
    if (UnpackVisibilityPixel(constants, threadID, instance, visID, visSample.pixelCoords) == false)
    {
        return false;
    }

    Texture2D<float2> barycentricCoordsTexture = ResourceDescriptorHeap[barycentricCoords];
    Texture2D<uint4> barycentricDerivsTexture = ResourceDescriptorHeap[barycentricDerivatives];
    Gleam::VertexAttributes attribs = LoadVertexAttributes(instance, visID.meshletID, visID.triangleID);
    Gleam::BarycentricDeriv deriv = UnpackBarycentricDerivatives(barycentricCoordsTexture.Load(int3(visSample.pixelCoords, 0)),
                                                                 barycentricDerivsTexture.Load(int3(visSample.pixelCoords, 0)));
    
    visSample.vertex = InterpolateVertexAttributes(instance, attribs, deriv, visSample.pixelCoords);
    visSample.surface = SurfMain(visSample.vertex);
    visSample.surface.roughness = max(visSample.surface.roughness, 0.04);
    return true;
}

#endif // VISIBILITY_BUFFER_COMMON_HLSLI
