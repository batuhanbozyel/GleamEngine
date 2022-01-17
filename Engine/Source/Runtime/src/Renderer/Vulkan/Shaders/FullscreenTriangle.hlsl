struct VertexOut
{
    float4 position : SV_POSITION;
};

VertexOut fullscreenTriangleVertexShader(uint vertex_id: SV_VertexID)
{
    VertexOut out;
    float2 uv = float2((id << 1) & 2, id & 2);
    out.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return out;
}

float4 fullscreenTriangleFragmentShader(VertexOut input) : SV_TARGET
{
    return float4(1.0, 0.0, 1.0, 1.0);
}
