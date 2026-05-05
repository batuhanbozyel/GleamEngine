#ifndef DEPTH_PREPASS_HLSL
#define DEPTH_PREPASS_HLSL

#include "Common.hlsli"
#include "SurfaceShading.hlsli"

PUSH_CONSTANT(Gleam::DepthPrepassConstants, constants);

namespace Gleam
{
struct DepthPrepassVertexOut
{
    float4 position : SV_POSITION;
    float4 currentClipPos : ATTRIB0;
    float4 prevClipPos : ATTRIB1;
    float4 color : ATTRIB4;
    float2 uv : ATTRIB5;
};

} // namespace Gleam

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
float2 main(Gleam::DepthPrepassVertexOut IN) : SV_TARGET
{
    Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    
    Gleam::MeshVertexOut meshVertexOut = (Gleam::MeshVertexOut)0;
    meshVertexOut.color = IN.color;
    meshVertexOut.uv = IN.uv;
    meshVertexOut.ddxUV = ddx(IN.uv);
    meshVertexOut.ddyUV = ddy(IN.uv);
    
#ifdef DEPTH_PREPASS_SURFACE_SHADING
    Gleam::SurfaceOutput surface = SurfMain(IN);
    clip(surface.albedo.a - surface.alphaCutoff);
#endif
    
    float2 currentNDC = IN.currentClipPos.xy / IN.currentClipPos.w;
    float2 prevNDC = IN.prevClipPos.xy / IN.prevClipPos.w;
    return (currentNDC - prevNDC) * float2(0.5f, -0.5f);
}
#endif // DEPTH_PREPASS_HLSL
