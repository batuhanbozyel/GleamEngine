#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::DebugShaderResources, resources, 0);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, 1);
PUSH_CONSTANT(Gleam::DebugMeshUniforms, uniforms);

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : ATTRIB0;
};

#pragma vertex debugVertexShader
#pragma vertex debugMeshVertexShader
#pragma fragment debugFragmentShader

VertexOut debugVertexShader(uint vertex_id: SV_VertexID)
{
	ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[resources.vertexBuffer];
    Gleam::DebugVertex vertex = vertexBuffer.Load<Gleam::DebugVertex>(vertex_id * sizeof(Gleam::DebugVertex));
    
    VertexOut OUT;
    OUT.position = mul(camera.viewProjectionMatrix, float4(vertex.position.xyz, 1.0f));
    OUT.color = unpack_unorm4x8_to_float(vertex.color);
    return OUT;
}

VertexOut debugMeshVertexShader(uint vertex_id: SV_VertexID)
{
	ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[resources.vertexBuffer];
    uint vertexID = vertex_id + uniforms.baseVertex;
    float3 position = vertexBuffer.Load<float3>(vertexID * sizeof(float3));
    
    VertexOut OUT;
    OUT.position = mul(camera.viewProjectionMatrix, mul(uniforms.transform, float4(position, 1.0f)));
    OUT.color = unpack_unorm4x8_to_float(uniforms.color);
    return OUT;
}

float4 debugFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color;
}
