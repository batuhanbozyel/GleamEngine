#define MESH_VERTEX_SHADER 1
#include "MeshShading.hlsli"

[shader("vertex")]
MeshVertexOut meshVertexShader(uint vertex_id : SV_VertexID)
{
	ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[resources.instanceBuffer];
	Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
	ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instanceData.interleavedBuffer];

	uint vertexID = vertex_id + instanceData.baseVertex;
	float3 position = positionBuffer.Load<float3>(vertexID * sizeof(float3));
	Gleam::InterleavedMeshVertex interleavedVert = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID * sizeof(Gleam::InterleavedMeshVertex));
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
