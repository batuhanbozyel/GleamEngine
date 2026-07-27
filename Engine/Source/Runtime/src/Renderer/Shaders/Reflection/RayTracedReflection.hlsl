#include "PathTraceCommon.hlsli"
#include "ReflectionRayList.hlsli"

PUSH_CONSTANT(Gleam::RayTracedReflectionConstants, constants);

void StoreReflection(uint2 pixelCoord, float4 radiance)
{
    RWTexture2D<float4> reflectionTarget = ResourceDescriptorHeap[pathTraceConstants.colorTarget];
    reflectionTarget[pixelCoord] = radiance;
}

void StoreReflectionQuad(uint2 pixelCoord, bool copyHorizontal, bool copyVertical, bool copyDiagonal, float4 radiance)
{
    StoreReflection(pixelCoord, radiance);

    const uint2 copyTarget = pixelCoord ^ 1;
    if (copyHorizontal)
    {
        StoreReflection(uint2(copyTarget.x, pixelCoord.y), radiance);
    }
    if (copyVertical)
    {
        StoreReflection(uint2(pixelCoord.x, copyTarget.y), radiance);
    }
    if (copyDiagonal)
    {
        StoreReflection(copyTarget, radiance);
    }
}

float4 EvaluateProbeFallback(float3 viewDir, float3 worldNormal, float roughness)
{
    TextureCube<float4> specularReflection = ResourceDescriptorHeap[constants.specularReflectionTexture];
    float3 reflectionDir = normalize(reflect(-viewDir, worldNormal));
    float3 specularDirection = GetSpecularDominantDir(worldNormal, reflectionDir, roughness);
    float mipLevel = PerceptualRoughnessToMipLevel(roughness, SPECULAR_RADIANCE_MAX_MIP_COUNT - 1);
    return float4(specularReflection.SampleLevel(Sampler_Trilinear_Repeat, specularDirection, mipLevel).rgb, min(camera.farPlane, 65504.0));
}

[shader("raygeneration")]
void rayTracedReflectionRayGen()
{
    ByteAddressBuffer rayList = ResourceDescriptorHeap[constants.rayListBuffer];
    const uint packedCoords = rayList.Load<uint>(DispatchRaysIndex().x * sizeof(uint));

    uint2 pixelCoord;
    bool copyHorizontal;
    bool copyVertical;
    bool copyDiagonal;
    UnpackReflectionRayCoords(packedCoords, pixelCoord, copyHorizontal, copyVertical, copyDiagonal);

    Texture2D<float> depthTexture = ResourceDescriptorHeap[constants.depthTexture];
    float depth = depthTexture[pixelCoord];

    float2 uv = (float2(pixelCoord) + 0.5) / camera.resolution;
    float3 worldPos = ScreenSpaceToWorldSpace(uv, depth, camera.invViewProjectionMatrix);
    float3 viewDir = normalize(camera.position - worldPos);

    Texture2D<float2> shadingNormalTexture = ResourceDescriptorHeap[constants.shadingNormalTexture];
    Texture2D<float2> geometryNormalTexture = ResourceDescriptorHeap[constants.geometryNormalTexture];
    float3 shadingNormal = OctDecode(shadingNormalTexture[pixelCoord]);
    float3 geometryNormal = OctDecode(geometryNormalTexture[pixelCoord]);

    Texture2D<float> roughnessTexture = ResourceDescriptorHeap[constants.roughnessTexture];
    float roughness = roughnessTexture[pixelCoord];

    Gleam::RayPayload payload;
    payload.seed = PCGInitSeed(pixelCoord, pathTraceConstants.frameIndex);
    payload.radiance = 0.0f;
    payload.throughput = 1.0f;
    payload.depth = 0;
    payload.hitDistance = min(camera.farPlane, 65504.0);

    float pdf;
    float3 H = ImportanceSampleGGX_VNDF(PCGRand2(payload.seed), viewDir, shadingNormal, roughness, pdf);
    float3 rayDir = reflect(-viewDir, H);
    if (dot(rayDir, geometryNormal) <= 0.0f)
    {
        StoreReflectionQuad(pixelCoord, copyHorizontal, copyVertical, copyDiagonal,
                            EvaluateProbeFallback(viewDir, shadingNormal, roughness));
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

    StoreReflectionQuad(pixelCoord, copyHorizontal, copyVertical, copyDiagonal,
                        float4(payload.radiance, payload.hitDistance));
}
