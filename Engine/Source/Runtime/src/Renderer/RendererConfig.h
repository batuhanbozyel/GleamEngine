#pragma once

namespace Gleam {

struct RendererProperties
{
	uint32_t width = 0, height = 0;
    uint32_t maxFramesInFlight = 3;
	uint32_t msaa = 1;
	bool vsync = true;
	bool multisampleEnabled = false;
	bool tripleBufferingEnabled = true;
};

} // namespace Gleam
