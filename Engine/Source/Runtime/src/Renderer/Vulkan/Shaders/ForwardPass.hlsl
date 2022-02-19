struct Vertex
{
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

StructuredBuffer<Vertex> VertexBuffer : register(t0);

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    Vertex vertex = VertexBuffer[vertex_id];

    VertexOut OUT;
    OUT.position = float4(vertex.px, vertex.py, vertex.pz, 1.0);
    OUT.normal = float3(vertex.nx, vertex.ny, vertex.nz);
    OUT.texCoord = float2(vertex.u, vertex.v);
    return OUT;
}

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    return float4(IN.texCoord, 0.0, 1.0);
}
