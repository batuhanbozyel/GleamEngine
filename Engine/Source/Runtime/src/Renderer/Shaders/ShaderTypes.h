#pragma once
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

struct MeshPassResources
{
	ConstantBufferView cameraBuffer;
	BufferResourceView positionBuffer;
	BufferResourceView interleavedBuffer;
    BufferResourceView materialBuffer;
    uint32_t materialInstanceId;
};

struct ForwardPassUniforms
{
	float4x4 modelMatrix;
	uint32_t baseVertex;
};

struct TonemapUniforms
{
	Texture2DResourceView<float4> sceneRT;
};

struct SurfaceInput
{
	float4 position;
	float3 worldNormal;
	float3 color;
	float2 uv;
};

struct SurfaceOutput
{
	float4 albedo;
	float4 emission;
	float3 normal;
	float metallic;
	float roughness;
};

} // namespace Gleam
