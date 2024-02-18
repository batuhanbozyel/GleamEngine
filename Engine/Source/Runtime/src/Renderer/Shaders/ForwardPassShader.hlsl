#include "Common.hlsli"
#include "ShaderTypes.h"

StructuredBuffer<float3> PositionBuffer : register(t0);

StructuredBuffer<Gleam::InterleavedMeshVertex> InterleavedBuffer : register(t1);

ConstantBuffer<Gleam::CameraUniforms> CameraBuffer : register(b0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

PUSH_CONSTANT(Gleam::ForwardPassUniforms, uniforms);

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(PositionBuffer[vertex_id].xyz, 1.0f)));
    OUT.normal = interleavedVert.normal.xyz;
    OUT.texCoord = interleavedVert.texCoord;
    return OUT;
}

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return float4(1.0f, 0.71f, 0.75f, 1.0f);
}
