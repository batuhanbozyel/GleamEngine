#pragma once

struct FScreenVertexOutput
{
    noperspective float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

SamplerState Sampler_Point_Repeat : register(s0);
SamplerState Sampler_Point_Clamp : register(s1);
SamplerState Sampler_Point_Mirror : register(s2);
SamplerState Sampler_Point_MirrorOnce : register(s3);

SamplerState Sampler_Bilinear_Repeat : register(s4);
SamplerState Sampler_Bilinear_Clamp : register(s5);
SamplerState Sampler_Bilinear_Mirror : register(s6);
SamplerState Sampler_Bilinear_MirrorOnce : register(s7);

SamplerState Sampler_Trilinear_Repeat : register(s8);
SamplerState Sampler_Trilinear_Clamp : register(s9);
SamplerState Sampler_Trilinear_Mirror : register(s10);
SamplerState Sampler_Trilinear_MirrorOnce : register(s11);

float4 unpack_unorm4x8_to_float(uint packedVal)
{
    return float4
    (
        float(packedVal & 0x000000ff) / 255.0f,
        float((packedVal >> 8) & 0x000000ff) / 255.0f,
        float((packedVal >> 16) & 0x000000ff) / 255.0f,
        float(packedVal >> 24) / 255.0f
    );
}