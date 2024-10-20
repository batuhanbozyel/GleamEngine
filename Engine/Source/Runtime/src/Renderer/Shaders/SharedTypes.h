#pragma once
#if defined(__cplusplus)
#include "Renderer/Shaders/ShaderInterop.h"
#endif

namespace Gleam {

struct CameraUniforms
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
	float4x4 viewProjectionMatrix;
	float4x4 invViewMatrix;
	float4x4 invProjectionMatrix;
	float4x4 invViewProjectionMatrix;

	float3 worldPosition;
};

} // namespace Gleam
