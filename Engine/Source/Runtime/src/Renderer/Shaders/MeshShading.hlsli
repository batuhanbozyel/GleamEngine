#ifndef MESH_SHADING_HLSL
#define MESH_SHADING_HLSL

#include "MeshLighting.hlsli"

PUSH_CONSTANT(Gleam::MeshShadingConstants, constants);

Gleam::MeshInstanceData LoadInstanceData(uint instanceID)
{
	ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
	Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[instance.materialBuffer];
	LoadMaterialInstance(materialBuffer, instance.materialID);
	return instance;
}

[shader("pixel")]
[earlydepthstencil]
float4 main(Gleam::MeshVertexOut IN) : SV_TARGET
{
	IN.ddxUV = ddx(IN.uv);
	IN.ddyUV = ddy(IN.uv);
	Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    Gleam::SurfaceOutput surface = SurfMain(IN);
    surface.roughness = max(surface.roughness, 0.04);
    
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
    
	float3 color = EvaluateMeshLighting(surface,
										IN.worldPosition,
										worldNormal,
										viewDir,
										uint2(IN.position.xy),
										constants.brdfTexture,
										constants.ggxEssTexture,
										constants.ggxEAvgTexture,
										constants.diffuseReflectionTexture,
										constants.specularReflectionTexture,
										constants.reflectionTexture,
										constants.shadowTexture,
										constants.aoTexture);
	return float4(color, surface.albedo.a);
}
#endif // MESH_SHADING_HLSL
