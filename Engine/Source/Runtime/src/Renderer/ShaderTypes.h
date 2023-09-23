#pragma once

namespace Gleam {

#if defined(__HLSL_VERSION)
typedef float2x2 Matrix2;
typedef float3x3 Matrix3;
typedef float4x4 Matrix4;
typedef float2 Vector2;
typedef float3 Vector3;
typedef float4 Vector4;
typedef uint Color32;
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
