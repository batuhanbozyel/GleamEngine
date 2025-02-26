#pragma once
#include "SharedTypes.h"

namespace Gleam {

struct InterleavedMeshVertex
{
    float3 normal;
	float4 tangent;
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
};

struct MeshPassResources
{
	float4x4 modelMatrix;

	BufferResourceView positionBuffer;
	BufferResourceView interleavedBuffer;
    BufferResourceView materialBuffer;

	uint32_t baseVertex;
	uint32_t materialID;
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
