#pragma once
#include "ShaderInterop.h"

namespace Gleam {

struct InterleavedMeshVertex
{
    float3 normal;
    float2 texCoord;
};

struct DebugVertex
{
    float3 position;
    uint color;
};

struct CameraUniforms
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
};

struct DebugShaderUniforms
{
    float4x4 modelMatrix;
    uint color;
};

struct ForwardPassUniforms
{
    float4x4 modelMatrix;
};

} // namespace Gleam
