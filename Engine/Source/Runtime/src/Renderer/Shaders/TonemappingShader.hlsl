#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::TonemapUniforms, uniforms);

float4 tonemappingFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    Texture2D finalColorRT = ResourceDescriptorHeap[uniforms.sceneRT];
    return finalColorRT.Sample(Sampler_Point_Clamp, IN.texCoord);
}
