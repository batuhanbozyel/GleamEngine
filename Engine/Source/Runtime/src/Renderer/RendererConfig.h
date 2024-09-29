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

GLEAM_TYPE(Gleam::RendererConfig, Guid("49EEFEB3-76C0-43DF-8E45-098E8ACCD8D9"))
	GLEAM_FIELD(sampleCount, Serializable())
	GLEAM_FIELD(vsync, Serializable())
	GLEAM_FIELD(tripleBufferingEnabled, Serializable())
GLEAM_END