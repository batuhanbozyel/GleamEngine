#include "BRDF.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

PUSH_CONSTANT(Gleam::PathTracerConstants, constants);

#pragma compute pathTraceShader

struct Sphere
{
    float3 center;
    float  radius;
    float3 albedo;
    float  roughness;
    float  metallic;
};

struct Payload
{
    float t;
    int hit;
};

static const int    NUM_SPHERES  = 4;
static const Sphere spheres[NUM_SPHERES] =
{
    // center                    radius  albedo                     roughness  metallic
    { float3( 0.0,  0.0,  -5.0), 1.0,    float3(0.9, 0.2, 0.2),     0.3,       0.0 },  // red plastic
    { float3( 2.5,  0.0,  -6.0), 1.0,    float3(0.9, 0.7, 0.2),     0.1,       1.0 },  // gold metal
    { float3(-2.5,  0.5,  -7.0), 1.5,    float3(0.2, 0.9, 0.3),     0.7,       0.0 },  // green rough
    { float3( 0.0, -101.0,-5.0), 100.0,  float3(0.8, 0.8, 0.8),     0.8,       0.0 },  // ground
};

float sdSphere(float3 p, float3 center, float radius)
{
    return length(p - center) - radius;
}

Payload sceneSD(float3 p)
{
    Payload payload;
    payload.t = 1e9;
    payload.hit = -1;
    
    for (int i = 0; i < NUM_SPHERES; i++)
    {
        float d = sdSphere(p, spheres[i].center, spheres[i].radius);
        if (d < payload.t)
        {
            payload.t = d;
            payload.hit = i;
        }
    }
    return payload;
}

static const int   MAX_STEPS  = 128;
static const float MAX_DIST   = 100.0;
static const float SURF_DIST  = 0.001;

Payload rayMarch(float3 ro, float3 rd)
{
    float t = 0.0;
    for (int i = 0; i < MAX_STEPS; i++)
    {
        float3 p  = ro + rd * t;
        Payload payload = sceneSD(p);
        if (payload.t < SURF_DIST)
        {
            payload.t = t;
            return payload;
        }
        t += payload.t;
        if (t >= MAX_DIST)
        {
            break;
        }
    }
    Payload miss;
    miss.t = -1.0;
    miss.hit = -1;
    return miss;
}

float3 calcNormal(float3 p)
{
    const float eps = 0.001;
    float2 e = float2(eps, 0.0);
    return normalize(float3(
        sceneSD(p + e.xyy).t - sceneSD(p - e.xyy).t,
        sceneSD(p + e.yxy).t - sceneSD(p - e.yxy).t,
        sceneSD(p + e.yyx).t - sceneSD(p - e.yyx).t
    ));
}

float softShadow(float3 ro, float3 rd, float mint, float maxt, float k)
{
    float res = 1.0;
    float t   = mint;
    for (int i = 0; i < 64 && t < maxt; i++)
    {
        float h = sceneSD(ro + rd * t).t;
        if (h < SURF_DIST)
        {
            return 0.0;
        }
        res  = min(res, k * h / t);
        t   += h;
    }
    return res;
}

static const float3 SKY_COLOR    = float3(0.3, 0.55, 0.85);
static const float3 HORIZON_COLOR= float3(0.7, 0.75, 0.8);

float3 skyGradient(float3 rd)
{
    float t = saturate(rd.y * 0.5 + 0.5);
    return lerp(HORIZON_COLOR, SKY_COLOR, t);
}

Gleam::SurfaceOutput buildSurface(Payload payload, float3 worldPos)
{
    Gleam::SurfaceOutput surface;
    surface.albedo    = float4(spheres[payload.hit].albedo, 1.0);
    surface.emission  = float4(0.0, 0.0, 0.0, 0.0);
    surface.normal    = calcNormal(worldPos); // world-space normal, no TBN needed
    surface.roughness = spheres[payload.hit].roughness;
    surface.metallic  = spheres[payload.hit].metallic;
    return surface;
}

float3 shade(float3 ro, float3 rd, Payload payload, DirectLight light)
{
    float3 worldPos = ro + rd * payload.t;
    float3 worldNormal = calcNormal(worldPos);
    float3 viewDir = -rd;

    Gleam::SurfaceOutput surface = buildSurface(payload, worldPos);

    float shadow = softShadow(worldPos + worldNormal * 0.002, light.direction, 0.01, 20.0, 16.0);
    DirectLight shadowedLight = light;
    shadowedLight.illuminance *= shadow;

    float3 color = 0.0;
    color += EvaluateDirectLight(surface, shadowedLight, viewDir, worldNormal);
    return color;
}

[numthreads(16, 16, 1)]
void pathTraceShader(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (any(dispatchThreadID.xy >= camera.resolution))
    {
        return;
    }
    
    float2 uv = (float2(dispatchThreadID.xy) + 0.5) / camera.resolution;
    float3 worldPos = ScreenSpaceToWorldSpace(uv, 0.0, camera.invViewProjectionMatrix);

    float3 rayOrigin = camera.position;
    float3 rayDir = normalize(worldPos - camera.position);
    
    DirectLight light;
    if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex && atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
    {
        light.direction = atmosphereUniforms.sunDirection;
        light.illuminance = GetSunLuminance(GetSkyWorldPosition(worldPos), atmosphereUniforms.sunDirection);
    }
    else
    {
        light.direction = atmosphereUniforms.sunDirection;
        light.illuminance = atmosphereUniforms.sunIlluminance;
    }
    
    Payload hit = rayMarch(rayOrigin, rayDir);

    float3 color;
    if (hit.t > 0.0)
        color = shade(rayOrigin, rayDir, hit, light);
    else
        color = skyGradient(rayDir);
    
    RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[constants.colorTarget];
    colorTarget[dispatchThreadID.xy] = float4(color, 1);
}
