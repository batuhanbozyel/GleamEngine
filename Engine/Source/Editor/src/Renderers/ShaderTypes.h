#pragma once

#if defined(__cplusplus)
#include "Renderer/Shaders/ShaderInterop.h"
#endif

#define SELECTION_MASK_GROUP_SIZE_X 8u
#define SELECTION_MASK_GROUP_SIZE_Y 8u

namespace GEditor {

struct InfiniteGridUniforms
{
	uint32_t majorGridDivision;
	float majorLineWidth;
	float minorLineWidth;
	float pad0;

	uint32_t majorLineColor;
	uint32_t minorLineColor;
	float pad1;
	float pad2;
};

struct ViewModeUniforms
{
	Gleam::ShaderResourceIndex sourceTexture;
	uint32_t mode;
	float pad0;
	float pad1;
};

struct MeshletVisualizationConstants
{
	Gleam::ShaderResourceIndex instanceBuffer;
	uint32_t instanceID;
	float pad0;
	float pad1;
};

struct SelectionMaskConstants
{
	Gleam::ShaderResourceIndex visibilityBuffer;
	Gleam::ShaderResourceIndex instanceMaskBuffer;
	Gleam::UnorderedAccessIndex selectionMask;
	uint32_t targetWidth;

	uint32_t targetHeight;
	float pad0;
	float pad1;
	float pad2;
};

struct SelectionOutlineUniforms
{
	Gleam::ShaderResourceIndex selectionMask;
	float outlineWidth;
	float pad0;
	float pad1;

	float4 outlineColor;
};

} // namespace Gleam
