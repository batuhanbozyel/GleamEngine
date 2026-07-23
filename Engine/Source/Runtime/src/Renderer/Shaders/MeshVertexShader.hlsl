#include "MeshShading.hlsli"
#include "MeshletCommon.hlsli"

[shader("amplification")]
[numthreads(MESH_AMPLIFICATION_THREADS, 1, 1)]
void meshAmplificationShader(uint threadID : SV_GroupThreadID, uint groupID : SV_GroupID)
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
void meshMeshletShader(
    uint groupThreadID : SV_GroupThreadID,
    uint meshletLocalID : SV_GroupID,
    in payload MeshletPayload meshletPayload,
    out indices uint3 outTriangles[MAX_MESHLET_TRIANGLES],
    out vertices Gleam::MeshVertexOut outVertices[MAX_MESHLET_VERTICES])
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

        Gleam::MeshVertexOut OUT;
        OUT.worldPosition = worldPosition.xyz;
        OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
        OUT.normal = normalize(mul(instanceData.transform, float4(interleavedVert.normal, 0.0f)).xyz);
        OUT.tangent = normalize(mul(instanceData.transform, float4(interleavedVert.tangent.xyz, 0.0f)).xyz);
        OUT.bitangent = normalize(cross(OUT.normal, OUT.tangent)) * interleavedVert.tangent.w;
        OUT.color = interleavedVert.color;
        OUT.uv = interleavedVert.texCoord;
        OUT.ddxUV = float2(0.0f, 0.0f);
        OUT.ddyUV = float2(0.0f, 0.0f);
        outVertices[groupThreadID] = OUT;
    }

    if (groupThreadID < meshlet.triangleCount)
    {
        uint packedTriangle = meshBuffer.Load(instanceData.meshletTriangleOffset + (meshlet.triangleOffset + groupThreadID) * sizeof(uint));
        outTriangles[groupThreadID] = UnpackMeshletTriangles(packedTriangle);
    }
}
