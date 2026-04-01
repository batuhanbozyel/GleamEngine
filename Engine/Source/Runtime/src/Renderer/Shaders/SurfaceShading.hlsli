#ifndef SURFACE_SHADING_HLSL
#define SURFACE_SHADING_HLSL

// Auto-generated inside material shader — loads material instance data into Material
void LoadMaterialInstance(uint materialID);

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

// User defined — implemented in the material's .shader file
Gleam::SurfaceOutput surf(MeshVertexOut IN);

#endif