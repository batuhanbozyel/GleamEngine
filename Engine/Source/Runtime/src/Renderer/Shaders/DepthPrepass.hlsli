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

float2 ComputeMotionVector(Gleam::DepthPrepassVertexOut IN)
{
    float2 currentNDC = IN.currentClipPos.xy / IN.currentClipPos.w;
    float2 prevNDC = IN.prevClipPos.xy / IN.prevClipPos.w;
    return (currentNDC - prevNDC) * float2(0.5f, -0.5f);
}

[shader("pixel")]
float2 main(Gleam::DepthPrepassVertexOut IN) : SV_TARGET
{
    Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    Gleam::MeshVertexOut meshVertexOut = (Gleam::MeshVertexOut)0;
    meshVertexOut.color = IN.color;
    meshVertexOut.uv = IN.uv;
    meshVertexOut.ddxUV = ddx(IN.uv);
    meshVertexOut.ddyUV = ddy(IN.uv);

    Gleam::SurfaceOutput surface = SurfMain(meshVertexOut);
    clip(surface.albedo.a - surface.alphaCutoff);

    return ComputeMotionVector(IN);
}
#endif // DEPTH_PREPASS_HLSL
