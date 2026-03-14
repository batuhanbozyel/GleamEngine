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

uint pcgHash(uint v)
{
	uint state = v * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float randFloat(inout uint seed)
{
	seed = pcgHash(seed);
	return float(seed) / float(0xFFFFFFFFu);
}

float2 randFloat2(inout uint seed)
{
	return float2(randFloat(seed), randFloat(seed));
}

uint initSeed(uint2 pixel, uint frameIndex)
{
	return pcgHash(pixel.x ^ pcgHash(pixel.y ^ pcgHash(frameIndex)));
}

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

float specularLobeProbability(float3 albedo, float metallic, float perceptualRoughness, float NdotV)
{
	float3 f0 = albedo * metallic + F0Dielectric(0.5) * (1.0 - metallic);
    // Use NdotV as a stand-in for VdotH at the sampling stage (H unknown yet)
	float f90 = lerp(F90Dielectric(NdotV, perceptualRoughness), F90_Metal, metallic);
	float3 F = F_Schlick(f0, f90, NdotV);

	float specLuma = dot(F, float3(0.2126, 0.7152, 0.0722));
	float diffLuma = dot(albedo * (1.0 - metallic), float3(0.2126, 0.7152, 0.0722));
	return specLuma / max(specLuma + diffLuma, 1e-4);
}

static const int MAX_BOUNCES = 4;

float3 tracePath(float3 rayOrigin, float3 rayDir, DirectLight light, inout uint seed)
{
	float3 radiance = 0.0;
	float3 throughput = 1.0;

	for (int bounce = 0; bounce <= MAX_BOUNCES; bounce++)
	{
		Payload hit = rayMarch(rayOrigin, rayDir);
		
		if (hit.t <= 0.0)
		{
			if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex &&
                atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
			{
				radiance += throughput * GetSunAndSkyIlluminance(rayOrigin, rayDir);
			}
			break;
		}

		float3 worldPos = rayOrigin + rayDir * hit.t;
		float3 worldNormal = calcNormal(worldPos);
		float3 viewDir = -rayDir;

		Gleam::SurfaceOutput surface = buildSurface(hit, worldPos);
		radiance += throughput * EvaluateDirectLight(surface, light, viewDir, worldNormal);

		if (bounce == MAX_BOUNCES)
		{
			break;
		}
        
		float3 albedo = surface.albedo.rgb;
		float perceptualRough = surface.roughness;
		float metallic = surface.metallic;
		float NdotV = abs(dot(worldNormal, viewDir)) + FLT_EPSILON;

		float pSpec = specularLobeProbability(albedo, metallic, perceptualRough, NdotV);
		float2 xi = randFloat2(seed);
		float3 nextDir;
		float3 brdfWeight;

		if (randFloat(seed) < pSpec)
		{
			float unusedPdf;
			float3 H = ImportanceSampleGGX(xi, worldNormal, perceptualRough, unusedPdf);
			nextDir = reflect(-viewDir, H);

			if (dot(nextDir, worldNormal) <= 0.0)
			{
				break;
			}

			float NdotL = saturate(dot(worldNormal, nextDir));
			float NdotH = saturate(dot(worldNormal, H));
			float VdotH = saturate(dot(viewDir, H));
			float LdotH = VdotH; // symmetric: LdotH == VdotH for reflect()

			float roughness = perceptualRough * perceptualRough;
			float3 f0 = albedo * metallic + F0Dielectric(0.5) * (1.0 - metallic);
			float f90 = lerp(F90Dielectric(LdotH, perceptualRough), F90_Metal, metallic);
			float3 F = F_Schlick(f0, f90, LdotH);
			float V = V_SmithGGXCorrelated(NdotL, NdotV, roughness);
			float G = V * (4.0 * NdotL * NdotV); // recover G from V_Smith

            // Full weight:  F * D * V * NdotL / pdf
            // pdf = D * NdotH / (4 * VdotH)
            // => F * G * VdotH / (NdotH * pSpec)
			brdfWeight = F * G * VdotH / max(NdotH * pSpec, 1e-4);
		}
		else
		{
			float unusedPdf;
			nextDir = CosineSampleHemisphere(xi, worldNormal, unusedPdf);

			float NdotL = saturate(dot(worldNormal, nextDir));
			float3 H = normalize(viewDir + nextDir);
			float LdotH = saturate(dot(nextDir, H));

            // weight = Fr_DisneyDiffuse * Fd_Lambert() * NdotL / pdf
            // CosineSampleHemisphere pdf = NdotL * INV_PI, so NdotL and INV_PI both cancel
			float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, perceptualRough);
			brdfWeight = albedo * (1.0 - metallic) * Fd / max(1.0 - pSpec, 1e-4);
		}

		throughput *= brdfWeight;
        
		if (bounce >= 2)
		{
			float p = max(throughput.r, max(throughput.g, throughput.b));
			if (randFloat(seed) > p)
			{
				break;
			}
			throughput /= p;
		}

		rayOrigin = worldPos + worldNormal * 0.002;
		rayDir = nextDir;
	}

	return radiance;
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
    
	uint seed = initSeed(dispatchThreadID.xy, constants.frameIndex);
    float3 color = tracePath(rayOrigin, rayDir, light, seed);
    
	RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[constants.colorTarget];
    if (constants.frameIndex == 0)
	{
		colorTarget[dispatchThreadID.xy] = float4(color, 1);
	}
	else
	{
		float3 prev = colorTarget[dispatchThreadID.xy].rgb;
		float weight = 1.0 / float(constants.frameIndex + 1);
		colorTarget[dispatchThreadID.xy] = float4(lerp(prev, color, weight), 1.0);
	}
}
