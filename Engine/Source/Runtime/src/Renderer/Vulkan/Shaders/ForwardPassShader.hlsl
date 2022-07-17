#include "../../ShaderTypes.h"

StructuredBuffer<Gleam::MeshVertex> VertexBuffer : register(t0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::MeshVertex vertex = VertexBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = float4(vertex.position, 1.0);
    OUT.normal = vertex.normal;
    OUT.texCoord = vertex.texCoord;
    return OUT;
}

[[vk::push_constant]]
Gleam::ForwardPassFragmentUniforms uniforms;

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return Gleam::unpack_unorm4x8_to_float(uniforms.color);
}
