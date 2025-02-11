#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, 0);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, 1);

struct MeshVertexOut
{
	float4 position : SV_POSITION;
	float3 normal : ATTRIB0;
	float4 tangent : ATTRIB1;
	float4 color : ATTRIB2;
	float2 uv : ATTRIB3;
};

#pragma fragment meshShadingPassShader

// User defined
Gleam::SurfaceOutput surf(MeshVertexOut IN);

float4 meshShadingPassShader(MeshVertexOut IN) : SV_TARGET
{
    Gleam::SurfaceOutput surface = surf(IN);
	
	float3 bitangent = normalize(cross(IN.normal, IN.tangent.xyz)) * sign(IN.tangent.w);
	float3x3 TBN = transpose(float3x3(IN.tangent.xyz, bitangent, IN.normal));
	float3 worldNormal = normalize(mul(TBN, surface.normal));
    
	float3 color = surface.albedo.rgb;
    float3 ambient = color * 0.1f;
	float3 lightDir = normalize(float3(0.43f, 0.43f, 0.0f));
	float3 diffuse = color * max(dot(worldNormal, lightDir), 0.0f);
    color = diffuse + ambient;
	return float4(color, 1.0f);
}
