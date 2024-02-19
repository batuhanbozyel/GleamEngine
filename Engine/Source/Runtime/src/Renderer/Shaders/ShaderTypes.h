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
    uint32_t color;
};

struct DebugShaderUniforms
{
    float4x4 modelMatrix;
	uint32_t color;
};

struct ForwardPassUniforms
{
    float4x4 modelMatrix;
};

} // namespace Gleam
