#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

struct VertexOut
{
    float4 position [[position]];
    float3 normal;
    float2 texCoord;
};

vertex VertexOut forwardPassVertexShader(uint vertexID [[vertex_id]],
                                         constant Gleam::Vector3* PositionBuffer [[buffer(Gleam::RendererBindingTable::PositionBuffer)]],
                                         constant Gleam::InterleavedMeshVertex* InterleavedBuffer [[buffer(Gleam::RendererBindingTable::InterleavedBuffer)]],
                                         constant Gleam::CameraUniforms& cameraUniforms [[buffer(Gleam::RendererBindingTable::CameraBuffer)]],
                                         constant Gleam::ForwardPassUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertexID];

    VertexOut out;
    out.position = cameraUniforms.viewProjectionMatrix * uniforms.modelMatrix * float4(PositionBuffer[vertexID].xyz, 1.0);
    out.normal = interleavedVert.normal.xyz;
    out.texCoord = interleavedVert.texCoord;
    return out;
}

fragment float4 forwardPassFragmentShader(VertexOut in [[stage_in]],
                                          constant Gleam::ForwardPassUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    return float4(1.0f, 0.71f, 0.75f, 1.0f);
}
