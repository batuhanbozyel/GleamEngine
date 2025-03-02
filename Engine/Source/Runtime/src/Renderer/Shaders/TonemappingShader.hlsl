#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::TonemapUniforms, uniforms);

#pragma fragment tonemappingFragmentShader

float4 tonemappingFragmentShader(FScreenVertexOutput IN) : SV_TARGET
{
    float4 color = uniforms.sceneColor.Sample(Sampler_Point_Clamp, IN.texCoord);
    color.rgb = LinearTosRGB(color.rgb);
    return color;
}
