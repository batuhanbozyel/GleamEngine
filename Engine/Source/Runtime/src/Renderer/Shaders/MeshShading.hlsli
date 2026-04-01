#ifndef MESH_SHADING_HLSL
#define MESH_SHADING_HLSL

#include "BRDF.hlsli"
#include "SurfaceShading.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

CONSTANT_BUFFER(Gleam::MeshPassResources, resources, MESH_PASS_RESOURCES_BINDING_SLOT);
PUSH_CONSTANT(Gleam::MeshShadingConstants, constants);

[shader("pixel")]
float4 meshShadingPassShader(MeshVertexOut IN) : SV_TARGET
{
	ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[resources.instanceBuffer];
	Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));
	LoadMaterialInstance(instanceData.materialID);

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
		light.direction = atmosphereUniforms.sunDirection;
		light.illuminance = GetSunLuminance(GetSkyWorldPosition(IN.worldPosition), atmosphereUniforms.sunDirection);
	}
	else
	{
		light.direction = atmosphereUniforms.sunDirection;
		light.illuminance = atmosphereUniforms.sunIlluminance;
	}
	
	float3 color = 0.0;
	color += EvaluateDirectLight(surface, light, viewDir, worldNormal);
	color += EvaluateIndirectLight(surface, resources.brdfTexture, resources.diffuseReflectionTexture, resources.specularReflectionTexture, viewDir, worldNormal);
	return float4(color, 1.0f);
}
#endif // MESH_SHADING_HLSL