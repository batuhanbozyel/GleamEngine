#include "BRDF.hlsli"

PUSH_CONSTANT(Gleam::BRDFLutConstants, constants);

#define SAMPLE_COUNT (8192u * 4u)
float3 IntegrateDFG(in float NdotV, in float perceptualRoughness)
{
	float3 N = float3(0.0, 1.0, 0.0);
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), NdotV, 0.0);
    float roughness = perceptualRoughness * perceptualRoughness;

    float3 preDFG = float3(0.0, 0.0, 0.0);
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);

		float pdf; // The pdf is not used because it's canceled with other terms
        float3 H = ImportanceSampleGGX(Xi, N, perceptualRoughness, pdf);
		float3 L = reflect(-V, H);

        float NdotL = L.y;
        if (NdotL > 0.0)
        {
            float NdotH = H.y;
            float VdotH = saturate(dot(V, H));
            
            float G = G_SmithGGXCorrelated(NdotL, NdotV, roughness);
			float G_Vis = G * VdotH / (NdotH * NdotV);
            
            float Fc = pow(1.0 - VdotH, 5.0);
            float f90 = F90Dielectric(VdotH, perceptualRoughness);
            
			preDFG.x += (1.0 - Fc) * G_Vis;
			preDFG.y += f90 * Fc * G_Vis;
			preDFG.z += F90_Metal * Fc * G_Vis;
		}
    }
	return preDFG / SAMPLE_COUNT;
}

float IntegrateDiffuse(in float NdotV, in float perceptualRoughness)
{
    float3 N = float3(0.0, 1.0, 0.0);
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), NdotV, 0.0);

    float irradiance = 0.0;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        
        float pdf; // The pdf is not used because it's canceled with other terms (The 1/PI from diffuse BRDF and the NdotL from Lambert's law).
        float3 L = CosineSampleHemisphere(Xi, N, pdf);

        float NdotL = L.y;
        if (NdotL > 0.0)
        {
            float3 H = normalize(V + L);
            float LdotH = dot(L, H);
            irradiance += Fr_DisneyDiffuse(NdotV, NdotL, LdotH, perceptualRoughness);
        }
    }
    return irradiance / SAMPLE_COUNT;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void integrateBRDFShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= BRDF_LUT_SIZE))
    {
        return;
    }
    
    float NdotV = (dispatchThreadId.x + 0.5) / BRDF_LUT_SIZE;
    float perceptualRoughness = (dispatchThreadId.y + 0.5) / BRDF_LUT_SIZE;
    RWTexture2D<float4> targetTexture = ResourceDescriptorHeap[constants.targetTexture];
	targetTexture[dispatchThreadId.xy] = float4(IntegrateDFG(NdotV, perceptualRoughness), IntegrateDiffuse(NdotV, perceptualRoughness));
}
