#pragma once
#if defined(__cplusplus)
#include "Renderer/Shaders/ShaderInterop.h"
#endif

namespace Gleam {

struct SunUniforms
{
	float3 direction;
	float angularDiameter;

	float3 illuminance;
	float pad1;
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

} // namespace Gleam
