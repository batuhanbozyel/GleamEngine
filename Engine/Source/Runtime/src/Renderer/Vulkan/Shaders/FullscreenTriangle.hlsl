struct VertexOut
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut fullscreenTriangleVertexShader(uint vertex_id: SV_VertexID)
{
    VertexOut OUT;
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    OUT.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    OUT.texCoord = uv;
    return OUT;
}

float4 fullscreenTriangleFragmentShader(VertexOut IN) : SV_TARGET
{
    return float4(IN.texCoord, 0.0, 1.0);
}
