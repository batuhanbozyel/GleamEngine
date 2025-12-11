#pragma once
#include "ShaderInterop.h"
#include "SharedTypes.h"

#define EPSILON         1.0e-4
#define PI              3.14159265359
#define TWO_PI          6.28318530718
#define INV_PI          0.31830988618
#define HALF_PI         1.57079632679

#define HALF_MAX        65504.0 // (2 - 2^-10) * 2^15
#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN         1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX         3.402823466e+38 // Maximum representable floating-point number

#define CONSTANT_BUFFER_HELPER(type, name, slot) ConstantBuffer<type> name : register(b##slot, space0)
#define CONSTANT_BUFFER(type, name, slot) CONSTANT_BUFFER_HELPER(type, name, slot)
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

// https://twitter.com/SebAaltonen/status/878250919879639040
// madd_sat + madd
float FastSign(float x)
{
	return saturate(x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

float2 FastSign(float2 x)
{
	return saturate(x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

float3 FastSign(float3 x)
{
	return saturate(x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

float4 FastSign(float4 x)
{
	return saturate(x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

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

// - r0: ray origin
// - rd: normalized ray direction
// - s0: sphere center
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
float RaySphereIntersectNearest(float3 r0, float3 rd, float3 s0, float sR)
{
	float a = dot(rd, rd);
	float3 s0_r0 = r0 - s0;
	float b = 2.0 * dot(rd, s0_r0);
	float c = dot(s0_r0, s0_r0) - (sR * sR);
	float delta = b * b - 4.0 * a * c;
	if (delta < 0.0 || a == 0.0)
	{
		return -1.0;
	}
	float sol0 = (-b - sqrt(delta)) / (2.0 * a);
	float sol1 = (-b + sqrt(delta)) / (2.0 * a);
	if (sol0 < 0.0 && sol1 < 0.0)
	{
		return -1.0;
	}
	if (sol0 < 0.0)
	{
		return max(0.0, sol1);
	}
	else if (sol1 < 0.0)
	{
		return max(0.0, sol0);
	}
	return max(0.0, min(sol0, sol1));
}