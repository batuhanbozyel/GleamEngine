#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

struct VertexOut
{
    float4 position [[position]];
    float4 color;
};

vertex VertexOut debugVertexShader(uint vertexID [[vertex_id]],
                                   constant Gleam::DebugVertex* VertexBuffer [[buffer(Gleam::RendererBindingTable::PositionBuffer)]],
                                   constant Gleam::CameraUniforms& CameraBuffer [[buffer(Gleam::RendererBindingTable::CameraBuffer)]])
{
    Gleam::DebugVertex vert = VertexBuffer[vertexID];

    VertexOut out;
    out.position = CameraBuffer.viewProjectionMatrix * float4(vert.position.xyz, 1.0);
    out.color = unpack_unorm4x8_to_float(vert.color);
    return out;
}

vertex VertexOut debugMeshVertexShader(uint vertexID [[vertex_id]],
                                       constant Gleam::Vector3* PositionBuffer [[buffer(Gleam::RendererBindingTable::PositionBuffer)]],
                                       constant Gleam::CameraUniforms& CameraBuffer [[buffer(Gleam::RendererBindingTable::CameraBuffer)]],
                                       constant Gleam::DebugShaderUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    VertexOut out;
    out.position = CameraBuffer.viewProjectionMatrix * uniforms.modelMatrix * float4(PositionBuffer[vertexID].xyz, 1.0);
    out.color = unpack_unorm4x8_to_float(uniforms.color);
    return out;
}

fragment float4 debugFragmentShader(VertexOut in [[stage_in]])
{
    return in.color;
}
