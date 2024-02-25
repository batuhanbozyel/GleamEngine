#pragma once

#if defined(__cplusplus)
using float2x2 = Gleam::Matrix2;
using float3x3 = Gleam::Matrix3;
using float4x4 = Gleam::Matrix4;
using float2 = Gleam::Vector2;
using float3 = Gleam::Vector3;
using float4 = Gleam::Vector4;
#endif

#ifndef __spirv__
#define PUSH_CONSTANT_SLOT 999
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
