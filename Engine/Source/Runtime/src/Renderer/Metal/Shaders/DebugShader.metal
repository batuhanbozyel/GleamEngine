#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

struct VertexOut
{
    float4 position [[position]];
    float4 color;
};

vertex VertexOut debugVertexShader(uint vertexID [[vertex_id]],
                                   constant Gleam::DebugVertex* VertexBuffer [[buffer(Gleam::RendererBindingTable::Buffer0)]],
                                   constant Gleam::DebugShaderUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    Gleam::DebugVertex vert = VertexBuffer[vertexID];

    VertexOut out;
    out.position = uniforms.camera.viewProjectionMatrix * float4(vert.position.xyz, 1.0);
    out.color = unpack_unorm4x8_to_float(vert.color);
    return out;
}

vertex VertexOut debugMeshVertexShader(uint vertexID [[vertex_id]],
                                       constant Gleam::Vector3* PositionBuffer [[buffer(Gleam::RendererBindingTable::Buffer0)]],
                                       constant Gleam::CameraUniforms& CameraBuffer [[buffer(Gleam::RendererBindingTable::CameraBuffer)]],
                                       constant Gleam::DebugShaderUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    VertexOut out;
    out.position = uniforms.camera.viewProjectionMatrix * uniforms.modelMatrix * float4(PositionBuffer[vertexID].xyz, 1.0);
    out.color = float4(1.0);
    return out;
}

fragment float4 debugFragmentShader(VertexOut in [[stage_in]],
                                    constant Gleam::DebugShaderUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    return in.color * unpack_unorm4x8_to_float(uniforms.color);
}
