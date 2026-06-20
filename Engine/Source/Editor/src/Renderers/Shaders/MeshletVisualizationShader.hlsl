#include "Common.hlsli"
#include "MeshletCommon.hlsli"
#include "../ShaderTypes.h"

PUSH_CONSTANT(GEditor::MeshletVisualizationConstants, constants);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

struct MeshletVisVertexOut
{
    float4 position : SV_POSITION;
};

struct MeshletVisPrimOut
{
    nointerpolation uint packedID : ATTRIB0;
};

[shader("amplification")]
[numthreads(MESH_AMPLIFICATION_THREADS, 1, 1)]
void meshletVisAmplificationShader(uint threadID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
    ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

    uint meshletID = groupID * MESH_AMPLIFICATION_THREADS + threadID;
    bool visible = false;

    if (meshletID < instanceData.meshletCount)
    {
        ByteAddressBuffer meshletsBuffer = ResourceDescriptorHeap[instanceData.meshletsBuffer];
        Gleam::MeshletDescriptor meshlet = meshletsBuffer.Load<Gleam::MeshletDescriptor>(instanceData.meshletOffset + meshletID * sizeof(Gleam::MeshletDescriptor));
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
void meshletVisMeshShader(
    uint groupThreadID : SV_GroupThreadID,
    uint meshletLocalID : SV_GroupID,
    in payload MeshletPayload meshletPayload,
    out indices uint3 outTriangles[MAX_MESHLET_TRIANGLES],
    out primitives MeshletVisPrimOut outPrims[MAX_MESHLET_TRIANGLES],
    out vertices MeshletVisVertexOut outVertices[MAX_MESHLET_VERTICES])
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

        MeshletVisVertexOut OUT;
        OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
        outVertices[groupThreadID] = OUT;
    }

    if (groupThreadID < meshlet.triangleCount)
    {
        ByteAddressBuffer meshletTriangleBuffer = ResourceDescriptorHeap[instanceData.meshletTriangleBuffer];
        uint packedTriangle = meshletTriangleBuffer.Load((meshlet.triangleOffset + groupThreadID) * sizeof(uint));
        outTriangles[groupThreadID] = UnpackMeshletTriangles(packedTriangle);
        outPrims[groupThreadID].packedID = (constants.instanceID << 16) | (meshletID & 0xFFFFu);
    }
}

float3 HashIDToColor(uint id)
{
    uint h = id * 2654435761u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return float3((h & 0xFFu), ((h >> 8) & 0xFFu), ((h >> 16) & 0xFFu)) / 255.0f;
}

[shader("pixel")]
float4 meshletVisFragmentShader(MeshletVisVertexOut IN, MeshletVisPrimOut prim) : SV_TARGET
{
    return float4(HashIDToColor(prim.packedID), 1.0f);
}
