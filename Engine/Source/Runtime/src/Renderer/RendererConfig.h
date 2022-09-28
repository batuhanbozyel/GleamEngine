#pragma once

namespace Gleam {

struct RendererProperties
{
    Size size = Size::zero;
	uint32_t maxFramesInFlight = 3;
	uint32_t sampleCount = 1;
	bool vsync = true;
	bool tripleBufferingEnabled = true;
};

} // namespace Gleam
