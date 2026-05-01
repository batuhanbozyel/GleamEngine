#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#if defined(__cplusplus)
#include "Renderer/Shaders/ShaderInterop.h"
#endif

namespace Gleam {

#define CAMERA_UNIFORMS_BINDING_SLOT 5
#define SKY_ATMOSPHERE_PARAMS_BINDING_SLOT 6
#define SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT 7
#define PATH_TRACER_CONSTANTS_BINDING_SLOT 8

struct SkyAtmosphereUniforms
{
	float3 sunIlluminance;
	float sunAngularDiameter;

	float3 sunDirection;
	float pad0;

	ShaderResourceIndex transmittanceLutTexture;
	ShaderResourceIndex multiScatterLutTexture;
	float2 pad1;
};

struct CameraUniforms
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
	float4x4 viewProjectionMatrix;
	float4x4 invViewMatrix;
	float4x4 invProjectionMatrix;
	float4x4 invViewProjectionMatrix;

	float3 position;
	float pad0;

	float2 resolution;
	float2 pad1;
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
#endif // SHARED_TYPES_H
