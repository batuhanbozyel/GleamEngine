#include "PathTraceCommon.hlsli"
#include "Atmosphere/SkyAtmosphereCommon.hlsli"

struct Payload
{
    float t;
    int hit;
};

#define RAY_MARCHING_
#ifdef RAY_MARCHING_

static const int MAX_STEPS = 128;
static const float SURF_DIST = 0.001;
static const float MAX_DIST = 1000.0;
static const int NUM_SPHERES = 4;

struct Sphere
{
	float3 center;
	float radius;
	float3 albedo;
	float roughness;
	float metallic;
};

static const Sphere spheres[NUM_SPHERES] =
{
    // center                    radius  albedo                     roughness  metallic
	{ float3(0.0, 0.0, -5.0), 1.0, float3(0.9, 0.2, 0.2), 0.3, 0.0 }, // red plastic
	{ float3(2.5, 0.0, -6.0), 1.0, float3(0.9, 0.7, 0.2), 0.1, 1.0 }, // gold metal
	{ float3(-2.5, 0.5, -7.0), 1.5, float3(0.2, 0.9, 0.3), 0.7, 0.0 }, // green rough
	{ float3(0.0, -101.0, -5.0), 100.0, float3(0.8, 0.8, 0.8), 0.8, 0.0 }, // ground
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

Payload RayMarch(Ray ray)
{
	float t = 0.0;
	for (int i = 0; i < MAX_STEPS; i++)
	{
		float3 p = ray.origin + ray.direction * t;
		Payload payload = sceneSD(p);
		if (payload.t < ray.tMin)
		{
			payload.t = t;
			return payload;
		}
		t += payload.t;
		if (t >= ray.tMax)
		{
			break;
		}
	}
	Payload miss;
	miss.t = -1.0;
	miss.hit = -1;
	return miss;
}

float3 ComputeNormal(float3 p)
{
	const float eps = 0.0001;
	float2 e = float2(eps, 0.0);
	return normalize(float3(
        sceneSD(p + e.xyy).t - sceneSD(p - e.xyy).t,
        sceneSD(p + e.yxy).t - sceneSD(p - e.yxy).t,
        sceneSD(p + e.yyx).t - sceneSD(p - e.yyx).t
    ));
}

float3 OffsetRayAlongNormal(const float3 p, const float3 n)
{
	return p + n * SURF_DIST;
}

Gleam::SurfaceOutput surf(Payload payload, float3 worldPos)
{
	Gleam::SurfaceOutput surface;
	surface.albedo = float4(spheres[payload.hit].albedo, 1.0);
	surface.emission = float4(0.0, 0.0, 0.0, 0.0);
	surface.normal = ComputeNormal(worldPos);
	surface.roughness = spheres[payload.hit].roughness;
	surface.metallic = spheres[payload.hit].metallic;
	return surface;
}
#endif

static const int MAX_BOUNCES = 10;

float3 TracePath(Ray ray, DirectLight light, inout uint seed)
{
	float3 radiance = 0.0;
	float3 throughput = 1.0;

	for (; ray.depth < MAX_BOUNCES; ray.depth++)
	{
		Payload hit = RayMarch(ray);
		
		if (hit.t <= 0.0)
		{
			if (atmosphereUniforms.transmittanceLutTexture != InvalidResourceIndex &&
                atmosphereUniforms.multiScatterLutTexture != InvalidResourceIndex)
			{
				radiance += throughput * GetSunAndSkyIlluminance(ray.origin, ray.direction);
			}
			break;
		}

		float3 worldPos = ray.origin + ray.direction * hit.t;
		float3 viewDir = -ray.direction;
		
		Gleam::SurfaceOutput surface = surf(hit, worldPos);
		if (dot(viewDir, surface.normal) < 0)
		{
			break;
		}
		
		radiance += throughput * EvaluateDirectLight(surface, light, viewDir, surface.normal);
		float NdotV = abs(dot(surface.normal, viewDir)) + FLT_EPSILON;
		
		BRDFType brdfType;
		if (surface.metallic == 1.0 && surface.roughness < PERFECT_MIRROR_ROUGHNESS)
		{
			brdfType = SPECULAR_BRDF;
		}
		else
		{
			float pSpec = SpecularLobeProbability(surface, NdotV);
			if (randFloat(seed) < pSpec)
			{
				brdfType = SPECULAR_BRDF;
				throughput /= pSpec;
			}
			else
			{
				brdfType = DIFFUSE_BRDF;
				throughput /= (1.0 - pSpec);
			}
		}
		
		float3 nextDir;
		float2 xi = randFloat2(seed);
		if (brdfType == SPECULAR_BRDF)
		{
			float partialPdf;
			float3 H = ImportanceSampleGGX(xi, surface.normal, surface.roughness, partialPdf);
			nextDir = reflect(-viewDir, H);

			if (dot(nextDir, surface.normal) <= 0.0)
			{
				break;
			}

			float NdotL = saturate(dot(surface.normal, nextDir));
			float NdotH = saturate(dot(surface.normal, H));
			float VdotH = saturate(dot(viewDir, H));
			float LdotH = VdotH; // symmetric: LdotH == VdotH for reflect()

			float roughness = surface.roughness * surface.roughness;
			float3 f0 = surface.albedo.rgb * surface.metallic + F0Dielectric(0.5) * (1.0 - surface.metallic);
			float f90 = lerp(F90Dielectric(LdotH, surface.roughness), F90_Metal, surface.metallic);
			float3 F = F_Schlick(f0, f90, LdotH);
			float G = G_SmithGGXCorrelated(NdotL, NdotV, roughness);
			
			// Full BRDF: F * D * G / (4 * NdotL * NdotV)
			// Monte Carlo weight: brdf * NdotL / pdf
			// pdf: D * NdotH / (4 * VdotH)
#if EXPLICIT_SPECULAR_BRDF_FORMULA
			float pdf = partialPdf * NdotH / (4.0 * VdotH);
			float3 brdf = F * partialPdf * G / max(4.0 * NdotL * NdotV, 1e-4);
			throughput *= brdf * NdotL / max(pdf, 1e-4);
#else
			throughput *= F * G * VdotH / max(NdotV * NdotH, 1e-4);
#endif
		}
		else
		{
			float pdf; // The pdf is not used because it's canceled with other terms (The 1/PI from diffuse BRDF and the NdotL from Lambert's law).
			nextDir = CosineSampleHemisphere(xi, surface.normal, pdf);

			float NdotL = saturate(dot(surface.normal, nextDir));
			if (NdotL <= 0.0)
			{
				break;
			}
			
			float3 H = normalize(viewDir + nextDir);
			float LdotH = saturate(dot(nextDir, H));

			float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, surface.roughness);
			
			// weight = Fr_DisneyDiffuse * Fd_Lambert() * NdotL / pdf
#if EXPLICIT_DIFFUSE_BRDF_FORMULA
			float3 brdf = surface.albedo.rgb * (1.0 - surface.metallic) * Fd * Fd_Lambert();
			throughput *= brdf * NdotL / max(pdf, 1e-4);
#else
            // CosineSampleHemisphere pdf = NdotL * INV_PI, so NdotL and INV_PI both cancel
			throughput *= surface.albedo.rgb * (1.0 - surface.metallic) * Fd;
#endif
		}
        
		if (ray.depth >= 5)
		{
			float p = max(throughput.r, max(throughput.g, throughput.b));
			if (randFloat(seed) > p)
			{
				break;
			}
			throughput /= p;
		}

		ray.origin = OffsetRayAlongNormal(worldPos, surface.normal);
		ray.direction = nextDir;
	}

	return radiance;
}

[shader("compute")]
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
    
	Ray ray = { rayOrigin, rayDir, SURF_DIST, MAX_DIST, 0 };
	uint seed = initSeed(dispatchThreadID.xy, constants.frameIndex);
	float3 color = TracePath(ray, light, seed);
    
	RWTexture2D<float4> colorTarget = ResourceDescriptorHeap[constants.colorTarget];
    if (constants.frameIndex == 0)
	{
		colorTarget[dispatchThreadID.xy] = float4(color, 1.0);
	}
	else
	{
		float3 prev = colorTarget[dispatchThreadID.xy].rgb;
		float weight = 1.0 / float(constants.frameIndex + 1);
		colorTarget[dispatchThreadID.xy] = float4(lerp(prev, color, weight), 1.0);
	}
}
