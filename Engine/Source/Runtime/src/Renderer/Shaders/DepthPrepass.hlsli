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
    float4 prevClipPos : ATTRIB0;
    float4 color : ATTRIB1;
    float2 uv : ATTRIB2;
    float3 normal : ATTRIB3;
};

struct DepthPrepassFragmentOut
{
    float2 motionVector : SV_TARGET0;
    float2 normal : SV_TARGET1;
#ifdef VISIBILITY_SHADING_PATH
    PackedVisibilityID visID : SV_TARGET2;
#endif // VISIBILITY_SHADING_PATH
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
    return prevViewport - IN.position.xy;
}

#ifdef VISIBILITY_SHADING_PATH
Gleam::DepthPrepassFragmentOut BuildDepthPrepassOutput(Gleam::DepthPrepassVertexOut IN, float2 resolution, PackedVisibilityID visID)
{
    Gleam::DepthPrepassFragmentOut OUT;
    OUT.motionVector = ComputeMotionVector(IN, resolution);
    OUT.normal = OctEncode(normalize(IN.normal));
    OUT.visID = visID;
    return OUT;
}

[shader("pixel")]
Gleam::DepthPrepassFragmentOut main(Gleam::DepthPrepassVertexOut IN, Gleam::VisibilityPrimOut prim)
{
    Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    Gleam::MeshVertexOut meshVertexOut = (Gleam::MeshVertexOut)0;
    meshVertexOut.color = IN.color;
    meshVertexOut.uv = IN.uv;
    meshVertexOut.ddxUV = ddx(IN.uv);
    meshVertexOut.ddyUV = ddy(IN.uv);

    Gleam::SurfaceOutput surface = SurfMain(meshVertexOut);
    clip(surface.albedo.a - surface.alphaCutoff);

    return BuildDepthPrepassOutput(IN, camera.resolution, prim.visID);
}
#else
Gleam::DepthPrepassFragmentOut BuildDepthPrepassOutput(Gleam::DepthPrepassVertexOut IN, float2 resolution)
{
    Gleam::DepthPrepassFragmentOut OUT;
    OUT.motionVector = ComputeMotionVector(IN, resolution);
    OUT.normal = OctEncode(normalize(IN.normal));
    return OUT;
}

[shader("pixel")]
Gleam::DepthPrepassFragmentOut main(Gleam::DepthPrepassVertexOut IN)
{
    Gleam::MeshInstanceData instance = LoadInstanceData(constants.instanceID);
    Gleam::MeshVertexOut meshVertexOut = (Gleam::MeshVertexOut)0;
    meshVertexOut.color = IN.color;
    meshVertexOut.uv = IN.uv;
    meshVertexOut.ddxUV = ddx(IN.uv);
    meshVertexOut.ddyUV = ddy(IN.uv);

    Gleam::SurfaceOutput surface = SurfMain(meshVertexOut);
    clip(surface.albedo.a - surface.alphaCutoff);

    return BuildDepthPrepassOutput(IN, camera.resolution);
}
#endif // VISIBILITY_SHADING_PATH

#endif // DEPTH_PREPASS_HLSL
