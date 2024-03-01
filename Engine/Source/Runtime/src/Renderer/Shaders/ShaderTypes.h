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

struct DebugMeshUniforms
{
    float4x4 modelMatrix;
	uint32_t color;
};

struct DebugShaderResources
{
    ShaderResourceIndex vertexBuffer;
    ShaderResourceIndex cameraBuffer;
};

struct ForwardPassUniforms
{
    float4x4 modelMatrix;
    ShaderResourceIndex positionBuffer;
    ShaderResourceIndex interleavedBuffer;
};

struct TonemapUniforms
{
    ShaderResourceIndex sceneRT;
};

} // namespace Gleam
