#include "Common.hlsl"
#include "../ShaderTypes.h"

StructuredBuffer<Gleam::Vector3> PositionBuffer : register(t0);

StructuredBuffer<Gleam::DebugVertex> VertexBuffer : register(t0);

ConstantBuffer<Gleam::CameraUniforms> CameraBuffer : register(b0);

PUSH_CONSTANT(Gleam::DebugShaderUniforms, uniforms);

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOut debugVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::DebugVertex vertex = VertexBuffer[vertex_id];
    
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, float4(vertex.position.xyz, 1.0f));
    OUT.color = unpack_unorm4x8_to_float(vertex.color);
    return OUT;
}

VertexOut debugMeshVertexShader(uint vertex_id: SV_VertexID)
{
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(PositionBuffer[vertex_id].xyz, 1.0f)));
    OUT.color = unpack_unorm4x8_to_float(uniforms.color);
    return OUT;
}

float4 debugFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color;
}
