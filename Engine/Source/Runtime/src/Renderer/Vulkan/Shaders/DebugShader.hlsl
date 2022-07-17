#include "../../ShaderTypes.h"

StructuredBuffer<Gleam::DebugVertex> VertexBuffer : register(t0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

[[vk::push_constant]]
Gleam::DebugVertexUniforms uniforms;

VertexOut debugVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::DebugVertex vertex = VertexBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = mul(uniforms.viewProjectionMatrix, float4(vertex.position, 1.0f));
    OUT.color = Gleam::unpack_unorm4x8_to_float(vertex.color);
    return OUT;
}

float4 debugFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color;
}
