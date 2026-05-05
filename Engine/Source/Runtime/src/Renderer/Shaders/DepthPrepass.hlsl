#include "DepthPrepass.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

[shader("vertex")]
Gleam::DepthPrepassVertexOut depthPrepassVertexShader(uint vertexID : SV_VertexID)
{
	ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
	Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
	float3 position = positionBuffer.Load<float3>((vertexID + instanceData.baseVertex) * sizeof(float3));
	float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));
    float4 prevWorldPosition = mul(instanceData.prevTransform, float4(position, 1.0f));

    Gleam::DepthPrepassVertexOut OUT;
    OUT.currentClipPos = mul(camera.viewProjectionMatrix, worldPosition);
    OUT.prevClipPos = mul(camera.prevViewProjectionMatrix, prevWorldPosition);
    OUT.position = OUT.currentClipPos;
	OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	OUT.uv = interleavedVert.texCoord;
	return OUT;
}
