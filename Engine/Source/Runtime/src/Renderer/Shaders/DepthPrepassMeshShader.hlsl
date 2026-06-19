#include "DepthPrepass.hlsli"
#include "MeshletCommon.hlsli"

#define MAX_MESHLET_VERTICES    64
#define MAX_MESHLET_TRIANGLES   126

[shader("amplification")]
[numthreads(MESH_AMPLIFICATION_THREADS, 1, 1)]
void depthPrepassAmplificationShader(uint threadID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
    if (threadID == 0)
    {
        BuildFrustumPlanes(camera.viewProjectionMatrix);
    }
    GroupMemoryBarrierWithGroupSync();

    ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

    uint meshletID = groupID * MESH_AMPLIFICATION_THREADS + threadID;
    bool visible = false;

    if (meshletID < instanceData.meshletCount)
    {
        ByteAddressBuffer meshletsBuffer = ResourceDescriptorHeap[instanceData.meshletsBuffer];
        Gleam::MeshletDescriptor meshlet = meshletsBuffer.Load<Gleam::MeshletDescriptor>(instanceData.meshletOffset + meshletID * sizeof(Gleam::MeshletDescriptor));

        float3 worldCenter = mul(instanceData.transform, float4(meshlet.center, 1.0f)).xyz;
        float3 worldConeApex = mul(instanceData.transform, float4(meshlet.coneApex, 1.0f)).xyz;
        float3 worldConeAxis = normalize(mul(instanceData.transform, float4(meshlet.coneAxis, 0.0f)).xyz);
        float scale = length(mul(instanceData.transform, float4(1.0f, 0.0f, 0.0f, 0.0f)).xyz);

        visible = FrustumCullMeshlet(worldCenter, meshlet.radius * scale)
               && !BackfaceCullMeshlet(worldConeApex, worldConeAxis, meshlet.coneCutoff, camera.position);
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
[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void depthPrepassMeshletShader(
    uint groupThreadID : SV_GroupThreadID,
    uint meshletLocalID : SV_GroupID,
    in payload MeshletPayload meshletPayload,
    out indices uint3 outTriangles[MAX_MESHLET_TRIANGLES],
    out vertices Gleam::DepthPrepassVertexOut outVertices[MAX_MESHLET_VERTICES])
{
    ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

    uint meshletID = meshletPayload.meshletIDs[meshletLocalID];

    ByteAddressBuffer meshletsBuffer = ResourceDescriptorHeap[instanceData.meshletsBuffer];
    Gleam::MeshletDescriptor meshlet = meshletsBuffer.Load<Gleam::MeshletDescriptor>(instanceData.meshletOffset + meshletID * sizeof(Gleam::MeshletDescriptor));

    SetMeshOutputCounts(meshlet.vertexCount, meshlet.triangleCount);

    if (groupThreadID < meshlet.vertexCount)
    {
        ByteAddressBuffer meshletVertexBuffer = ResourceDescriptorHeap[instanceData.meshletVertexBuffer];
        uint localVertexIndex = meshletVertexBuffer.Load<uint>((meshlet.vertexOffset + groupThreadID) * sizeof(uint));
        uint vertexID = localVertexIndex + instanceData.baseVertex;

        ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
        float3 position = positionBuffer.Load<float3>(vertexID * sizeof(float3));
        float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));
        float4 prevWorldPosition = mul(instanceData.previousTransform, float4(position, 1.0f));

        ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instanceData.interleavedBuffer];
        Gleam::InterleavedMeshVertex interleavedVert = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID * sizeof(Gleam::InterleavedMeshVertex));

        Gleam::DepthPrepassVertexOut OUT;
        OUT.prevClipPos = mul(camera.prevViewProjectionMatrix, prevWorldPosition);
        OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
        OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
        OUT.uv = interleavedVert.texCoord;
        OUT.normal = normalize(mul(instanceData.transform, float4(interleavedVert.normal, 0.0f)).xyz);
        outVertices[groupThreadID] = OUT;
    }

    if (groupThreadID < meshlet.triangleCount)
    {
        ByteAddressBuffer meshletTriangleBuffer = ResourceDescriptorHeap[instanceData.meshletTriangleBuffer];
        uint triByteOffset = meshlet.triangleOffset + groupThreadID * 3u;

        uint dword0 = meshletTriangleBuffer.Load(triByteOffset & ~3u);
        uint dword1 = meshletTriangleBuffer.Load((triByteOffset + 1u) & ~3u);
        uint dword2 = meshletTriangleBuffer.Load((triByteOffset + 2u) & ~3u);

        outTriangles[groupThreadID] = uint3(
            (dword0 >> ((triByteOffset & 3u) * 8u)) & 0xFFu,
            (dword1 >> (((triByteOffset + 1u) & 3u) * 8u)) & 0xFFu,
            (dword2 >> (((triByteOffset + 2u) & 3u) * 8u)) & 0xFFu);
    }
}
