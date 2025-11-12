#define MESH_VERTEX_SHADER 1
#include "MeshShading.hlsli"

#pragma vertex meshVertexShader

MeshVertexOut meshVertexShader(uint vertex_id : SV_VertexID)
{
	uint vertexID = vertex_id + instanceData.baseVertex;
	Gleam::InterleavedMeshVertex interleavedVert = instanceData.interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID);
    float3 position = instanceData.positionBuffer.Load<float3>(vertexID);
	float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));

    MeshVertexOut OUT;
    OUT.worldPosition = worldPosition.xyz;
    OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
	OUT.normal = normalize(mul(instanceData.transform, float4(interleavedVert.normal, 0.0f)).xyz);
	OUT.tangent = normalize(mul(instanceData.transform, float4(interleavedVert.tangent.xyz, 0.0f)).xyz);
    OUT.bitangent = normalize(cross(OUT.normal, OUT.tangent)) * sign(interleavedVert.tangent.w);
    OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    OUT.uv = interleavedVert.texCoord;
    return OUT;
}
