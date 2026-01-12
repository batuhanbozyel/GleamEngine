#ifndef MESH_SHADING_HLSL
#define MESH_SHADING_HLSL

#include "BRDF.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, MESH_PASS_RESOURCES_BINDING_SLOT);

// We only need this for legacy vertex shader path
// When switched to mesh shaders, we should be fetching instance data from instance buffer
CONSTANT_BUFFER(Gleam::MeshInstanceData, instanceData, MESH_INSTANCE_DATA_BINDING_SLOT);

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
	if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex && atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
	{
		const float3 planetCenterWorld = float3(0.0f, -atmosphereParams.bottomRadius, 0.0f);
		const float3 worldPositionInKM = IN.worldPosition * M_TO_KM;
		
		light.direction = atmosphereUniforms.sunDirection;
		light.illuminance = GetSunLuminance(worldPositionInKM - planetCenterWorld, atmosphereUniforms.sunDirection);
	}
	else
	{
		light.direction = atmosphereUniforms.sunDirection;
		light.illuminance = atmosphereUniforms.sunIlluminance;
	}
	float3 color = EvaluateDirectLight(surface, light, viewDir, worldNormal);
	return float4(color, 1.0f);
}
#endif // MESH_SHADING_HLSL