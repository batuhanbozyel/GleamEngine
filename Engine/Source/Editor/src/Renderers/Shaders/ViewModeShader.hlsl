#include "Common.hlsli"
#include "VisibilityBufferCommon.hlsli"
#include "../ViewMode.h"
#include "../ShaderTypes.h"

PUSH_CONSTANT(GEditor::ViewModeUniforms, uniforms);

// Reconstruct view-space Z from device depth using the inverse projection.
float LinearizeViewZ(float deviceDepth, float4x4 invProjectionMatrix)
{
    float4 viewPos = mul(invProjectionMatrix, float4(0.0, 0.0, deviceDepth, 1.0));
    return viewPos.z / viewPos.w;
}

float3 HashIDToColor(uint id)
{
    uint h = id * 2654435761u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return float3((h & 0xFFu), ((h >> 8) & 0xFFu), ((h >> 16) & 0xFFu)) / 255.0f;
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

            const float cellSize    = 32.0;
            const float shaftWidth  = 0.04;
            const float headLenFrac = 0.35;
            const float headWidth   = 0.12;
            const float speedScale  = 20.0;

            float2 pixelPos = IN.texCoord * camera.resolution;

            // Grid cell containing this pixel
            float2 cellIdx    = floor(pixelPos / cellSize);
            float2 cellCenter = (cellIdx + 0.5) * cellSize;

            // Sample motion at cell center (UV-space offset)
            float2 cellMotion = motionTexture.Sample(Sampler_Point_Clamp, cellCenter / camera.resolution) / camera.resolution;
            float2 velocity   = cellMotion * speedScale;
            float  speed      = length(velocity);
            float  arrowLen   = min(speed, 0.45);
            float2 arrowDir   = speed > EPSILON ? velocity / speed : float2(1.0, 0.0);

            // Arrow-aligned coordinates in cell-local space [-0.5, 0.5]
            float2 localPos = (pixelPos - cellCenter) / cellSize;
            float  uAlong   = dot(localPos, arrowDir);
            float  vPerp    = dot(localPos, float2(-arrowDir.y, arrowDir.x));
            float  u        = uAlong + arrowLen * 0.5;   // [0, arrowLen]

            float headLen  = arrowLen * headLenFrac;
            float shaftEnd = arrowLen - headLen;

            // Shaft SDF
            float shaftSdf = max(abs(vPerp) - shaftWidth, max(-u, u - shaftEnd));
            // Arrowhead SDF (tapered triangle)
            float headProgress = clamp((u - shaftEnd) / (headLen + EPSILON), 0.0, 1.0);
            float headSdf = max(abs(vPerp) - headWidth * (1.0 - headProgress),
                                max(-(u - shaftEnd), u - arrowLen));

            float arrowSdf  = (speed > EPSILON) ? min(shaftSdf, headSdf) : 1.0;
            float fw        = fwidth(arrowSdf);
            float arrowMask = 1.0 - smoothstep(-fw, fw, arrowSdf);

            // Angle -> hue (compact HSV with S=1, V=1)
            float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);

            float  arrowHue   = atan2(arrowDir.y, arrowDir.x) / TWO_PI + 0.5;
            float3 arrowHsvP  = abs(frac(arrowHue + K.xyz) * 6.0 - K.www);
            float3 arrowColor = saturate(arrowHsvP - K.xxx);

            // Continuous background: dim direction field sampled per-pixel
            float2 bgMotion = motionTexture.Sample(Sampler_Bilinear_Clamp, IN.texCoord) / camera.resolution;
            float  bgHue    = atan2(bgMotion.y, bgMotion.x) / TWO_PI + 0.5;
            float3 bgHsvP   = abs(frac(bgHue + K.xyz) * 6.0 - K.www);
            float3 bgColor  = saturate(bgHsvP - K.xxx) * saturate(length(bgMotion * speedScale)) * 0.25;

            color = lerp(bgColor, arrowColor, arrowMask);
            break;
        }
        case Gleam::ViewMode::ShadowMask:
        {
            Texture2D<float> shadowTexture = ResourceDescriptorHeap[uniforms.sourceTexture];
            color = shadowTexture.Sample(Sampler_Point_Clamp, IN.texCoord).xxx;
            break;
        }
        case Gleam::ViewMode::MeshletVisualization:
        {
            Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[uniforms.sourceTexture];
            PackedVisibilityID packedID = visibilityBuffer.Load(int3(IN.texCoord * camera.resolution, 0));
            if (IsValidVisibilityID(packedID))
            {
                Gleam::VisibilityID visibility = UnpackVisibilityID(packedID);
                color = HashIDToColor((visibility.instanceID << 16) | (visibility.meshletID & 0xFFFFu));
            }
            break;
        }
        case Gleam::ViewMode::VisibilityIDs:
        {
            Texture2D<PackedVisibilityID> visibilityBuffer = ResourceDescriptorHeap[uniforms.sourceTexture];
            PackedVisibilityID packedID = visibilityBuffer.Load(int3(IN.texCoord * camera.resolution, 0));
            if (IsValidVisibilityID(packedID))
            {
                color = HashIDToColor(packedID.x * 2654435761u + packedID.y);
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return float4(color, 1.0);
}
