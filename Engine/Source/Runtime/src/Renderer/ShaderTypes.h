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
typedef uint Color32;

float4 unpack_unorm4x8_to_float(uint packedVal)
{
    return float4
    {
        float(packedVal & 0x000000ff) / 255.0f,
        float((packedVal >> 8) & 0x000000ff) / 255.0f,
        float((packedVal >> 16) & 0x000000ff) / 255.0f,
        float(packedVal >> 24) / 255.0f
    };
}

#elif defined(__METAL_VERSION__)
typedef packed_float2 Vector2;
typedef packed_float3 Vector3;
typedef packed_float4 Vector4;
typedef uint Color32;
#endif

#pragma pack(push, 1)
struct MeshVertex
{
	Vector3 position;
	Vector3 normal;
	Vector2 texCoord;
};

struct DebugVertex
{
	Vector3 position;
	Color32 color;
};

struct ForwardPassFragmentUniforms
{
    Color32 color;
};
#pragma pack(pop)

} // namespace Gleam
