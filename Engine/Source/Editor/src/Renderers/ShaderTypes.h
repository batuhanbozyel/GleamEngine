#pragma once

#if defined(__cplusplus)
#include "Renderer/Shaders/ShaderInterop.h"
#endif

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

} // namespace Gleam
