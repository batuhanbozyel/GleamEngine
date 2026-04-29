#ifndef DEPTH_PREPASS_HLSL
#define DEPTH_PREPASS_HLSL

#include "Common.hlsli"
#include "SurfaceShading.hlsli"

PUSH_CONSTANT(Gleam::DepthPrepassConstants, constants);

Gleam::MeshInstanceData LoadInstanceData(uint instanceID)
{
	ByteAddressBuffer instanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
	Gleam::MeshInstanceData instance = instanceBuffer.Load<Gleam::MeshInstanceData>(instanceID * sizeof(Gleam::MeshInstanceData));

	ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[instance.materialBuffer];
	LoadMaterialInstance(materialBuffer, instance.materialID);
	return instance;
}

[shader("pixel")]
void main(Gleam::MeshVertexOut IN) : SV_TARGET
{
    IN.ddxUV = ddx(IN.uv);
    IN.ddyUV = ddy(IN.uv);
    Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    Gleam::SurfaceOutput surface = SurfMain(IN);

    clip(surface.albedo.a - surface.alphaCutoff);
}
#endif // DEPTH_PREPASS_HLSL
