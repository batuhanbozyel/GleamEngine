#include "DepthPrepass.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

[shader("vertex")]
Gleam::MeshVertexOut depthPrepassVertexShader(uint vertex_id : SV_VertexID)
{
	ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
	Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
	uint vertexID = vertex_id + instanceData.baseVertex;
	float3 position = positionBuffer.Load<float3>(vertexID * sizeof(float3));
	float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));
    
    ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instanceData.interleavedBuffer];
    Gleam::InterleavedMeshVertex interleavedVert = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID * sizeof(Gleam::InterleavedMeshVertex));

    Gleam::MeshVertexOut OUT;
	OUT.worldPosition = worldPosition.xyz;
	OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
	OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	OUT.uv = interleavedVert.texCoord;
	OUT.ddxUV = float2(0.0f, 0.0f);
	OUT.ddyUV = float2(0.0f, 0.0f);
	return OUT;
}
