#include "Common.hlsli"

FScreenVertexOutput fullscreenTriangleVertexShader(uint vertex_id: SV_VertexID)
{
    FScreenVertexOutput OUT;
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    OUT.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    OUT.texCoord = uv;
    return OUT;
}

float4 fullscreenTriangleFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    return float4(IN.texCoord, 0.0, 1.0);
}