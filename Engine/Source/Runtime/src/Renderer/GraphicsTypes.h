#pragma once

#if defined(__METAL_VERSION__)
#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_math>
using namespace metal;
#endif

namespace Gleam {

#if defined(__HLSL_VERSION)
typedef float2 Vector2;
typedef float3 Vector3;
typedef float4 Vector4;
#elif defined(__METAL_VERSION__)
typedef packed_float2 Vector2;
typedef packed_float3 Vector3;
typedef packed_float4 Vector4;
#endif

#pragma pack(push, 1)
struct Vertex
{
	Vector3 position;
	Vector3 normal;
	Vector2 texCoord;
};

struct ForwardPassFragmentUniforms
{
    Vector4 color;
};
    
#pragma pack(pop)

} // namespace Gleam
