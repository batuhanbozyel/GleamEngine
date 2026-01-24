#include "BRDF.hlsli"

#pragma compute diffuseIrradianceConvolutionShader
#pragma compute specularPrefilterConvolutionShader

PUSH_CONSTANT(Gleam::ProbeConvolutionConstants, constants);

#define IRRADIANCE_SAMPLE_COUNT 256u
#define RADIANCE_SAMPLE_COUNT 128u

static TextureCube<float4> srcTexture = ResourceDescriptorHeap[constants.sourceTexture];
static RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[constants.targetTexture];

[numthreads(16, 16, 1)]
void diffuseIrradianceConvolutionShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (any(dispatchThreadId.xy >= constants.resolution))
	{
		return;
	}
	
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / constants.resolution;

	float3 V = GetCubemapDirection(uv, constants.face);
	float3 N = V;
	
	// The mipmap level is clamped to something lower than 8x8
	// in order to avoid cubemap filtering issues
	float maxMipLevel = floor(log2(float(constants.probeResolution))) - 3.0;
	float omegaP = 4.0 * PI / (6.0 * float(constants.resolution * constants.resolution));
	
	float3 irradiance = 0.0;
	for (uint i = 0u; i < IRRADIANCE_SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, IRRADIANCE_SAMPLE_COUNT);

		float pdf;
		float3 L = CosineSampleHemisphere(Xi, N, pdf);
		
		float omegaS = 1.0 / (IRRADIANCE_SAMPLE_COUNT * pdf);
		float mipLevel = clamp(0.5 * log2(omegaS / omegaP) + 5.0f, 0.0f, maxMipLevel);
		irradiance += srcTexture.SampleLevel(Sampler_Trilinear_Repeat, L, mipLevel).rgb;
	}
	dstTexture[dispatchThreadId.xy] = float4(irradiance / IRRADIANCE_SAMPLE_COUNT, 1.0);
}

[numthreads(16, 16, 1)]
void specularPrefilterConvolutionShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (any(dispatchThreadId.xy >= constants.resolution))
	{
		return;
	}
	
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / constants.resolution;

	float3 V = GetCubemapDirection(uv, constants.face);
	float3 N = V;
	
	// The mipmap level is clamped to something lower than 8x8
	// in order to avoid cubemap filtering issues
	float maxMipLevel = floor(log2(float(constants.probeResolution))) - 3.0;
	float omegaP = 4.0 * PI / (6.0 * float(constants.probeResolution * constants.probeResolution));
	float perceptualRoughness = MipLevelToPerceptualRoughness(constants.level, SPECULAR_RADIANCE_MAX_MIP_COUNT - 1);
	
	float weight = 0.0;
	float3 radiance = 0.0;
	for (uint i = 0u; i < RADIANCE_SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, RADIANCE_SAMPLE_COUNT);

		float pdf;
		float3 H = ImportanceSampleGGX(Xi, N, perceptualRoughness, pdf);
		float3 L = reflect(-V, H);

		float NdotL = dot(N, L);
		if (NdotL > 0.0)
		{
			float LdotH = saturate(dot(L, H));
			float VdotH = saturate(dot(V, H));
			
			// Since we pre-integrate the result for normal direction,
			// N == V and then NdotH == LdotH. This is why the BRDF pdf
			// can be simplifed from:
			// pdf = D_GGX(NdotH, roughness) * NdotH / (4 * VdotH);
			// to
			// pdf = D_GGX(NdotH, roughness) / 4;
			pdf /= 4.0;
			
			float omegaS = 1.0 / (RADIANCE_SAMPLE_COUNT * pdf);
			float mipLevel = clamp(0.5 * log2(omegaS / omegaP), 0.0f, maxMipLevel);
			radiance += srcTexture.SampleLevel(Sampler_Trilinear_Repeat, L, mipLevel).rgb * NdotL;
			weight += NdotL;
		}
	}
	dstTexture[dispatchThreadId.xy] = float4(radiance / weight, 1.0);
}