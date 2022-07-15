#include "../../ShaderTypes.h"

struct VertexOut
{
    float4 position [[position]];
    uint color;
};

vertex VertexOut debugVertexShader(uint vertexID [[vertex_id]],
                                   constant Gleam::DebugVertex* VertexBuffer [[buffer(0)]])
{
    Gleam::DebugVertex vert = VertexBuffer[vertexID];

    VertexOut out;
    out.position = float4(vert.position, 1.0);
    out.color = vert.color;
    return out;
}

fragment float4 debugFragmentShader(VertexOut in [[stage_in]])
{
    return unpack_unorm4x8_to_float(in.color);
}
