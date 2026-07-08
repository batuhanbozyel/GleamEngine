#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#include "SharedTypes.h"

#define SHADOW_TILE_WIDTH  8u
#define SHADOW_TILE_HEIGHT 4u

namespace Gleam {

#ifndef __cplusplus
// HLSL-side mirror of Gleam::CullMode (Renderer/PipelineStateDescriptor.h).
enum class CullMode
{
	Off,
	Front,
	Back
};

struct InterleavedMeshVertex
{
	float3 normal;
	float4 tangent;
	float2 texCoord;
};

struct MeshletDescriptor
{
	float3 center;
	float radius;
	float3 coneApex;
	float3 coneAxis;
	float coneCutoff;
	uint vertexOffset;
	uint triangleOffset;
	uint vertexCount;
	uint triangleCount;
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

struct DepthPrepassConstants
{
	ShaderResourceIndex instanceBuffer;
	uint32_t instanceID;
	float pad0;
	float pad1;
};

struct MeshShadingConstants
{
	ShaderResourceIndex instanceBuffer;
	ShaderResourceIndex diffuseReflectionTexture;
	ShaderResourceIndex specularReflectionTexture;
	ShaderResourceIndex brdfTexture;

	ShaderResourceIndex ggxEssTexture;
	ShaderResourceIndex ggxEAvgTexture;
	ShaderResourceIndex shadowTexture;
	uint32_t instanceID;
};

struct MeshInstanceData
{
	float4x4 transform;
	float4x4 previousTransform;

	ShaderResourceIndex positionBuffer;
	ShaderResourceIndex interleavedBuffer;
	ShaderResourceIndex indexBuffer;
	ShaderResourceIndex materialBuffer;

	uint32_t baseVertex;
	uint32_t indexCount;
	uint32_t firstIndex;
	uint32_t materialID;

	ShaderResourceIndex meshletsBuffer;
	ShaderResourceIndex meshletVertexBuffer;
	ShaderResourceIndex meshletTriangleBuffer;
	uint32_t baseMeshlet;

	uint32_t meshletCount;
	uint32_t cullMode;
	uint32_t batchIndex;
	float pad0;
};

struct VisibilityClassifyConstants
{
	ShaderResourceIndex visibilityBuffer;
	UnorderedAccessIndex countsBuffer;
	UnorderedAccessIndex cursorsBuffer;
	UnorderedAccessIndex pixelListBuffer;
};

struct VisibilityAllocateConstants
{
	ShaderResourceIndex countsBuffer;
	UnorderedAccessIndex offsetsBuffer;
	UnorderedAccessIndex cursorsBuffer;
	UnorderedAccessIndex dispatchArgsBuffer;

	uint32_t numBatches;
	float pad0;
	float pad1;
	float pad2;
};

struct VisibilityShadingConstants
{
	ShaderResourceIndex instanceBuffer;
	ShaderResourceIndex visibilityBuffer;
	ShaderResourceIndex pixelListBuffer;
	ShaderResourceIndex offsetsBuffer;

	ShaderResourceIndex countsBuffer;
	UnorderedAccessIndex colorTarget;
	ShaderResourceIndex brdfTexture;
	ShaderResourceIndex ggxEssTexture;

	ShaderResourceIndex ggxEAvgTexture;
	ShaderResourceIndex diffuseReflectionTexture;
	ShaderResourceIndex specularReflectionTexture;
	ShaderResourceIndex shadowTexture;

	uint32_t batchIndex;
	float pad0;
	float pad1;
	float pad2;
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

struct RayTracedSunShadowConstants
{
	ShaderResourceIndex depthTexture;
	ShaderResourceIndex normalTexture;
	ShaderResourceIndex tileBuffer;
	ShaderResourceIndex tileCountBuffer;
};

struct RayTracedSunShadowClassificationConstants
{
	ShaderResourceIndex  depthTexture;
	ShaderResourceIndex  normalTexture;
	UnorderedAccessIndex tileBuffer;
	UnorderedAccessIndex tileCountBuffer;

	UnorderedAccessIndex rayHitTexture;
	uint32_t tileTolerance;
	float pad0;
	float pad1;
};

struct PrepareShadowRayDispatchArgsConstants
{
	ShaderResourceIndex  tileCountBuffer;
	UnorderedAccessIndex dispatchArgsBuffer;
	float pad0;
	float pad1;
};

struct ShadowDenoiserTileClassificationConstants
{
	float4x4 reprojectionMatrix;

	ShaderResourceIndex hitMaskResults;
	ShaderResourceIndex depth;
	ShaderResourceIndex velocity;
	ShaderResourceIndex previousDepth;
	
	ShaderResourceIndex previousMoments;
	ShaderResourceIndex historyShadow;
	UnorderedAccessIndex tileMetadata;
	UnorderedAccessIndex currentMoments;
	
	UnorderedAccessIndex reprojectionResults;
	ShaderResourceIndex normalTexture;
	uint32_t isFirstFrame;
	float pad0;
};

struct ShadowDenoiserDepthCopyConstants
{
	ShaderResourceIndex  sourceDepth;
	UnorderedAccessIndex destDepth;
	float pad0;
	float pad1;
};

struct ShadowDenoiserFilterConstants
{
	ShaderResourceIndex depth;
	ShaderResourceIndex tileMetadata;
	ShaderResourceIndex filterInput;
	UnorderedAccessIndex history;

	UnorderedAccessIndex shadowMaskOutput;
	ShaderResourceIndex normalTexture;
	uint32_t passIndex;
	float pad0;
};

struct DrawIndirectArguments
{
	uint32_t vertexCountPerInstance;
	uint32_t instanceCount;
	uint32_t startVertexLocation;
	uint32_t startInstanceLocation;
};

struct DrawIndexedIndirectArguments
{
	uint32_t indexCountPerInstance;
	uint32_t instanceCount;
	uint32_t startIndexLocation;
	int32_t  baseVertexLocation;
	uint32_t startInstanceLocation;
};

struct DispatchIndirectArguments
{
	uint32_t threadGroupCountX;
	uint32_t threadGroupCountY;
	uint32_t threadGroupCountZ;
};

} // namespace Gleam
#endif // SHADER_TYPES_H
