#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, 0);

struct MeshVertexOut
{
	float4 position : SV_POSITION;
	float3 worldNormal : NORMAL;
	float3 color : COLOR;
	float2 uv : TEXCOORD0;
};

#pragma fragment meshShadingPassShader

// User defined
Gleam::SurfaceOutput surf(MeshVertexOut IN);

float4 meshShadingPassShader(MeshVertexOut IN) : SV_TARGET
{
    Gleam::SurfaceOutput surface = surf(IN);

    float3 color = 1.0;
    float3 ambient = color * 0.05f;
    float3 lightDir = float3(0.43f, 0.43f, 0.0f);
    float3 diffuse = color * max(dot(normalize(IN.worldNormal), lightDir), 0.0f);
    color = diffuse + ambient;
    return float4(color.x, color.y, color.z, 1.0f);
}
