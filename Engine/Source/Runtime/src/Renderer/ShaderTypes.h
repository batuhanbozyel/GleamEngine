#pragma once

#if defined(__METAL_VERSION__)
#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_math>
using namespace metal;
#endif

namespace Gleam {

#if defined(__HLSL_VERSION) || defined(__METAL_VERSION__)
typedef float2x2 Matrix2;
typedef float3x3 Matrix3;
typedef float4x4 Matrix4;
typedef float2 Vector2;
typedef float4 Vector3;
typedef float4 Vector4;
typedef uint Color32;
#endif

#if defined(__HLSL_VERSION)
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
#endif

struct InterleavedMeshVertex
{
    Vector3 normal;
    Vector2 texCoord;
};

struct DebugVertex
{
    Vector3 position;
    Color32 color;
};

struct CameraUniforms
{
    Matrix4 viewMatrix;
    Matrix4 projectionMatrix;
    Matrix4 viewProjectionMatrix;
};

struct DebugShaderUniforms
{
    Matrix4 modelMatrix;
    Color32 color;
};

struct ForwardPassUniforms
{
    Matrix4 modelMatrix;
};

} // namespace Gleam
