#include "../../ShaderTypes.h"

StructuredBuffer<Gleam::DebugVertex> VertexBuffer : register(t0);

struct VertexOut
{
    float4 position : SV_POSITION;
    uint color;
};

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::DebugVertex vertex = VertexBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = float4(vertex.position, 1.0);
    OUT.color = vertex.color;
    return OUT;
}

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return unpack_unorm4x8_to_float(IN.color);
}
