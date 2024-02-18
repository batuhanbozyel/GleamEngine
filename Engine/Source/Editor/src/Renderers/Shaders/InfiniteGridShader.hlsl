#include "Common.hlsli"
#include "ShaderTypes.h"
#include "../ShaderTypes.h"

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

ConstantBuffer<Gleam::CameraUniforms> CameraBuffer : register(b0);
PUSH_CONSTANT(GEditor::InfiniteGridUniforms, uniforms);

VertexOut infiniteGridVertexShader(uint vertex_id: SV_VertexID)
{
    static const float3 gridPlane[6] = {
        float3(1, 1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
        float3(-1, -1, 0), float3(1, 1, 0), float3(1, -1, 0)
    };
    
    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, float4(gridPlane[vertex_id], 1.0f));
    OUT.color = unpack_unorm4x8_to_float(uniforms.color);
    return OUT;
}

float4 infiniteGridFragmentShader(VertexOut IN) : SV_TARGET
{
    return IN.color;
}

