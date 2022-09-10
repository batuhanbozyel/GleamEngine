#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

[[vk::binding(Gleam::RendererBindingTable::Buffer0)]]
StructuredBuffer<Gleam::Vector3> PositionBuffer;

[[vk::binding(Gleam::RendererBindingTable::Buffer0)]]
StructuredBuffer<Gleam::DebugVertex> VertexBuffer;

[[vk::binding(Gleam::RendererBindingTable::CameraBuffer)]]
cbuffer<Gleam::CameraUniforms> CameraBuffer;

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOut debugVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::DebugVertex vertex = VertexBuffer[vertex_id];
    
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, float4(vertex.position, 1.0f));
    OUT.color = Gleam::unpack_unorm4x8_to_float(vertex.color);
    return OUT;
}

VertexOut debugMeshVertexShader(uint vertex_id: SV_VertexID)
{
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(PositionBuffer[vertex_id], 1.0f)));
    OUT.color = float4(1.0);
    return OUT;
}

[[vk::push_constant]]
Gleam::DebugFragmentUniforms fragmentUniforms;

float4 debugFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color * Gleam::unpack_unorm4x8_to_float(fragmentUniforms.color);
}
