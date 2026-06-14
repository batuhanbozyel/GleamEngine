#include "Common.hlsli"
#include "../ViewMode.h"
#include "../ShaderTypes.h"

PUSH_CONSTANT(GEditor::ViewModeUniforms, uniforms);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, 0);

// Reconstruct view-space Z from device depth using the inverse projection.
float LinearizeViewZ(float deviceDepth, float4x4 invProjectionMatrix)
{
    float4 viewPos = mul(invProjectionMatrix, float4(0.0, 0.0, deviceDepth, 1.0));
    return viewPos.z / viewPos.w;
}

[shader("pixel")]
float4 viewModeFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    float3 color = float3(0.0, 0.0, 0.0);

    switch ((Gleam::ViewMode)uniforms.mode)
    {
        case Gleam::ViewMode::WorldNormal:
        {
            Texture2D<float2> normalTexture = ResourceDescriptorHeap[uniforms.sourceTexture];
            float2 encoded = normalTexture.Sample(Sampler_Point_Clamp, IN.texCoord);
            float3 normal = OctDecode(encoded);
            color = normal * 0.5 + 0.5;
            break;
        }
        case Gleam::ViewMode::Depth:
        {
            Texture2D<float> depthTexture = ResourceDescriptorHeap[uniforms.sourceTexture];
            float deviceDepth = depthTexture.Sample(Sampler_Point_Clamp, IN.texCoord);
            float nearZ = LinearizeViewZ(0.0, camera.invProjectionMatrix);
            float farZ  = LinearizeViewZ(1.0, camera.invProjectionMatrix);
            float viewZ = LinearizeViewZ(deviceDepth, camera.invProjectionMatrix);
            float gray = saturate((viewZ - nearZ) / max(farZ - nearZ, EPSILON));
            color = sqrt(gray).xxx; // spread the perspective distribution for readability
            break;
        }
        case Gleam::ViewMode::MotionVectors:
        {
            Texture2D<float2> motionTexture = ResourceDescriptorHeap[uniforms.sourceTexture];
            float width, height;
            motionTexture.GetDimensions(width, height);
            float2 motion = motionTexture.Sample(Sampler_Point_Clamp, IN.texCoord) / float2(width, height);
            color = float3(saturate(0.5 + motion * 20.0), 0.5);
            break;
        }
        case Gleam::ViewMode::ShadowMask:
        {
            Texture2D<float> shadowTexture = ResourceDescriptorHeap[uniforms.sourceTexture];
            color = shadowTexture.Sample(Sampler_Point_Clamp, IN.texCoord).xxx;
            break;
        }
        default:
        {
            break;
        }
    }

    return float4(color, 1.0);
}
