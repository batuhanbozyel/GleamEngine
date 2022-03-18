#include "../../GraphicsTypes.h"

StructuredBuffer<Gleam::Vertex> VertexBuffer : register(t0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::Vertex vertex = VertexBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = float4(vertex.position, 1.0);
    OUT.normal = vertex.normal;
    OUT.texCoord = vertex.texCoord;
    return OUT;
}

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return float4(IN.texCoord, 0.0, 1.0);
}