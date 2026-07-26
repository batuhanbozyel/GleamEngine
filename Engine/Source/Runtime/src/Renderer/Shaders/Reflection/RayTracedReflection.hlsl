#include "PathTraceCommon.hlsli"

PUSH_CONSTANT(Gleam::RayTracedReflectionConstants, constants);

float4 EvaluateProbeFallback(float3 viewDir, float3 worldNormal, float roughness)
{
    TextureCube<float4> specularReflection = ResourceDescriptorHeap[constants.specularReflectionTexture];
    float3 reflectionDir = normalize(reflect(-viewDir, worldNormal));
    float3 specularDirection = GetSpecularDominantDir(worldNormal, reflectionDir, roughness);
    float mipLevel = PerceptualRoughnessToMipLevel(roughness, SPECULAR_RADIANCE_MAX_MIP_COUNT - 1);
    return float4(specularReflection.SampleLevel(Sampler_Trilinear_Repeat, specularDirection, mipLevel).rgb, 1.0);
}

[shader("raygeneration")]
void rayTracedReflectionRayGen()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    if (any(pixelCoord >= (uint2)camera.resolution))
    {
        return;
    }
    RWTexture2D<float4> reflectionTarget = ResourceDescriptorHeap[pathTraceConstants.colorTarget];

    Texture2D<float> depthTexture = ResourceDescriptorHeap[constants.depthTexture];
    float depth = depthTexture[pixelCoord];
    if (depth >= (1.0f - FLT_EPSILON))
    {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) / camera.resolution;
    float3 worldPos = ScreenSpaceToWorldSpace(uv, depth, camera.invViewProjectionMatrix);
    float3 viewDir = normalize(camera.position - worldPos);

    Texture2D<float2> shadingNormalTexture = ResourceDescriptorHeap[constants.shadingNormalTexture];
    Texture2D<float2> geometryNormalTexture = ResourceDescriptorHeap[constants.geometryNormalTexture];
    float3 shadingNormal = OctDecode(shadingNormalTexture[pixelCoord]);
    float3 geometryNormal = OctDecode(geometryNormalTexture[pixelCoord]);

    Texture2D<float> roughnessTexture = ResourceDescriptorHeap[constants.roughnessTexture];
    float roughness = roughnessTexture[pixelCoord];
    if (roughness > constants.roughnessCutoff)
    {
        reflectionTarget[pixelCoord] = EvaluateProbeFallback(viewDir, shadingNormal, roughness);
        return;
    }

    Gleam::RayPayload payload;
    payload.seed = PCGInitSeed(pixelCoord, pathTraceConstants.frameIndex);
    payload.radiance = 0.0f;
    payload.throughput = 1.0f;
    payload.depth = 0;

    float pdf;
    float3 H = ImportanceSampleGGX_VNDF(PCGRand2(payload.seed), viewDir, shadingNormal, roughness, pdf);
    float3 rayDir = reflect(-viewDir, H);
    if (dot(rayDir, geometryNormal) <= 0.0f)
    {
        reflectionTarget[pixelCoord] = EvaluateProbeFallback(viewDir, shadingNormal, roughness);
        return;
    }

    RayDesc ray;
    ray.Origin    = OffsetRayAlongNormal(worldPos, geometryNormal);
    ray.Direction = rayDir;
    ray.TMin      = camera.nearPlane;
    ray.TMax      = camera.farPlane;

    RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[pathTraceConstants.accelerationStructure];
    TraceRay(accelerationStructure,
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xFF,
            (uint)Gleam::RayType::PrimaryRay,
            0,
            (uint)Gleam::RayType::PrimaryRay,
            ray,
            payload);
    reflectionTarget[pixelCoord] = float4(payload.radiance, 1.0);
}
