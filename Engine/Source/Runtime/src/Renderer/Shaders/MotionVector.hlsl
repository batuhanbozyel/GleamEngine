#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::MotionVectorConstants, constants);

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);

namespace Gleam {
struct MotionVectorVertexOut
{
    float4 position : SV_POSITION;
    float4 currentClipPos : ATTRIB0;
    float4 prevClipPos : ATTRIB1;
};
}

[shader("vertex")]
Gleam::MotionVectorVertexOut motionVectorVertexShader(uint vertex_id : SV_VertexID)
{
    ByteAddressBuffer globalInstanceBuffer = ResourceDescriptorHeap[constants.instanceBuffer];
    Gleam::MeshInstanceData instanceData = globalInstanceBuffer.Load<Gleam::MeshInstanceData>(constants.instanceID * sizeof(Gleam::MeshInstanceData));

    ByteAddressBuffer positionBuffer = ResourceDescriptorHeap[instanceData.positionBuffer];
    uint vertexID = vertex_id + instanceData.baseVertex;
    float3 position = positionBuffer.Load<float3>(vertexID * sizeof(float3));

    float4 worldPosition = mul(instanceData.transform, float4(position, 1.0f));
    float4 prevWorldPosition = mul(instanceData.previousTransform, float4(position, 1.0f));

    Gleam::MotionVectorVertexOut OUT;
    OUT.currentClipPos = mul(camera.viewProjectionMatrix, worldPosition);
    OUT.prevClipPos = mul(camera.prevViewProjectionMatrix, prevWorldPosition);
    OUT.position = OUT.currentClipPos;
    return OUT;
}

[shader("pixel")]
float2 motionVectorPixelShader(Gleam::MotionVectorVertexOut IN) : SV_TARGET
{
    float2 currentNDC = IN.currentClipPos.xy / IN.currentClipPos.w;
    float2 prevNDC = IN.prevClipPos.xy / IN.prevClipPos.w;
    return (currentNDC - prevNDC) * float2(0.5f, -0.5f);
}
