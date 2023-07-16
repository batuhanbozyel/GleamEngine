#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

[[vk::binding(Gleam::RendererBindingTable::PositionBuffer)]]
StructuredBuffer<Gleam::Vector3> PositionBuffer;

[[vk::binding(Gleam::RendererBindingTable::PositionBuffer)]]
StructuredBuffer<Gleam::DebugVertex> VertexBuffer;

[[vk::binding(Gleam::RendererBindingTable::CameraBuffer)]]
ConstantBuffer<Gleam::CameraUniforms> CameraBuffer;

[[vk::push_constant]]
Gleam::DebugShaderUniforms uniforms;

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
    OUT.color = Gleam::unpack_unorm4x8_to_float(vertex.color);
    return OUT;
}

VertexOut debugMeshVertexShader(uint vertex_id: SV_VertexID)
{
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(PositionBuffer[vertex_id].xyz, 1.0f)));
    OUT.color = Gleam::unpack_unorm4x8_to_float(uniforms.color);
    return OUT;
}

float4 debugFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color;
}
