#pragma once
#include "ShaderInterop.h"
#include "SharedTypes.h"

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
	uint32_t baseVertex;
	uint32_t color;
};

struct DebugShaderResources
{
    BufferResourceView vertexBuffer;
    ConstantBufferView cameraBuffer;
};

struct ForwardPassUniforms
{
    float4x4 modelMatrix;
    ConstantBufferView cameraBuffer;
    BufferResourceView positionBuffer;
    BufferResourceView interleavedBuffer;
	uint32_t baseVertex;
};

struct TonemapUniforms
{
    Texture2DResourceView<float4> sceneRT;
};

} // namespace Gleam
