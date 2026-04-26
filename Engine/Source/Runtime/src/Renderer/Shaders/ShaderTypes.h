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
	UnorderedAccessIndex targetTexture;
};

struct MultiscatterGGXLutConstants
{
	UnorderedAccessIndex targetTexture;
	ShaderResourceIndex essTexture;
};

#define SPECULAR_RADIANCE_MAX_MIP_COUNT 5
struct ProbeConvolutionConstants
{
	ShaderResourceIndex sourceTexture;
	UnorderedAccessIndex targetTexture;
	uint32_t resolution;
	uint32_t level;

	uint32_t probeResolution;
	uint32_t face;
	float pad1;
	float pad2;
};

struct GenerateMipsConstants
{
	UnorderedAccessIndex sourceTexture;
	UnorderedAccessIndex targetTexture;
	uint32_t resolution;
	uint32_t level;

	uint32_t face;
	float pad0;
	float pad1;
	float pad2;
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
	ShaderResourceIndex vertexBuffer;
	float pad0;
	float pad1;
	float pad2;
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

struct MeshShadingConstants
{
	ShaderResourceIndex instanceBuffer;
	ShaderResourceIndex diffuseReflectionTexture;
	ShaderResourceIndex specularReflectionTexture;
	ShaderResourceIndex brdfTexture;

	ShaderResourceIndex ggxEssTexture;
	ShaderResourceIndex ggxEAvgTexture;
	uint32_t instanceID;
	float pad0;
};

struct MeshInstanceData
{
	float4x4 transform;

	ShaderResourceIndex positionBuffer;
	ShaderResourceIndex interleavedBuffer;
	ShaderResourceIndex indexBuffer;
	ShaderResourceIndex materialBuffer;

	uint32_t baseVertex;
	uint32_t indexCount;
	uint32_t firstIndex;
	uint32_t materialID;
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
	UnorderedAccessIndex targetTexture;
	ShaderResourceIndex depthTexture;
	float pad0;
	float pad1;
};

enum class RayType
{
	PrimaryRay = 0,
	ShadowRay = 1,
	COUNT = 2
};
typedef uint32_t RayTypeFlagBits;

struct RayPayload
{
	uint4 seed;
	float3 radiance;
	float3 throughput;
	uint32_t depth;
};

struct ShadowPayload
{
	float visibility;
};

struct PathTracerConstants
{
	ShaderResourceIndex accelerationStructure;
	ShaderResourceIndex instanceBuffer;
	UnorderedAccessIndex colorTarget;
	uint32_t frameIndex;

	ShaderResourceIndex ggxEssTexture;
	ShaderResourceIndex ggxEAvgTexture;
	uint32_t maxRayRecursionDepth;
	uint32_t samplesPerPixel;
};

} // namespace Gleam
#endif // SHADER_TYPES_H
