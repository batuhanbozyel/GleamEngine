struct VertexOut
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut fullscreenTriangleVertexShader(uint vertex_id: SV_VertexID)
{
    VertexOut vOut;
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    vOut.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    vOut.texCoord = uv;
    return vOut;
}

float4 fullscreenTriangleFragmentShader(VertexOut input) : SV_TARGET
{
    return float4(input.texCoord, 0.0, 1.0);
}
