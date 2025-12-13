#include "BRDF.hlsli"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, 0);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, 1);

// We only need this for legacy vertex shader path
// When switched to mesh shaders, we should be fetching instance data from instance buffer
CONSTANT_BUFFER(Gleam::MeshInstanceData, instanceData, 2);

struct MeshVertexOut
{
	float4 position : SV_POSITION;
	float3 worldPosition : ATTRIB0;
	float3 normal : ATTRIB1;
	float3 tangent : ATTRIB2;
	float3 bitangent : ATTRIB3;
	float4 color : ATTRIB4;
	float2 uv : ATTRIB5;
};

#pragma fragment meshShadingPassShader

// User defined
Gleam::SurfaceOutput surf(MeshVertexOut IN);

float4 meshShadingPassShader(MeshVertexOut IN) : SV_TARGET
{
    Gleam::SurfaceOutput surface = surf(IN);
    
	float3 viewDir = normalize(camera.position - IN.worldPosition);
	float3x3 TBN = transpose(float3x3(IN.tangent, IN.bitangent, IN.normal));
	float3 worldNormal = normalize(mul(TBN, surface.normal));
	
	//return float4(IN.tangent * 0.5 + 0.5, 1.0f);
	//return float4(IN.normal * 0.5 + 0.5, 1.0f);
	//return float4(IN.bitangent * 0.5 + 0.5, 1.0f);
	//return float4(worldNormal * 0.5 + 0.5, 1.0f);
	//return float4(surface.normal * 0.5 + 0.5, 1.0f);
	//return float4(surface.metallic, surface.roughness, 0.0f, 1.0f);
	//return float4(surface.metallic.xxx, 1.0f);
	//return float4(surface.roughness.xxx, 1.0f);
    
	DirectLight light;
	light.direction = resources.sun.direction;
	light.illuminance = resources.sun.illuminance;
	float3 color = EvaluateDirectLight(surface, light, viewDir, worldNormal);
	return float4(color, 1.0f);
}
