#include "../../ShaderTypes.h"
#include "../../RendererBindingTable.h"

struct VertexOut
{
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut fullscreenTriangleVertexShader(uint vertexID [[vertex_id]])
{
    VertexOut out;
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    out.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    out.texCoord = uv;
    return out;
}

fragment float4 fullscreenTriangleFragmentShader(VertexOut in [[stage_in]])
{
    return float4(in.texCoord, 0.0, 1.0);
}

fragment float4 tonemappingFragmentShader(VertexOut in [[stage_in]],
                                          texture2d<float, access::sample> finalColorRT[[texture(0)]])
{
    constexpr sampler linearSampler(mip_filter::nearest,
                                    mag_filter::linear,
                                    min_filter::linear);
    
    return finalColorRT.sample(linearSampler, in.texCoord);
}
