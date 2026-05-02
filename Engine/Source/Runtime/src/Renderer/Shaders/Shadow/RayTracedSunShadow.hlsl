#define USE_PCG
#include "PathTraceCommon.hlsli"

PUSH_CONSTANT(Gleam::RayTracedSunShadowConstants, constants);

[shader("raygeneration")]
void rayTracedSunShadowRayGen()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    if (any(pixelCoord >= (uint2)camera.resolution))
    {
        return;
    }

    RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[pathTraceConstants.accelerationStructure];
    Texture2D<float> depthTex = ResourceDescriptorHeap[constants.depthTexture];
    RWTexture2D<uint> shadowMask = ResourceDescriptorHeap[pathTraceConstants.colorTarget];

    uint2 tileCoord = pixelCoord / uint2(SHADOW_TILE_WIDTH, SHADOW_TILE_HEIGHT);
    uint  bitIndex  = (pixelCoord.y % SHADOW_TILE_HEIGHT) * SHADOW_TILE_WIDTH + (pixelCoord.x % SHADOW_TILE_WIDTH);
    uint  bitMask   = 1u << bitIndex;

    float depth = depthTex[pixelCoord];
    if (depth >= (1.0f - FLT_EPSILON))
    {
        InterlockedOr(shadowMask[tileCoord], bitMask);
        return;
    }

    float2 uv       = (float2(pixelCoord) + 0.5f) / camera.resolution;
    float3 worldPos = ScreenSpaceToWorldSpace(uv, depth, camera.invViewProjectionMatrix);
    uint4  seed     = PCGInitSeed(pixelCoord, pathTraceConstants.frameIndex);

    uint2 pixelDx = min(pixelCoord + uint2(1, 0), (uint2)camera.resolution - 1u);
    uint2 pixelDy = min(pixelCoord + uint2(0, 1), (uint2)camera.resolution - 1u);
    float3 worldPosDx = ScreenSpaceToWorldSpace((float2(pixelDx) + 0.5f) / camera.resolution, depthTex[pixelDx], camera.invViewProjectionMatrix);
    float3 worldPosDy = ScreenSpaceToWorldSpace((float2(pixelDy) + 0.5f) / camera.resolution, depthTex[pixelDy], camera.invViewProjectionMatrix);
    float3 geometricNormal = normalize(cross(worldPosDx - worldPos, worldPosDy - worldPos));

    float shadowConePdf;
    const float sunHalfAngle    = 0.5 * atmosphereUniforms.sunAngularDiameter * (PI / 180.0);
    const float cosSunHalfAngle = cos(sunHalfAngle);
    float3 shadowDir = UniformSampleCone(PCGRand2(seed), atmosphereUniforms.sunDirection, cosSunHalfAngle, shadowConePdf);

    RayDesc ray;
    ray.Origin    = worldPos + geometricNormal * 1e-2f;
    ray.Direction = shadowDir;
    ray.TMin      = 0.1;
    ray.TMax      = 1000.0;

    Gleam::ShadowPayload payload;
    payload.visibility = 0.0f;

    TraceRay(accelerationStructure,
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             0xFF,
             (uint)Gleam::RayType::PrimaryRay,
             0,
             (uint)Gleam::RayType::PrimaryRay,
             ray,
             payload);

    if (payload.visibility > 0.5f)
    {
        InterlockedOr(shadowMask[tileCoord], bitMask);
    }
    else
    {
        InterlockedAnd(shadowMask[tileCoord], ~bitMask);
    }
}

[shader("miss")]
void rayTracedSunShadowMiss(inout Gleam::ShadowPayload payload : SV_RayPayload)
{
    payload.visibility = 1.0f;
}
