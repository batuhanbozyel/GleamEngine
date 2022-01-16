struct VertexIn
{
    float3 position : POS;
};

struct VertexOut
{
    float4 position : SV_POSITION;
};

VertexOut triangleVertexShader(VertexIn input)
{
    VertexOut output;
    output.position = float4(input.position, 1.0);
    return output;
}

float4 triangleFragmentShader(VertexOut input) : SV_TARGET
{
    return float4(1.0, 0.0, 1.0, 1.0);
}