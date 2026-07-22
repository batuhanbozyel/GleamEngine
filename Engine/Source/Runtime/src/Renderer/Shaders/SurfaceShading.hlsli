#ifndef SURFACE_SHADING_HLSL
#define SURFACE_SHADING_HLSL

#include "ShaderInterop.h"
#include "ShaderTypes.h"

namespace Gleam {

struct MeshVertexOut
{
    float4 position : SV_POSITION;
    float3 worldPosition : ATTRIB0;
    float3 normal : ATTRIB1;
    float3 tangent : ATTRIB2;
    float3 bitangent : ATTRIB3;
    float4 color : ATTRIB4;
    float2 uv : ATTRIB5;
    float2 ddxUV : ATTRIB6;
    float2 ddyUV : ATTRIB7;
#ifdef MOTION_VECTOR_PASS
    float3 prevWorldPosition : ATTRIB8;
#endif // MOTION_VECTOR_PASS
};

struct SurfaceOutput
{
    float4 albedo;
    float4 emission;
    float3 normal;
    float metallic;
    float roughness;
    float occlusion;
    float alphaCutoff;
};

} // namespace Gleam

// Auto-generated inside material shader — loads material instance data into Material
void LoadMaterialInstance(ByteAddressBuffer materialBuffer, uint materialID);

// User defined — implemented in the material's .shader file
Gleam::SurfaceOutput SurfMain(Gleam::MeshVertexOut IN);

#endif
