#include "BRDF.hlsli"

#pragma compute diffuseIrradianceConvolutionShader
#pragma compute specularPrefilterConvolutionShader

PUSH_CONSTANT(Gleam::ProbeConvolutionConstants, constants);

[numthreads(16, 16, 1)]
void diffuseIrradianceConvolutionShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    
}

[numthreads(16, 16, 1)]
void specularPrefilterConvolutionShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    
}