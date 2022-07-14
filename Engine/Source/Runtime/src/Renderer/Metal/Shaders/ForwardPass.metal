#include "../../ShaderTypes.h"

struct VertexOut
{
    float4 position [[position]];
    float3 normal;
    float2 texCoord;
};

vertex VertexOut forwardPassVertexShader(uint vertexID [[vertex_id]],
                                         constant Gleam::MeshVertex* VertexBuffer [[buffer(0)]])
{
    Gleam::MeshVertex vert = VertexBuffer[vertexID];

    VertexOut out;
    out.position = float4(vert.position, 1.0);
    out.normal = vert.normal;
    out.texCoord = vert.texCoord;
    return out;
}

fragment float4 forwardPassFragmentShader(VertexOut in [[stage_in]],
                                          constant Gleam::ForwardPassFragmentUniforms& uniforms [[buffer(0)]])
{
    return uniforms.color;
}
