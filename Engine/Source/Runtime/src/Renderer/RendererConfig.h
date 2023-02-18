#pragma once
#include "TextureFormat.h"

namespace Gleam {

struct RendererConfig
{
	TextureFormat format = TextureFormat::B8G8R8A8_UNorm;
	uint32_t sampleCount = 1;
	bool vsync = true;
	bool tripleBufferingEnabled = true;
};

} // namespace Gleam
