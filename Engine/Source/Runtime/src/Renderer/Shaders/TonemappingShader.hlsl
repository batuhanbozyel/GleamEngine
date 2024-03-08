#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::TonemapUniforms, uniforms);

float4 tonemappingFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    return uniforms.sceneRT.Sample(Sampler_Point_Clamp, IN.texCoord);
}
