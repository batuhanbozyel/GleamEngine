#pragma once

namespace Gleam {

struct RendererProperties
{
	uint32_t maxFramesInFlight = 3;
	uint32_t sampleCount = 1;
	bool vsync = true;
	bool tripleBufferingEnabled = true;
};

} // namespace Gleam
