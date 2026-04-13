#include "PathTraceCommon.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

void EvaluateMiss(inout Gleam::RayPayload payload, float3 origin, float3 direction)
{
    if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex &&
        atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
    {
        payload.radiance += payload.throughput * GetSunAndSkyIlluminance(origin, direction);
    }
    else
    {
        payload.radiance += payload.throughput * atmosphereUniforms.sunIlluminance;
    }
}

[shader("raygeneration")]
void pathTraceRayGen()
{
    uint2 pixelCoord = DispatchRaysIndex().xy;
    if (any(pixelCoord >= (uint2)camera.resolution))
    {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) / camera.resolution;
    float3 worldPos  = ScreenSpaceToWorldSpace(uv, 0.0, camera.invViewProjectionMatrix);
    float3 rayOrigin = camera.position;
    float3 rayDir    = normalize(worldPos - rayOrigin);

    RayDesc ray;
    ray.Origin    = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin      = 1e-3;
    ray.TMax      = 1e6;

    float3 accumRadiance = 0.0;
    if (pathTraceConstants.accelerationStructure == InvalidResourceIndex)
    {
        Gleam::RayPayload payload;
        payload.radiance   = 0.0;
        payload.throughput = 1.0;
        payload.depth      = 0;
        payload.seed       = 0;
        EvaluateMiss(payload, ray.Origin, ray.Direction);
        accumRadiance = payload.radiance;
    }
    else
    {
        uint   spp       = pathTraceConstants.samplesPerPixel;
        uint   baseIndex = pathTraceConstants.frameIndex * spp;
        for (uint s = 0; s < spp; s++)
        {
            Gleam::RayPayload payload;
            payload.radiance   = 0.0;
            payload.throughput = 1.0;
            payload.depth      = 0;
            payload.seed       = PathTraceInitSeed(pixelCoord, baseIndex + s);

            TraceRay(
                accelerationStructure,
                RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
                0xFF,
                (uint)Gleam::RayType::PrimaryRay,
                0,
                (uint)Gleam::RayType::PrimaryRay,
                ray,
                payload
            );

            accumRadiance += payload.radiance;
        }
        accumRadiance /= float(spp);
    }

    RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[pathTraceConstants.colorTarget];
    if (pathTraceConstants.frameIndex == 0)
    {
        colorTarget[pixelCoord] = float4(accumRadiance, 1.0);
    }
    else
    {
        float3 prev = colorTarget[pixelCoord].rgb;
        float weight = 1.0 / float(pathTraceConstants.frameIndex + 1);
        colorTarget[pixelCoord] = float4(lerp(prev, accumRadiance, weight), 1.0);
    }
}

[shader("miss")]
void pathTraceMiss(inout Gleam::RayPayload payload : SV_RayPayload)
{
    EvaluateMiss(payload, WorldRayOrigin(), WorldRayDirection());
}

[shader("miss")]
void pathTraceShadowMiss(inout Gleam::ShadowPayload payload : SV_RayPayload)
{
    payload.visibility = 1.0;
}
