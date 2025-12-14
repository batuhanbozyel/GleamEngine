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
	SunUniforms sun;
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
	// Rayleigh scattering coefficients
	float3 rayleighScattering;
	// Rayleigh scattering exponential distribution scale in the atmosphere
	float rayleighDensityExpScale;

	// Mie scattering coefficients
	float3 mieScattering;
	// Mie scattering exponential distribution scale in the atmosphere
	float mieDensityExpScale;
	
	// Mie extinction coefficients
	float3 mieExtinction;
	// Mie phase function excentricity
	float miePhaseG;

	// Mie absorption coefficients
	float3 mieAbsorption;

	// Radius of the planet (center to ground)
	float bottomRadius;

	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float3 absorptionExtinction;

	// Maximum considered atmosphere height (center to atmosphere top)
	float topRadius;

	// The albedo of the ground.
	float3 groundAlbedo;

	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;
};

struct SkyAtmosphereCommonUniforms
{
	float3 sunIlluminance;
	ShaderResourceIndex transmittanceLutTexture;

	float3 sunDirection;
	ShaderResourceIndex multiScatterLutTexture;

	float2 rayMarchMinMaxSPP;
	ShaderResourceIndex skyViewLutTexture;
	ShaderResourceIndex depthTexture;
};

} // namespace Gleam
