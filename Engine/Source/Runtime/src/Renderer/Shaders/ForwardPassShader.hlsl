#include "Common.hlsli"
#include "ShaderTypes.h"

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, 0);
PUSH_CONSTANT(Gleam::ForwardPassUniforms, uniforms);

VertexOut forwardPassVertexShader(uint vertex_id: SV_VertexID)
{
    uint vertexID = vertex_id + uniforms.baseVertex;
    Gleam::CameraUniforms CameraBuffer = resources.cameraBuffer.Load<Gleam::CameraUniforms>();
    Gleam::InterleavedMeshVertex interleavedVert = resources.interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID);
    float3 position = resources.positionBuffer.Load<float3>(vertexID);

    VertexOut OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(position, 1.0f)));
    OUT.normal = normalize(mul(uniforms.modelMatrix, float4(interleavedVert.normal, 0.0f)).xyz);
    OUT.texCoord = interleavedVert.texCoord;
    return OUT;
}

float4 forwardPassFragmentShader(VertexOut IN) : SV_TARGET
{
    float3 color = float3(IN.texCoord.x, IN.texCoord.y, 0.0f);
    float3 ambient = color * 0.05f;
    float3 lightDir = float3(0.43f, 0.43f, 0.0f);
    float3 diffuse = color * max(dot(normalize(IN.normal), lightDir), 0.0f);
    color = diffuse + ambient;
    return float4(color.x, color.y, color.z, 1.0f);
}
