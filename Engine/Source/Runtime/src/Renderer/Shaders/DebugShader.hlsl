#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::DebugShaderResources, resources, 0);
PUSH_CONSTANT(Gleam::DebugMeshUniforms, uniforms);

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

#pragma vertex debugVertexShader
#pragma vertex debugMeshVertexShader
#pragma fragment debugFragmentShader

VertexOut debugVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::CameraUniforms CameraBuffer = resources.cameraBuffer.Load<Gleam::CameraUniforms>();
    Gleam::DebugVertex vertex = resources.vertexBuffer.Load<Gleam::DebugVertex>(vertex_id);
    
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, float4(vertex.position.xyz, 1.0f));
    OUT.color = unpack_unorm4x8_to_float(vertex.color);
    return OUT;
}

VertexOut debugMeshVertexShader(uint vertex_id: SV_VertexID)
{
    uint vertexID = vertex_id + uniforms.baseVertex;
    Gleam::CameraUniforms CameraBuffer = resources.cameraBuffer.Load<Gleam::CameraUniforms>();
    float3 position = resources.vertexBuffer.Load<float3>(vertexID);
    
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(position, 1.0f)));
    OUT.color = unpack_unorm4x8_to_float(uniforms.color);
    return OUT;
}

float4 debugFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color;
}
