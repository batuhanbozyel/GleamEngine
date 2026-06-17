#include "MeshShading.hlsli"

[shader("vertex")]
Gleam::MeshVertexOut meshVertexShader(uint vertexID : SV_VertexID)
{
	ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
	Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
    uint baseVertexID = vertexID + instanceData.baseVertex;
	float3 position = positionBuffer.Load<float3>(baseVertexID * sizeof(float3));
	float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));

	ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instanceData.interleavedBuffer];
	Gleam::InterleavedMeshVertex interleavedVert = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(baseVertexID * sizeof(Gleam::InterleavedMeshVertex));

    Gleam::MeshVertexOut OUT;
	OUT.worldPosition = worldPosition.xyz;
	OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
	OUT.normal = normalize(mul(instanceData.transform, float4(interleavedVert.normal, 0.0f)).xyz);
	OUT.tangent = normalize(mul(instanceData.transform, float4(interleavedVert.tangent.xyz, 0.0f)).xyz);
	OUT.bitangent = normalize(cross(OUT.normal, OUT.tangent)) * interleavedVert.tangent.w;
	OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	OUT.uv = interleavedVert.texCoord;
	OUT.ddxUV = float2(0.0f, 0.0f);
	OUT.ddyUV = float2(0.0f, 0.0f);
	return OUT;
}
