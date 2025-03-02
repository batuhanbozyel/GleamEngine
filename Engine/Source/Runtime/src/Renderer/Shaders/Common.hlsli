#pragma once
#include "ShaderInterop.h"
#include "SharedTypes.h"

#define PI 3.1415926535897932384626433832795

#define CONSTANT_BUFFER(type, name, slot) ConstantBuffer<type> name : register(b##slot)
#define PUSH_CONSTANT(type, name) CONSTANT_BUFFER(type, name, 999)

struct FScreenVertexOutput
{
    noperspective float4 position : SV_POSITION;
	float2 texCoord : ATTRIB0;
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

half3 LinearTosRGB(half3 lin)
{
    return select(lin < 0.00313067, lin * 12.92, pow(lin, (1.0/2.4)) * 1.055 - 0.055);
}

half3 sRGBToLinear(half3 color)
{
    color = max(6.10352e-5, color); // minimum positive non-denormal (fixes black problem on DX11 AMD and NV)
    return select(color > 0.04045, pow(color * (1.0 / 1.055) + 0.0521327, 2.4), color * (1.0 / 12.92));
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
