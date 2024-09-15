#include "MeshShading.hlsli"

#pragma vertex meshVertexShader

MeshVertexOut meshVertexShader(uint vertex_id: SV_VertexID)
{
    uint vertexID = vertex_id + resources.baseVertex;
    Gleam::CameraUniforms CameraBuffer = resources.cameraBuffer.Load<Gleam::CameraUniforms>();
    Gleam::InterleavedMeshVertex interleavedVert = resources.interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID);
    float3 position = resources.positionBuffer.Load<float3>(vertexID);

    MeshVertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(resources.modelMatrix, float4(position, 1.0f)));
    OUT.worldNormal = normalize(mul(resources.modelMatrix, float4(interleavedVert.normal, 0.0f)).xyz);
    OUT.color = float3(interleavedVert.texCoord.x, interleavedVert.texCoord.y, 0.0f);
    OUT.uv = interleavedVert.texCoord;
    return OUT;
}
