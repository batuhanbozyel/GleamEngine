#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

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

#define BRDF_LUT_SIZE 256
struct BRDFLutConstants
{
	ShaderResourceIndex targetTexture;
};

#define SPECULAR_RADIANCE_MAX_MIP_LEVEL 5
struct ProbeConvolutionConstants
{
	ShaderResourceIndex sourceTexture;
	ShaderResourceIndex targetTexture;
	uint32_t resolution;
	uint32_t level;
};

struct GenerateCubemapMipsConstants
{
	ShaderResourceIndex texture;
	uint32_t resolution;
	uint32_t level;
	float pad0;
};

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
	ShaderResourceIndex sceneColor;
};

#define MESH_PASS_RESOURCES_BINDING_SLOT 0
#define MESH_INSTANCE_DATA_BINDING_SLOT 1
struct MeshInstanceData
{
	float4x4 transform;

	ShaderResourceIndex positionBuffer;
	ShaderResourceIndex interleavedBuffer;
	ShaderResourceIndex indexBuffer;
	float pad0;

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

struct SkyAtmosphereRenderConstants
{
	ShaderResourceIndex targetTexture;
	ShaderResourceIndex depthTexture;
	uint32_t renderSun;
	float pad1;
};

} // namespace Gleam
#endif // SHADER_TYPES_H