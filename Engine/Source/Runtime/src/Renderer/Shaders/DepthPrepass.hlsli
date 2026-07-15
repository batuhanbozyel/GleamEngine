#ifndef DEPTH_PREPASS_HLSL
#define DEPTH_PREPASS_HLSL

#include "Common.hlsli"
#include "SurfaceShading.hlsli"
#include "VisibilityBufferCommon.hlsli"

PUSH_CONSTANT(Gleam::DepthPrepassConstants, constants);

namespace Gleam {

struct DepthPrepassVertexOut
{
    float4 position : SV_POSITION;
#ifndef OPAQUE_DEPTH_PREPASS
    float4 color : ATTRIB0;
    float2 uv : ATTRIB1;
#endif // OPAQUE_DEPTH_PREPASS
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
PackedVisibilityID main(Gleam::DepthPrepassVertexOut IN, Gleam::VisibilityPrimOut prim) : SV_Target0
{
    Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    Gleam::MeshVertexOut meshVertexOut = (Gleam::MeshVertexOut)0;
    meshVertexOut.color = IN.color;
    meshVertexOut.uv = IN.uv;
    meshVertexOut.ddxUV = ddx(IN.uv);
    meshVertexOut.ddyUV = ddy(IN.uv);

    Gleam::SurfaceOutput surface = SurfMain(meshVertexOut);
    clip(surface.albedo.a - surface.alphaCutoff);

    return prim.visID;
}

#endif // DEPTH_PREPASS_HLSL
