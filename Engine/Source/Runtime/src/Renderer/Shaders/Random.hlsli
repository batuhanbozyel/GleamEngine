#ifndef RANDOM_HLSL
#define RANDOM_HLSL

#include "Common.hlsli"

#define PCGSeed uint4

// PCG random numbers generator
// Source: "Hash Functions for GPU Rendering" by Jarzynski & Olano
PCGSeed pcg4d(PCGSeed v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    v = v ^ (v >> 16u);

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    return v;
}

float rand(inout PCGSeed seed)
{
    seed.w++;
    return UIntToFloat(pcg4d(seed).x);
}

float2 rand2(inout PCGSeed seed)
{
	return float2(rand(seed), rand(seed));
}

#endif // RANDOM_HLSL