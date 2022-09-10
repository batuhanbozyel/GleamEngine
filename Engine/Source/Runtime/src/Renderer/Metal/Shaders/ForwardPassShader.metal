#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

struct VertexOut
{
    float4 position [[position]];
    float3 normal;
    float2 texCoord;
};

vertex VertexOut forwardPassVertexShader(uint vertexID [[vertex_id]],
                                         constant Gleam::Vector3* PositionBuffer [[buffer(Gleam::RendererBindingTable::Buffer0)]],
                                         constant Gleam::InterleavedMeshVertex* InterleavedBuffer [[buffer(Gleam::RendererBindingTable::Buffer1)]],
                                         constant Gleam::CameraUniforms& cameraUniforms [[buffer(Gleam::RendererBindingTable::CameraBuffer)]],
                                         constant Gleam::ForwardPassUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertexID];

    VertexOut out;
    out.position = cameraUniforms.viewProjectionMatrix * uniforms.modelMatrix * float4(PositionBuffer[vertexID], 1.0);
    out.normal = interleavedVert.normal;
    out.texCoord = interleavedVert.texCoord;
    return out;
}

fragment float4 forwardPassFragmentShader(VertexOut in [[stage_in]],
                                          constant Gleam::ForwardPassUniforms& uniforms [[buffer(Gleam::RendererBindingTable::PushConstantBlock)]])
{
    return unpack_unorm4x8_to_float(uniforms.color);
}
