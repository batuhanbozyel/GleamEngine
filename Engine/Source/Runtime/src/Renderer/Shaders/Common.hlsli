#pragma once
#include "ShaderInterop.h"

#ifdef __spirv__
#define PUSH_CONSTANT(type, name) [[vk::push_constant]] type name
#else
#define PUSH_CONSTANT(type, name) ConstantBuffer<type> name : register(b999)
#endif

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

float3 ClipSpaceToViewSpace(float3 position, float4x4 invProjectionMatrix)
{
    float4 viewPosition = mul(invProjectionMatrix, float4(position, 1.0));
    return viewPosition.xyz /= viewPosition.w;
}

float3 ViewSpaceToWorldSpace(float3 position, float4x4 invViewMatrix)
{
    float4 worldPosition = mul(invViewMatrix, float4(position, 1.0));
    return worldPosition.xyz;
}

float3 ClipSpaceToWorldSpace(float3 position, float4x4 invViewProjectionMatrix)
{
    float4 worldPosition = mul(invViewProjectionMatrix, float4(position, 1.0));
    return worldPosition.xyz /= worldPosition.w;
}

float3 ClipSpaceToWorldSpace(float3 position, float4x4 invViewMatrix, float4x4 invProjectionMatrix)
{
    float4 viewPosition = mul(invProjectionMatrix, float4(position, 1.0));
    viewPosition /= viewPosition.w;
    
    float4 worldPosition = mul(invViewMatrix, viewPosition);
    return worldPosition.xyz;
}
