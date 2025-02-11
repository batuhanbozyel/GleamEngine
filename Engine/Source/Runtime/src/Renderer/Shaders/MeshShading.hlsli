#include "BRDF.hlsli"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, 0);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, 1);

struct MeshVertexOut
{
	float4 position : SV_POSITION;
	float3 worldPosition : ATTRIB0;
	float3 normal : ATTRIB1;
	float4 tangent : ATTRIB2;
	float4 color : ATTRIB3;
	float2 uv : ATTRIB4;
};

#pragma fragment meshShadingPassShader

// User defined
Gleam::SurfaceOutput surf(MeshVertexOut IN);

float4 meshShadingPassShader(MeshVertexOut IN) : SV_TARGET
{
    Gleam::SurfaceOutput surface = surf(IN);
	
	float3 viewDir = normalize(camera.position - IN.worldPosition);
	float3 lightDir = normalize(float3(0.43f, 0.43f, 0.0f));
	
	float3 bitangent = normalize(cross(IN.normal, IN.tangent.xyz)) * sign(IN.tangent.w);
	float3x3 TBN = transpose(float3x3(IN.tangent.xyz, bitangent, IN.normal));
	float3 worldNormal = normalize(mul(TBN, surface.normal));
    
	float3 color = EvaluateDirectLight(surface, lightDir, viewDir, worldNormal);
	float3 ambient = surface.albedo.rgb * 0.1f;
	return float4(color + ambient, 1.0f);
}
