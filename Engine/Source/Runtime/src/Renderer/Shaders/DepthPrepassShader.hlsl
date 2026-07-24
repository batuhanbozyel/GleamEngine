#include "DepthPrepass.hlsli"
#include "MeshletCommon.hlsli"
#include "VisibilityBufferCommon.hlsli"

[shader("amplification")]
[numthreads(MESH_AMPLIFICATION_THREADS, 1, 1)]
void depthPrepassAmplificationShader(uint threadID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
    ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

    uint meshletID = groupID * MESH_AMPLIFICATION_THREADS + threadID;
    bool visible = false;

    if (meshletID < instanceData.meshletCount)
    {
        ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[instanceData.meshBuffer];
        Gleam::MeshletDescriptor meshlet = meshBuffer.Load<Gleam::MeshletDescriptor>(instanceData.meshletsOffset + (instanceData.baseMeshlet + meshletID) * sizeof(Gleam::MeshletDescriptor));
        visible = MeshletIsVisible(instanceData, meshlet, camera);
    }

    uint slot = WavePrefixCountBits(visible);
    uint visibleCount = WaveActiveCountBits(visible);

    if (visible)
    {
        gPayload.meshletIDs[slot] = meshletID;
    }

    DispatchMesh(visibleCount, 1, 1, gPayload);
}

[shader("mesh")]
[numthreads(MESH_SHADER_THREADS, 1, 1)]
[outputtopology("triangle")]
void depthPrepassMeshletShader(
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

    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[instanceData.meshBuffer];
    Gleam::MeshletDescriptor meshlet = meshBuffer.Load<Gleam::MeshletDescriptor>(instanceData.meshletsOffset + (instanceData.baseMeshlet + meshletID) * sizeof(Gleam::MeshletDescriptor));

    SetMeshOutputCounts(meshlet.vertexCount, meshlet.triangleCount);

    if (groupThreadID < meshlet.vertexCount)
    {
        uint localVertexIndex = meshBuffer.Load<uint>(instanceData.meshletVertexOffset + (meshlet.vertexOffset + groupThreadID) * sizeof(uint));
        uint vertexID = localVertexIndex + instanceData.baseVertex;

        float3 position = meshBuffer.Load<float3>(instanceData.positionsOffset + vertexID * sizeof(float3));
        float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));

        Gleam::InterleavedMeshVertex interleavedVert = meshBuffer.Load<Gleam::InterleavedMeshVertex>(instanceData.interleavedOffset + vertexID * sizeof(Gleam::InterleavedMeshVertex));

        Gleam::DepthPrepassVertexOut OUT;
        OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
        OUT.color = interleavedVert.color;
        OUT.uv = interleavedVert.texCoord;
        outVertices[groupThreadID] = OUT;
    }

    if (groupThreadID < meshlet.triangleCount)
    {
        uint packedTriangle = meshBuffer.Load(instanceData.meshletTriangleOffset + (meshlet.triangleOffset + groupThreadID) * sizeof(uint));
        outTriangles[groupThreadID] = UnpackMeshletTriangles(packedTriangle);
        outPrims[groupThreadID].visID = PackVisibilityID(instanceData.batchIndex, constants.instanceID, meshletID, groupThreadID);
    }
}

[shader("pixel")]
[earlydepthstencil]
PackedVisibilityID opaqueDepthPrepassFragmentShader(Gleam::DepthPrepassVertexOut IN, Gleam::VisibilityPrimOut prim) : SV_Target0
{
    return prim.visID;
}
