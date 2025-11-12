#pragma once
#include "SharedTypes.h"

namespace Gleam {

#ifndef __cplusplus
struct InterleavedMeshVertex
{
	float3 normal;
	float4 tangent;
	float2 texCoord;
};
#endif

struct DebugVertex
{
    float3 position;
    uint32_t color;
};

struct DebugMeshUniforms
{
	float4x4 transform;
	uint32_t baseVertex;
	uint32_t color;
};

struct DebugShaderResources
{
	BufferResourceView vertexBuffer;
};

struct ImGuiResources
{
	float4x4 projMatrix;
	ShaderResourceIndex texture;
	ShaderResourceIndex vertexBuffer;
	uint32_t vertexOffset;
};

struct MeshInstanceData
{
	float4x4 transform;

	BufferResourceView positionBuffer;
	BufferResourceView interleavedBuffer;
	BufferResourceView indexBuffer;

	uint32_t baseVertex;
	uint32_t indexCount;
	uint32_t firstIndex;
	uint32_t materialID;
};

struct MeshPassResources
{
	BufferResourceView instanceBuffer;
	BufferResourceView materialBuffer;
};

struct TonemapUniforms
{
	Texture2DResourceView<float4> sceneColor;
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
