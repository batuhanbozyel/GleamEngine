#pragma once
#include "TextureFormat.h"

namespace Gleam {

struct RendererConfig
{
	uint32_t sampleCount = 1;
	bool vsync = true;
	bool tripleBufferingEnabled = true;
};

} // namespace Gleam
