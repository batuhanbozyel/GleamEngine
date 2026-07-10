#define VISIBILITY_SHADING_PATH
#include "DepthPrepass.hlsli"
#include "MeshletCommon.hlsli"
#include "VisibilityBufferCommon.hlsli"

[shader("mesh")]
[numthreads(MESH_SHADER_THREADS, 1, 1)]
[outputtopology("triangle")]
void depthPrepassVisibilityMeshletShader(
    uint groupThreadID : SV_GroupThreadID,
    uint meshletLocalID : SV_GroupID,
    in payload MeshletPayload meshletPayload,
    out indices uint3 outTriangles[MAX_MESHLET_TRIANGLES],
    out primitives Gleam::VisibilityPrimOut outPrims[MAX_MESHLET_TRIANGLES],
    out vertices Gleam::DepthPrepassVertexOut outVertices[MAX_MESHLET_VERTICES])
{
    ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

    uint meshletID = meshletPayload.meshletIDs[meshletLocalID];

    ByteAddressBuffer meshletsBuffer = ResourceDescriptorHeap[instanceData.meshletsBuffer];
    Gleam::MeshletDescriptor meshlet = meshletsBuffer.Load<Gleam::MeshletDescriptor>((instanceData.baseMeshlet + meshletID) * sizeof(Gleam::MeshletDescriptor));

    SetMeshOutputCounts(meshlet.vertexCount, meshlet.triangleCount);

    if (groupThreadID < meshlet.vertexCount)
    {
        ByteAddressBuffer meshletVertexBuffer = ResourceDescriptorHeap[instanceData.meshletVertexBuffer];
        uint localVertexIndex = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + groupThreadID) * sizeof(uint));
        uint vertexID = localVertexIndex + instanceData.baseVertex;

        ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
        float3 position = positionBuffer.Load<float3>(vertexID * sizeof(float3));
        float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));

        ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instanceData.interleavedBuffer];
        Gleam::InterleavedMeshVertex interleavedVert = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID * sizeof(Gleam::InterleavedMeshVertex));

        Gleam::DepthPrepassVertexOut OUT;
        OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
        OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
        OUT.uv = interleavedVert.texCoord;
        OUT.normal = normalize(mul(instanceData.transform, float4(interleavedVert.normal, 0.0f)).xyz);
        outVertices[groupThreadID] = OUT;
    }

    if (groupThreadID < meshlet.triangleCount)
    {
        ByteAddressBuffer meshletTriangleBuffer = ResourceDescriptorHeap[instanceData.meshletTriangleBuffer];
        uint packedTriangle = meshletTriangleBuffer.Load((meshlet.triangleOffset + groupThreadID) * sizeof(uint));
        outTriangles[groupThreadID] = UnpackMeshletTriangles(packedTriangle);
        outPrims[groupThreadID].visID = PackVisibilityID(instanceData.batchIndex, constants.instanceID, meshletID, groupThreadID);
    }
}

[shader("pixel")]
[earlydepthstencil]
PackedVisibilityID opaqueDepthPrepassVisibilityFragmentShader(Gleam::DepthPrepassVertexOut IN, Gleam::VisibilityPrimOut prim) : SV_Target0
{
    return prim.visID;
}
