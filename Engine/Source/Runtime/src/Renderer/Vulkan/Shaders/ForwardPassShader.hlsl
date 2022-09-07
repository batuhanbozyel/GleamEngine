#include "../../ShaderTypes.h"

StructuredBuffer<Gleam::Vector3> PositionBuffer : register(t0);
StructuredBuffer<Gleam::InterleavedMeshVertex> InterleavedBuffer : register(t0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

[[vk::push_constant]]
Gleam::CameraUniforms cameraUniforms;

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = mul(cameraUniforms.modelViewProjectionMatrix, float4(PositionBuffer[vertex_id], 1.0));
    OUT.normal = mul(cameraUniforms.normalMatrix, interleavedVert.normal);
    OUT.texCoord = interleavedVert.texCoord;
    return OUT;
}

[[vk::push_constant]]
Gleam::ForwardPassFragmentUniforms fragmentUniforms;

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return Gleam::unpack_unorm4x8_to_float(fragmentUniforms.color);
}
