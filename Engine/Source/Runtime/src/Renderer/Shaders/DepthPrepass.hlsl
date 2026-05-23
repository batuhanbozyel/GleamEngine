#include "DepthPrepass.hlsli"

[shader("vertex")]
Gleam::DepthPrepassVertexOut depthPrepassVertexShader(uint vertexID : SV_VertexID)
{
	ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
	Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
    uint baseVertexID = vertexID + instanceData.baseVertex;
	float3 position = positionBuffer.Load<float3>(baseVertexID * sizeof(float3));
	float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));
    float4 prevWorldPosition = mul(instanceData.previousTransform, float4(position, 1.0f));
	
	ByteAddressBuffer interleavedBuffer = ResourceDescriptorHeap[instanceData.interleavedBuffer];
	Gleam::InterleavedMeshVertex interleavedVert = interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(baseVertexID * sizeof(Gleam::InterleavedMeshVertex));

    Gleam::DepthPrepassVertexOut OUT;
    OUT.prevClipPos = mul(camera.prevViewProjectionMatrix, prevWorldPosition);
    OUT.position = mul(camera.viewProjectionMatrix, worldPosition);
	OUT.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	OUT.uv = interleavedVert.texCoord;
	return OUT;
}

[shader("pixel")]
[earlydepthstencil]
float2 opaqueDepthPrepassFragmentShader(Gleam::DepthPrepassVertexOut IN) : SV_TARGET
{
    return ComputeMotionVector(IN, camera.resolution);
}
