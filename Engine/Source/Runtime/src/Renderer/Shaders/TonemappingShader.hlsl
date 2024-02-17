#include "Common.hlsli"

Texture2D finalColorRT : register(t0);

float4 tonemappingFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    return finalColorRT.Sample(Sampler_Point_Clamp, IN.texCoord);
}
