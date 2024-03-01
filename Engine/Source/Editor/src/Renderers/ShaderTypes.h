#pragma once

#if defined(__cplusplus)
#include "Renderer/Shaders/ShaderInterop.h"
#endif

namespace GEditor {

struct InfiniteGridUniforms
{
    Gleam::ShaderResourceIndex cameraBuffer;
    
	uint32_t majorGridDivision;

	float majorLineWidth;
	float minorLineWidth;

	uint32_t majorLineColor;
	uint32_t minorLineColor;
};

} // namespace Gleam
