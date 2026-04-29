#include "BRDF.hlsli"
#include "ShaderTypes.h"
#include "Random.hlsli"

CONSTANT_BUFFER(Gleam::CameraUniforms, camera, CAMERA_UNIFORMS_BINDING_SLOT);
CONSTANT_BUFFER(Gleam::SkyAtmosphereUniforms, atmosphereUniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

PUSH_CONSTANT(Gleam::RayTracedSunShadowConstants, constants);

[shader("raygeneration")]
void rayTracedSunShadowRayGen()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    if (any(pixelCoord >= (uint2)camera.resolution))
    {
        return;
    }

    RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[constants.accelerationStructure];
    Texture2D<float> depthTex = ResourceDescriptorHeap[constants.depthTexture];
    float depth = depthTex[pixelCoord];

    RWTexture2D<float> shadowMask = ResourceDescriptorHeap[constants.shadowMask];
    if (depth >= (1.0f - FLT_EPSILON))
    {
        shadowMask[pixelCoord] = 1.0f;
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5f) / camera.resolution;
    float3 worldPos = ScreenSpaceToWorldSpace(uv, depth, camera.invViewProjectionMatrix);
    uint4 seed = PCGInitSeed(pixelCoord, constants.frameIndex);

    float shadowConePdf;
    const float sunHalfAngle = 0.5 * atmosphereUniforms.sunAngularDiameter * (PI / 180.0);
    const float cosSunHalfAngle = cos(sunHalfAngle);
    float3 shadowDir = UniformSampleCone(PCGRand2(seed), atmosphereUniforms.sunDirection, cosSunHalfAngle, shadowConePdf);
    
    RayDesc ray;
    ray.Origin    = worldPos;
    ray.Direction = shadowDir;
    ray.TMin      = 1e-3f;
    ray.TMax      = 1e6f;

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

    shadowMask[pixelCoord] = payload.visibility;
}

[shader("miss")]
void rayTracedSunShadowMiss(inout Gleam::ShadowPayload payload : SV_RayPayload)
{
    payload.visibility = 1.0f;
}
