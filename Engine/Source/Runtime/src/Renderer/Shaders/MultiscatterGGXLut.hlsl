#include "BRDF.hlsli"

PUSH_CONSTANT(Gleam::MultiscatterGGXLutConstants, constants);

#define SAMPLE_COUNT (8192u * 2u)
float IntegrateEss(float NdotV, float perceptualRoughness)
{
    float3 N = float3(0.0, 1.0, 0.0);
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), NdotV, 0.0);
    float roughness = perceptualRoughness * perceptualRoughness;

    float E_ss = 0.0;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);

        float pdf;
        float3 H = ImportanceSampleGGX_VNDF(Xi, V, N, perceptualRoughness, pdf);
        float3 L = reflect(-V, H);

        float NdotL = L.y;
        if (NdotL > 0.0)
        {
            float G2  = G_SmithGGXCorrelated(NdotL, NdotV, roughness);
            float G1V = G1_SmithGGX(NdotV, roughness);

            float G_Vis = G2 / G1V;

            E_ss += G_Vis;
        }
    }
    return E_ss / SAMPLE_COUNT;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void integrateEssShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= BRDF_LUT_SIZE))
    {
        return;
    }
    
    float NdotV = (dispatchThreadId.x + 0.5) / BRDF_LUT_SIZE;
    float perceptualRoughness = max((dispatchThreadId.y + 0.5) / BRDF_LUT_SIZE, PERFECT_MIRROR_ROUGHNESS);

    RWTexture2D<float> target = ResourceDescriptorHeap[constants.targetTexture];
    target[dispatchThreadId.xy] = IntegrateEss(NdotV, perceptualRoughness);
}

float IntegrateEAvg(float perceptualRoughness, Texture2D<float> essLUT)
{
    float result = 0.0;
    for (uint i = 0u; i < BRDF_LUT_SIZE; ++i)
    {
        float NdotV = (i + 0.5) / BRDF_LUT_SIZE;
        float Ess = essLUT.SampleLevel(Sampler_Bilinear_Clamp, float2(NdotV, perceptualRoughness), 0);
        result += Ess * NdotV;
    }
    return 2.0 * result / BRDF_LUT_SIZE;
}

[shader("compute")]
[numthreads(16, 1, 1)]
void integrateEAvgShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= BRDF_LUT_SIZE)
    {
        return;
    }

    float perceptualRoughness = max((dispatchThreadId.x + 0.5) / float(BRDF_LUT_SIZE), PERFECT_MIRROR_ROUGHNESS);
    Texture2D<float> essTexture = ResourceDescriptorHeap[constants.essTexture];
    RWTexture2D<float> targetTexture = ResourceDescriptorHeap[constants.targetTexture];
    targetTexture[uint2(dispatchThreadId.x, 0)] = IntegrateEAvg(perceptualRoughness, essTexture);
}
