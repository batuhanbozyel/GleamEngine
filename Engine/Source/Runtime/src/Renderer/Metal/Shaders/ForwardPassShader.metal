#include "../../ShaderTypes.h"

struct VertexOut
{
    float4 position [[position]];
    float3 normal;
    float2 texCoord;
};

vertex VertexOut forwardPassVertexShader(uint vertexID [[vertex_id]],
                                         constant Gleam::Vector3* PositionBuffer [[buffer(0)]],
                                         constant Gleam::InterleavedMeshVertex* InterleavedBuffer [[buffer(1)]])
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertexID];

    VertexOut out;
    out.position = float4(PositionBuffer[vertexID], 1.0);
    out.normal = interleavedVert.normal;
    out.texCoord = interleavedVert.texCoord;
    return out;
}

fragment float4 forwardPassFragmentShader(VertexOut in [[stage_in]],
                                          constant Gleam::ForwardPassFragmentUniforms& uniforms [[buffer(0)]])
{
    return unpack_unorm4x8_to_float(uniforms.color);
}
