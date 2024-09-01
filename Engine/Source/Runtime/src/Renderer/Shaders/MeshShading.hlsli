#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, 0);
PUSH_CONSTANT(Gleam::ForwardPassUniforms, uniforms);

#pragma vertex meshVertexShader
#pragma fragment meshShadingPassShader

// User defined
SurfaceOutput surf(SurfaceInput IN);

SurfaceInput meshVertexShader(uint vertex_id: SV_VertexID)
{
    uint vertexID = vertex_id + uniforms.baseVertex;
    Gleam::CameraUniforms CameraBuffer = resources.cameraBuffer.Load<Gleam::CameraUniforms>();
    Gleam::InterleavedMeshVertex interleavedVert = resources.interleavedBuffer.Load<Gleam::InterleavedMeshVertex>(vertexID);
    float3 position = resources.positionBuffer.Load<float3>(vertexID);

    SurfaceInput OUT;
    OUT.position = mul(CameraBuffer.viewProjectionMatrix, mul(uniforms.modelMatrix, float4(position, 1.0f)));
    OUT.worldNormal = normalize(mul(uniforms.modelMatrix, float4(interleavedVert.normal, 0.0f)).xyz);
    OUT.color = float3(interleavedVert.texCoord.x, interleavedVert.texCoord.y, 0.0f);
    OUT.uv = interleavedVert.texCoord;
    return OUT;
}

float4 meshShadingPassShader(SurfaceInput IN) : SV_TARGET
{
    SurfaceOutput surface = surf(IN);

    float3 color = surface.albedo.rgb;
    float3 ambient = color * 0.05f;
    float3 lightDir = float3(0.43f, 0.43f, 0.0f);
    float3 diffuse = color * max(dot(normalize(IN.worldNormal), lightDir), 0.0f);
    color = diffuse + ambient;
    return float4(color.x, color.y, color.z, 1.0f);
}