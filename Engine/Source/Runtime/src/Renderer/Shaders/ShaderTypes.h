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

struct TonemapUniforms
{
	Texture2DResourceView<float4> sceneColor;
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

struct SkyAtmosphereParameters
{
	// Radius of the planet (center to ground)
	float bottomRadius;
	// Maximum considered atmosphere height (center to atmosphere top)
	float topRadius;

	// Rayleigh scattering exponential distribution scale in the atmosphere
	float rayleighDensityExpScale;
	// Rayleigh scattering coefficients
	float3 rayleighScattering;

	// Mie scattering exponential distribution scale in the atmosphere
	float mieDensityExpScale;
	// Mie scattering coefficients
	float3 mieScattering;
	// Mie extinction coefficients
	float3 mieExtinction;
	// Mie absorption coefficients
	float3 mieAbsorption;
	// Mie phase function excentricity
	float miePhaseG;

	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;
	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float3 absorptionExtinction;

	// The albedo of the ground.
	float3 groundAlbedo;
};

struct SkyAtmosphereTransmittanceLutUniforms
{
	ShaderResourceIndex texture;
};

} // namespace Gleam
