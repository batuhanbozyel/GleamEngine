#include "../../ShaderTypes.h"

[[vk::binding(Gleam::RendererBindingTable::Buffer0)]]
StructuredBuffer<Gleam::Vector3> PositionBuffer;

[[vk::binding(Gleam::RendererBindingTable::Buffer1)]]
StructuredBuffer<Gleam::InterleavedMeshVertex> InterleavedBuffer;

[[vk::binding(Gleam::RendererBindingTable::CameraBuffer)]]
cbuffer<Gleam::CameraUniforms> CameraBuffer;

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

[[vk::push_constant]]
Gleam::ForwardPassUniforms uniforms;

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(PositionBuffer[vertex_id], 1.0f)));
    OUT.normal = interleavedVert.normal;
    OUT.texCoord = interleavedVert.texCoord;
    return OUT;
}

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return Gleam::unpack_unorm4x8_to_float(uniforms.color);
}
