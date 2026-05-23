#ifndef DEPTH_PREPASS_HLSL
#define DEPTH_PREPASS_HLSL

#include "Common.hlsli"
#include "SurfaceShading.hlsli"

PUSH_CONSTANT(Gleam::DepthPrepassConstants, constants);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

namespace Gleam
{
struct DepthPrepassVertexOut
{
    float4 position : SV_POSITION;
    float4 prevClipPos : ATTRIB0;
    float4 color : ATTRIB1;
    float2 uv : ATTRIB2;
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

float2 ComputeMotionVector(Gleam::DepthPrepassVertexOut IN, float2 resolution)
{
    float2 prevNDC = IN.prevClipPos.xy / IN.prevClipPos.w;
    float2 prevViewport = (prevNDC * float2(0.5f, -0.5f) + 0.5f) * resolution;
    return IN.position.xy - prevViewport;
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

    return ComputeMotionVector(IN, camera.resolution);
}
#endif // DEPTH_PREPASS_HLSL
