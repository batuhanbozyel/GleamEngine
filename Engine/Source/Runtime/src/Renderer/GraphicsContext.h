#pragma once

#ifdef USE_VULKAN_RENDERER
#include <volk.h>
#define GraphicsDevice VkDevice
#else
#include <Metal/Metal.hpp>
#define GraphicsDevice MTL::Device*
#endif

struct SDL_Window;

namespace Gleam {

struct Version;

class GraphicsContext
{
public:

	static void Create(SDL_Window* window, const TString& appName, const Version& appVersion);
	static void Destroy();

	static GraphicsDevice GetDevice();

};

} // namespace Gleam